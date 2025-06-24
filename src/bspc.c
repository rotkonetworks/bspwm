/* Copyright (c) 2012, Bastien Dejean
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/un.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include "helpers.h"
#include "common.h"

#define MAX_ARGS 1024
#define MSG_CHUNK_SIZE 4096

int main(int argc, char *argv[])
{
	int sock_fd;
	struct sockaddr_un sock_address = {0};
	char *msg = NULL;
	char rsp[BUFSIZ];
	size_t msg_size = 0;
	size_t msg_capacity = MSG_CHUNK_SIZE;

	if (argc < 2) {
		err("No arguments given.\n");
	}

	if (argc > MAX_ARGS) {
		err("Too many arguments (max %d).\n", MAX_ARGS);
	}

	sock_address.sun_family = AF_UNIX;
	char *sp = getenv(SOCKET_ENV_VAR);

	if (sp != NULL) {
		size_t sp_len = strnlen(sp, sizeof(sock_address.sun_path));
		if (sp_len >= sizeof(sock_address.sun_path)) {
			err("Socket path too long.\n");
		}
		memcpy(sock_address.sun_path, sp, sp_len);
		sock_address.sun_path[sp_len] = '\0';
	} else {
		char *host = NULL;
		int dn = 0, sn = 0;
		if (xcb_parse_display(NULL, &host, &dn, &sn) != 0) {
			int ret = snprintf(sock_address.sun_path, sizeof(sock_address.sun_path),
			                   SOCKET_PATH_TPL, host, dn, sn);
			if (ret < 0 || (size_t)ret >= sizeof(sock_address.sun_path)) {
				free(host);
				err("Socket path too long.\n");
			}
		}
		free(host);
	}

	if (streq(argv[1], "--print-socket-path")) {
		printf("%s\n", sock_address.sun_path);
		return EXIT_SUCCESS;
	}

	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0)) == -1) {
		err("Failed to create the socket.\n");
	}

	if (connect(sock_fd, (struct sockaddr *) &sock_address, sizeof(sock_address)) == -1) {
		close(sock_fd);
		err("Failed to connect to the socket.\n");
	}

	msg = malloc(msg_capacity);
	if (msg == NULL) {
		close(sock_fd);
		err("Failed to allocate message buffer.\n");
	}

	argc--, argv++;

	for (int i = 0; i < argc; i++) {
		size_t arg_len = strlen(argv[i]) + 1;

		while (msg_size + arg_len > msg_capacity) {
			size_t new_capacity = msg_capacity * 2;
			if (new_capacity > INT_MAX) {
				free(msg);
				close(sock_fd);
				err("Message too large.\n");
			}
			char *new_msg = realloc(msg, new_capacity);
			if (new_msg == NULL) {
				free(msg);
				close(sock_fd);
				err("Failed to grow message buffer.\n");
			}
			msg = new_msg;
			msg_capacity = new_capacity;
		}

		memcpy(msg + msg_size, argv[i], arg_len - 1);
		msg[msg_size + arg_len - 1] = '\0';
		msg_size += arg_len;
	}

	ssize_t sent = 0;
	while (sent < (ssize_t)msg_size) {
		ssize_t n = send(sock_fd, msg + sent, msg_size - sent, MSG_NOSIGNAL);
		if (n == -1) {
			if (errno == EINTR) continue;
			free(msg);
			close(sock_fd);
			err("Failed to send the data.\n");
		}
		sent += n;
	}

	free(msg);

	int ret = EXIT_SUCCESS, nb;

	struct pollfd fds[] = {
		{sock_fd, POLLIN, 0},
		{STDOUT_FILENO, POLLHUP, 0},
	};

	while (poll(fds, 2, -1) > 0) {
		if (fds[0].revents & POLLIN) {
			if ((nb = recv(sock_fd, rsp, sizeof(rsp)-1, 0)) > 0) {
				rsp[nb] = '\0';
				if (rsp[0] == FAILURE_MESSAGE[0]) {
					ret = EXIT_FAILURE;
					fprintf(stderr, "%s", rsp + 1);
					fflush(stderr);
				} else {
					fprintf(stdout, "%s", rsp);
					fflush(stdout);
				}
			} else {
				break;
			}
		}
		if (fds[1].revents & (POLLERR | POLLHUP)) {
			break;
		}
	}

	close(sock_fd);
	return ret;
}
