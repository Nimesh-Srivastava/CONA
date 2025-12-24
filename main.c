#include<stdio.h>

#include <raylib.h>
#include "plug.h"

#include <dlfnc.h>

int main(void) {

    void *dlopen("lib", int flags);
    
    InitWindow(800, 600, "CONA");

    while(!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RED);
        EndDrawing();
    }

    CloseWindow();

    return  0;
}
