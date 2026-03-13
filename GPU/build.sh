#!/bin/bash
gcc -o main xdg-shell-protocol.c main.c glad/src/gl.c \
    $(pkg-config --cflags --libs wayland-client wayland-egl egl freetype2 xkbcommon) \
    -lGL

