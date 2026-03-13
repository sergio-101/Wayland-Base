#include "main.h"

static void keyboard_keymap(void *data, struct wl_keyboard *keyboard,uint32_t format, int32_t fd, uint32_t size){
	char *keymap_string = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0);
	xkb_keymap_unref (keymap);
	keymap = xkb_keymap_new_from_string (xkb_context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	munmap (keymap_string, size);
	close (fd);
	xkb_state_unref (xkb_state);
	xkb_state = xkb_state_new (keymap);
}
static void keyboard_enter(void *data, struct wl_keyboard *keyboard,uint32_t serial, struct wl_surface *surface, struct wl_array *keys){}
static void keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface){}
static void keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group){}
static void keyboard_repeat_info(void *data, struct wl_keyboard *keyboard,int32_t rate, int32_t delay){}

static void key_listener(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state){
    if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		xkb_keysym_t keysym = xkb_state_key_get_one_sym (xkb_state, key+8);
		uint32_t utf32 = xkb_keysym_to_utf32 (keysym);
		if (utf32) {
			if (utf32 >= 0x21 && utf32 <= 0x7E) {
				printf ("the key %c was pressed\n", (char)utf32);
			}
			else {
				printf ("the key U+%04X was pressed\n", utf32);
			}
		}
		else {
			char name[64];
			xkb_keysym_get_name (keysym, name, 64);
			printf ("the key %s was pressed\n", name);
		}
	}
}


static void xdg_wm_base_ping(void *data, struct xdg_wm_base *xdg_wm_base, uint32_t serial){
    xdg_wm_base_pong(xdg_wm_base, serial);
}

void draw_text(uint32_t *pixels, const char *text, float x, float y, float size, uint32_t color) {
    static stbtt_fontinfo font;
    static int font_loaded = 0;
    static unsigned char *font_buf;

    if (!font_loaded) {
        FILE *f = fopen("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", "rb");
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        rewind(f);
        font_buf = malloc(len);
        fread(font_buf, 1, len, f);
        fclose(f);
        stbtt_InitFont(&font, font_buf, 0);
        font_loaded = 1;
    }

    float scale = stbtt_ScaleForPixelHeight(&font, size);
    int ascent, descent;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, NULL);
    int baseline = (int)(y + ascent * scale);

    float xpos = x;
    for (int i = 0; text[i]; i++) {
        int w, h, xoff, yoff;
        unsigned char *bitmap = stbtt_GetCodepointBitmap(
            &font, 0, scale, text[i], &w, &h, &xoff, &yoff);

        for (int row = 0; row < h; row++) {
            for (int col = 0; col < w; col++) {
                int px = (int)xpos + col + xoff;
                int py = baseline + row + yoff;
                if (px < 0 || px >= WIDTH || py < 0 || py >= HEIGHT) continue;
                uint8_t alpha = bitmap[row * w + col];
                if (alpha > 0) {
                    // Simple alpha blend over black background
                    uint8_t r = ((color >> 16) & 0xFF) * alpha / 255;
                    uint8_t g = ((color >>  8) & 0xFF) * alpha / 255;
                    uint8_t b = ((color >>  0) & 0xFF) * alpha / 255;
                    pixels[py * WIDTH + px] = (r << 16) | (g << 8) | b;
                }
            }
        }
        int advance, lsb;
        stbtt_GetCodepointHMetrics(&font, text[i], &advance, &lsb);
        xpos += advance * scale;
        stbtt_FreeBitmap(bitmap, NULL);
    }
}

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    xdg_surface_ack_configure(xdg_surface, serial);
    if(data && buffer){
        uint32_t *pixels = data;
        for(int i = 0; i < WIDTH * HEIGHT; ++i){
            pixels[i] = 0xFF000000;
        }
        draw_text(data, "Hello, World!", 100, 100, 50, 0x00FF0000);
        wl_surface_attach(surface, buffer, 0, 0);
        wl_surface_commit(surface);
    }
    printf("surface configured\n");
}

static void xdg_toplevel_configure(void *data, struct xdg_toplevel *toplevel, int32_t width, int32_t height, struct wl_array *states) {
    printf("WINDOW RECONFIGURED: %dx%d\n", width, height);
}

