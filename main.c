#include<stdio.h>

#include <raylib.h>
#include "plug.h"

#include <dlfcn.h>

int main(void) {

    const char *libplug_path = "./libplug.dylib";
    void *libplug = dlopen(libplug_path, RTLD_NOW);

    if(libplug == NULL) {
        fprintf(stderr, "Error: could not load %s: %s\n", libplug_path, dlerror());
        return 1;
    }
    
    InitWindow(800, 600, "CONA");

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RED);
        EndDrawing();
    }

    CloseWindow();

    dlclose(libplug);

    return  0;
}
