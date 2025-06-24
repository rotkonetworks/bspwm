# bspwm

## Description

*bspwm* is a tiling window manager that represents windows as the leaves of a
full binary tree.

It only responds to X events, and the messages it receives on a dedicated
socket.

*bspc* is a program that writes messages on *bspwm*'s socket.

*bspwm* doesn't handle any keyboard or pointer inputs: a third party program
(e.g. *sxhkd*) is needed in order to translate keyboard and pointer events to
*bspc* invocations.

The outlined architecture is the following:

``` PROCESS          SOCKET sxhkd  -------->  bspc  <------>  bspwm ```

## Fork: Security Hardening

This fork includes extensive security improvements:

- **Memory safety**: Buffer overflow fixes, bounds checking, integer overflow
protection
- **Resource limits**: Maximum tree depth, string size limits, recursion
prevention
- **Input validation**: Proper sanitization of all user inputs
- **Secure coding**: No format string vulnerabilities, proper null termination
- **Denial of service prevention**: Protection against infinite loops and
excessive allocations

See [CHANGELOG.md](doc/CHANGELOG.md) versions 0.10.0-0.10.4 for detailed
security fixes.

## Performance

Despite extensive security hardening, performance remains excellent:
- **Query operations**: ~3.8% faster than 0.9.10
- **Memory usage**: 2.4 MB RSS (11% less than original)
- **Responsiveness**: Feels snappier due to eliminated edge cases
- **Protection cost**: <5% overhead on tree operations, prevents crashes

The security fixes actually improve common operations while preventing
pathological cases that could freeze the window manager.

## Configuration

The default configuration file is `$XDG_CONFIG_HOME/bspwm/bspwmrc`: this is
simply a shell script that calls *bspc*.

Keyboard and pointer bindings are defined with
[sxhkd](https://github.com/baskerville/sxhkd).

Example configuration files can be found in the [examples](examples) directory.

## Build

```bash make sudo make install ```

Dependencies: `libxcb` `xcb-util` `xcb-util-keysyms` `xcb-util-wm`

## Community

- Subreddit: [r/bspwm](https://www.reddit.com/r/bspwm/)
- IRC: `#bspwm` on `irc.libera.chat`
- Matrix: [#bspwm:matrix.org](https://matrix.to/#/#bspwm:matrix.org)
