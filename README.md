# Wayland Setup

Wayland Setup to render a basic window with text via CPU/GPU both, written in C from scratch, with keyboard input support. 

## Approaches

### CPU 
Writes pixel manually into a shared memory buffer shared by compositor and app

- Font rasterization via stb_truetype 
- Keyboard input via `wl_keyboard` + xkbcommon (raw keycodes → UTF-8)
<img width="1122" height="615" alt="image" src="https://github.com/user-attachments/assets/43dca0df-acd0-4f37-9ea7-9a5930dd6224" />


### GPU
Renders text using the GPU via OpenGL. FreeType rasterizes glyphs into textures, drawn as textured quads using a GLSL shader.

- Font rasterization via FreeType
- Glyph bitmaps uploaded as `GL_RED` textures
- Textured quads drawn with a custom vertex + fragment shader
- Keyboard input via `wl_keyboard` + xkbcommon (raw keycodes → UTF-8)
- EGL connects Wayland to OpenGL
- Double buffered
<img width="1122" height="615" alt="image" src="https://github.com/user-attachments/assets/044983aa-29dc-446e-96fe-e7816a8b5312" />


## Dependencies

### CPU
- libwayland-client
- xkbcommon
- stb_truetype (included as single header)

### GPU
- libwayland-client
- libwayland-egl
- libEGL
- libGL
- libfreetype2
- xkbcommon
- GLAD (included, generated for OpenGL 3.3 core)

## Structure
```
CPU/
    main.h
    main.c
    stb_truetype.h
    xdg-shell-client-protocol.h
    xdg-shell-protocol.c
    build.sh
    run.sh

GPU/
    main.h
    main.c
    xdg-shell-client-protocol.h
    xdg-shell-protocol.c
    glad/
    build.sh
    run.sh
```

## Building & Running

### CPU
```bash
cd CPU
./build.sh
./run.sh
```

### GPU
```bash
cd GPU
./build.sh
./run.sh
```

## Generating protocol files (both approaches)
```bash
wayland-scanner client-header \
    /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
    xdg-shell-client-protocol.h

wayland-scanner private-code \
    /usr/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml \
    xdg-shell-protocol.c
```

## Generating GLAD (GPU only)
```bash
glad --api gl:core=3.3 --out-path glad/ c
```
