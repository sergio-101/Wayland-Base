#!/bin/bash
gcc -o main xdg-shell-protocol.c main.c \
    $(pkg-config --cflags --libs wayland-client xkbcommon) \
    -lGL

