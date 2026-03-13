#define _GNU_SOURCE
#include "glad/include/glad/gl.h"
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <xkbcommon/xkbcommon.h>
#include "xdg-shell-client-protocol.h"

#define WIDTH  (1920/2)
#define HEIGHT (1080/2)

// SHADER / TEXT
const char *vert_src =
    "#version 330 core\n"
    "layout(location = 0) in vec4 vertex;\n"
    "out vec2 TexCoords;\n"
    "uniform mat4 projection;\n"
    "void main() {\n"
    "    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);\n"
    "    TexCoords = vertex.zw;\n"
    "}\n";

const char *frag_src =
    "#version 330 core\n"
    "in vec2 TexCoords;\n"
    "out vec4 color;\n"
    "uniform sampler2D text;\n"
    "uniform vec3 textColor;\n"
    "void main() {\n"
    "    float alpha = texture(text, TexCoords).r;\n"
    "    color = vec4(textColor, alpha);\n"
    "}\n";


struct wl_display *display = NULL;
struct wl_seat *seat = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct xdg_wm_base *xdg_wm_base = NULL;
struct xdg_surface *xdg_surface = NULL;
struct xdg_toplevel *top_window = NULL;
static struct xkb_context *xkb_context;
static struct xkb_keymap *keymap = NULL;
static struct xkb_state *xkb_state = NULL;

typedef struct {
    unsigned int textureID;
    int width, height;
    int bearingX, bearingY;
    unsigned int advance;
} Glyph;
static Glyph glyphs[128];
static unsigned int VAO, VBO;
static unsigned int shader;
EGLDisplay egl_display;
EGLSurface egl_surface;
EGLContext egl_context;
struct wl_egl_window *egl_window;

void make_ortho(float *m, float l, float r, float b, float t);
unsigned int create_shader(const char *vert_path, const char *frag_path);
void init_freetype(const char *font_path, int font_size);
void render_text(const char *text, float x, float y, float scale, float r, float g, float b);
static void keyboard_keymap(void *data, struct wl_keyboard *keyboard,uint32_t format, int32_t fd, uint32_t size);
static void keyboard_enter(void *data, struct wl_keyboard *keyboard,uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface);
static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
static void key_listener(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial);
static void xdg_toplevel_configure(void *data, struct xdg_toplevel *toplevel, int32_t width, int32_t height, struct wl_array *states);
static void xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel);
static void global_registry_handler(
    void *data, 
    struct wl_registry *registry, 
    uint32_t id,
    const char *interface, 
    uint32_t version
);
static void global_registry_remover(void *data, struct wl_registry *registry, uint32_t id);