static void xdg_toplevel_close(void *data, struct xdg_toplevel *toplevel){
    printf("WINDOW CLOSED\n");
    exit(0);
}

static void global_registry_handler(
    void *data, 
    struct wl_registry *registry, 
    uint32_t id,
    const char *interface, 
    uint32_t version
){
    printf("Got a registry event for %s id %d\n", interface, id);
    if(!strcmp(interface, "wl_compositor")){
        compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    }
    else if(!strcmp(interface, "xdg_wm_base")){
        xdg_wm_base = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
        static const struct xdg_wm_base_listener xdg_wm_base_listener = {
            xdg_wm_base_ping
        };
        xdg_wm_base_add_listener(xdg_wm_base, &xdg_wm_base_listener, NULL);
    }
    else if(!strcmp(interface, "wl_shm")){
        shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
    }
    else if (strcmp(interface, "wl_seat") == 0){
        seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
    }

}
static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
    printf("Got a registry losing event for %d\n", id);
}
int main(int argc, char **argv) {

    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Can't connect to display\n");
        exit(1);
    }
    printf("connected to display\n");

    // Used to decode from the format of which wayland gives us the keys(raw_linux_keycode) -> UTF8;
    xkb_context = xkb_context_new (XKB_CONTEXT_NO_FLAGS);

    struct wl_registry *registry = wl_display_get_registry(display);
    static const struct wl_registry_listener registry_listener = {
        global_registry_handler,
        global_registry_remover
    };
    wl_registry_add_listener(registry, &registry_listener, NULL);
    wl_display_roundtrip(display);

    if (compositor == NULL || xdg_wm_base == NULL) { fprintf(stderr, "Can't find compositor or xdg_wm_base\n"); exit(1); }
    if(!seat){ fprintf(stderr, "Can't find Keyboard Seat"); exit(1);}

    // A shared memory b/w program and compositer; Program can write the pixles into it and compositer can display. Reduces the cost of constantly copying the buffer.
    int fd = memfd_create("buffer", 0);
    ftruncate(fd, size); 
    pool = wl_shm_create_pool(shm, fd, size);
    buffer = wl_shm_pool_create_buffer(pool, 0, WIDTH, HEIGHT, stride, WL_SHM_FORMAT_XRGB8888);
    void *data = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    struct wl_keyboard *keyboard = wl_seat_get_keyboard(seat);
    static const struct wl_keyboard_listener keyboard_callbacks = {
        .keymap      = keyboard_keymap,
        .enter       = keyboard_enter,
        .leave       = keyboard_leave,
        .key         = key_listener,
        .modifiers   = keyboard_modifiers,
        .repeat_info = keyboard_repeat_info
    };
    wl_keyboard_add_listener(keyboard, &keyboard_callbacks, data);

    // surface ~ pixels
    surface = wl_compositor_create_surface(compositor);
    // xdg_surface is just a wrapper over surface
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);
    // `xdg_surface_configure` callback runs whenever resized, or changed focus; 
    static const struct xdg_surface_listener xdg_surface_listener = {
        xdg_surface_configure
    };
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, data);
    //                                                            ^ THIS IS WHAT IS PASSED AS DATA POINTER IN XDG_SURFACE_CONFIGURE CALLBACK
    
    // promote xdg_surface to a top level window; dropdown, select etc are other than top level kind.
    top_window = xdg_surface_get_toplevel(xdg_surface);

    // listens event got from the window, like, when resized.
    static const struct xdg_toplevel_listener xdg_toplevel_listener = {
        xdg_toplevel_configure,
        xdg_toplevel_close
    };
    xdg_toplevel_add_listener(top_window, &xdg_toplevel_listener, NULL);

    // Obvious.
    xdg_toplevel_set_title(top_window, "My Window");

    // Commit to let compositor know surface is ready
    wl_surface_commit(surface);
    wl_display_roundtrip(display);

    // Event loop
    while (wl_display_dispatch(display) != -1) {
    }

    wl_display_disconnect(display);
    return 0;
}
