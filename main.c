#include <stdio.h>
#include <raylib.h>
#include "plug.h"
#include "platform.h"

#ifdef _WIN32
    const char *libplug_file_name = "plug.dll";
#elif defined(__APPLE__)
    const char *libplug_file_name = "libplug.dylib";
#else
    const char *libplug_file_name = "libplug.so";
#endif

void *libplug = NULL;

plug_init_t plug_init = NULL;
plug_pre_reload_t plug_pre_reload = NULL;
plug_post_reload_t plug_post_reload = NULL;
plug_update_t plug_update = NULL;

bool load_plug(void) {
    if (libplug != NULL) {
        platform_free_library(libplug);
        libplug = NULL;
    }

    libplug = platform_load_library(libplug_file_name);
    if (libplug == NULL) {
        return false;
    }

    #define LOAD_SYM(name) \
        name = (name##_t) platform_get_symbol(libplug, #name); \
        if (name == NULL) { \
            return false; \
        }

    LOAD_SYM(plug_init);
    LOAD_SYM(plug_pre_reload);
    LOAD_SYM(plug_post_reload);
    LOAD_SYM(plug_update);

    return true;
}

int main(void) {
    if (!load_plug()) return 1;

    InitWindow(800, 600, "CONA");
    plug_init();

    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        // if (IsKeyPressed(KEY_R)) {
        //     void *state = plug_pre_reload();
        //     if (!load_plug()) return 1;
        //     plug_post_reload(state);
        // }
        //
        plug_update();
    }

    CloseWindow();
    
    return 0;
}
