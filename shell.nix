{ pkgs ? import <nixpkgs> {} }:
pkgs.mkShell {
  buildInputs = with pkgs; [
    xorg.libxcb
    xorg.xcbutil
    xorg.xcbutilkeysyms
    xorg.xcbutilwm

    gcc
    gnumake

    # headless X
    xorg.xvfb
    xorg.libX11

    # drive windows
    xdotool

	gdb
  ];
}

