#define STB_TRUETYPE_IMPLEMENTATION
#define _GNU_SOURCE
#include <EGL/egl.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon.h>
#include "xdg-shell-client-protocol.h"
#include "stb_truetype.h"

#define WIDTH  (1920/2)
#define HEIGHT (1080/2)

static unsigned int size = WIDTH * HEIGHT * 4;
static unsigned int stride = WIDTH * 4;

struct wl_display *display = NULL;
struct wl_shm *shm = NULL;
struct wl_seat *seat = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_shm_pool *pool = NULL;
struct wl_buffer *buffer = NULL;
struct xdg_wm_base *xdg_wm_base = NULL;
struct xdg_surface *xdg_surface = NULL;
struct xdg_toplevel *top_window = NULL;
static struct xkb_context *xkb_context;
static struct xkb_keymap *keymap = NULL;
static struct xkb_state *xkb_state = NULL;

static void keyboard_keymap(void *data, struct wl_keyboard *keyboard,uint32_t format, int32_t fd, uint32_t size);
static void keyboard_enter(void *data, struct wl_keyboard *keyboard,uint32_t serial, struct wl_surface *surface, struct wl_array *keys);
static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface);
static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group);
static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard,int32_t rate, int32_t delay);
static void key_listener(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial);
void draw_text(uint32_t *pixels, const char *text, float x, float y,float size, uint32_t color);
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
