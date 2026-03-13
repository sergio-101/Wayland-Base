#include "main.h"
void make_ortho(float *m, float l, float r, float b, float t) {
    memset(m, 0, 16 * sizeof(float));
    m[0]  =  2.0f / (r - l);
    m[5]  =  2.0f / (t - b);
    m[10] = -1.0f;
    m[12] = -(r + l) / (r - l);
    m[13] = -(t + b) / (t - b);
    m[15] =  1.0f;
}
unsigned int create_shader(const char *vert_path, const char *frag_path) {
    unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, (const char **)&vert_src, NULL);
    glCompileShader(vert);

    unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, (const char **)&frag_src, NULL);
    glCompileShader(frag);

    unsigned int program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

void init_freetype(const char *font_path, int font_size) {
    FT_Library ft;
    FT_Init_FreeType(&ft);

    FT_Face face;
    FT_New_Face(ft, font_path, 0, &face);
    FT_Set_Pixel_Sizes(face, 0, font_size);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (unsigned char c = 0; c < 128; c++) {
        FT_Load_Char(face, c, FT_LOAD_RENDER);

        unsigned int tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0, GL_RED, GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glyphs[c] = (Glyph){
            .textureID = tex,
            .width     = face->glyph->bitmap.width,
            .height    = face->glyph->bitmap.rows,
            .bearingX  = face->glyph->bitmap_left,
            .bearingY  = face->glyph->bitmap_top,
            .advance   = face->glyph->advance.x, // in 1/64 pixels
        };
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    // Setup VAO/VBO for quads (dynamic — updated per glyph)
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
}

void render_text(const char *text, float x, float y, float scale, float r, float g, float b) {
    glUseProgram(shader);
    glUniform3f(glGetUniformLocation(shader, "textColor"), r, g, b);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(VAO);

    // Enable blending for alpha
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const char *c = text; *c; c++) {
        Glyph *g = &glyphs[(unsigned char)*c];

        float xpos = x + g->bearingX * scale;
        float ypos = y - (g->height - g->bearingY) * scale;
        float w = g->width  * scale;
        float h = g->height * scale;

        float vertices[6][4] = {
            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos,     ypos,     0.0f, 1.0f },
            { xpos + w, ypos,     1.0f, 1.0f },
            { xpos,     ypos + h, 0.0f, 0.0f },
            { xpos + w, ypos,     1.0f, 1.0f },
            { xpos + w, ypos + h, 1.0f, 0.0f },
        };

        glBindTexture(GL_TEXTURE_2D, g->textureID);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (g->advance >> 6) * scale; 
    }
}

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

static void xdg_surface_configure(void *data, struct xdg_surface *xdg_surface, uint32_t serial) {
    xdg_surface_ack_configure(xdg_surface, serial);
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
    else if (strcmp(interface, "wl_seat") == 0){
        seat = wl_registry_bind(registry, id, &wl_seat_interface, 1);
    }

}
static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id){
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

    // surface ~ pixels
    surface = wl_compositor_create_surface(compositor);

    // xdg_surface is just a wrapper over surface
    xdg_surface = xdg_wm_base_get_xdg_surface(xdg_wm_base, surface);

    // `xdg_surface_configure` callback runs whenever resized, or changed focus; 
    static const struct xdg_surface_listener xdg_surface_listener = {
        xdg_surface_configure
    };
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, NULL);
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

    // KEYBOARD LISTENER
    struct wl_keyboard *keyboard = wl_seat_get_keyboard(seat);
    static const struct wl_keyboard_listener keyboard_callbacks = {
        .keymap      = keyboard_keymap,
        .enter       = keyboard_enter,
        .leave       = keyboard_leave,
        .key         = key_listener,
        .modifiers   = keyboard_modifiers,
    };
    wl_keyboard_add_listener(keyboard, &keyboard_callbacks, NULL);

    // WL_DISPLAY -|> EGL_DISPLAY
    egl_display = eglGetDisplay((EGLNativeDisplayType)display);
    eglInitialize(egl_display, NULL, NULL);

    // Configure EGL
    EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_config;
    eglChooseConfig(egl_display, attribs, &config, 1, &num_config);
    eglBindAPI(EGL_OPENGL_API);
    EGLint ctx_attribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 3,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_NONE
    };
    // EGL CONTEXT on which OpenGL can draw
    egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT, ctx_attribs);

    // WL_SURFACE -|> EGL_WINDOW (+EGL_CONFIG +EGL_DISPLAY) -|> EGL_SURFACE
    egl_window  = wl_egl_window_create(surface, WIDTH, HEIGHT);
    egl_surface = eglCreateWindowSurface(egl_display, config, (EGLNativeWindowType)egl_window, NULL);

    // Set current Display, Draw/Read Surfaces, Context (Doing it before the loop because only single window is enough for this project)
    // Whatever OpenGl does, it'll do on the surface which was marked by the latest eglMakeCurrent Call
    eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    //                          ^ Draw Surface  ^ Read Surface
    
    if (!gladLoadGL((GLADloadfunc)eglGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        exit(1);
    }

    // OpenGL
    shader = create_shader("shaders/text.vert", "shaders/text.frag");
    init_freetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 48);
    float projection[16];
    make_ortho(projection, 0, WIDTH, 0, HEIGHT);
    glUseProgram(shader);
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, projection);

    // Render loop
    while (wl_display_dispatch(display) != -1) {
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        render_text("Hello, World!", 100.0f, 300.0f, 1.0f, 1.0f, 1.0f, 1.0f);
        eglSwapBuffers(egl_display, egl_surface);
    }

    wl_display_disconnect(display);
    return 0;
}
