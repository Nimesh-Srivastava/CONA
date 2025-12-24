#define NOB_IMPLEMENTATION
#include "nob.h"

#ifdef _WIN32
    #define LIB_EXT ".dll"
#elif defined(__APPLE__)
    #define LIB_EXT ".dylib"
#else 
    #define LIB_EXT ".so"
#endif

void cc(Nob_Cmd *cmd) {
    nob_cmd_append(cmd, "cc");
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-g");
    #ifndef _WIN32
         nob_cmd_append(cmd, "-std=c11"); // MSVC might not like this if using cl, but using cc/gcc implies MinGW/CodeBlocks on Windows usually or clang-cl? 
         // Assuming 'cc' on Windows is MinGW/gcc or clang. If MSVC `cl`, flags are different.
         // For now, let's assume MinGW for Windows or clang.
    #endif
    
    // Raylib Include Paths
    #ifdef __APPLE__
        nob_cmd_append(cmd, "-I./raylib/raylib-5.5_macos/include");
    #elif defined(_WIN32)
        nob_cmd_append(cmd, "-I./raylib/raylib-5.5_win64_mingw/include");
    #else 
        // Linux
        nob_cmd_append(cmd, "-I./raylib/raylib-5.5_linux_amd64/include");
    #endif
}

void libs(Nob_Cmd *cmd) {
    #ifdef __APPLE__
        nob_cmd_append(cmd, "-L./raylib/raylib-5.5_macos/lib");
        nob_cmd_append(cmd, "-Wl,-rpath,@loader_path/raylib/raylib-5.5_macos/lib");
        nob_cmd_append(cmd, "-Wl,-rpath,@loader_path/");
        nob_cmd_append(cmd, "-framework", "OpenGL",
                            "-framework", "Cocoa",
                            "-framework", "IOKit",
                            "-framework", "CoreVideo");
    #elif defined(_WIN32)
        nob_cmd_append(cmd, "-L./raylib/raylib-5.5_win64_mingw/lib");
        nob_cmd_append(cmd, "-lraylib", "-lgdi32", "-lwinmm");
        // Windows DLLs need to be next to exe usually.
    #else 
        // Linux
        nob_cmd_append(cmd, "-L./raylib/raylib-5.5_linux_amd64/lib");
        nob_cmd_append(cmd, "-Wl,-rpath,$ORIGIN/raylib/raylib-5.5_linux_amd64/lib");
        nob_cmd_append(cmd, "-Wl,-rpath,$ORIGIN");
        nob_cmd_append(cmd, "-lraylib", "-lm");
    #endif

    #ifndef _WIN32 
        nob_cmd_append(cmd, "-lraylib");
    #endif
}

bool build_plug(Nob_Cmd *cmd) {
    cmd->count = 0;
    cc(cmd);
    #ifdef _WIN32
        nob_cmd_append(cmd, "-shared", "-o", "plug.dll", "plug.c");
        nob_cmd_append(cmd, "-lraylib", "-lgdi32", "-lwinmm"); 
        nob_cmd_append(cmd, "-L./raylib/raylib-5.5_win64_mingw/lib");
    #elif defined(__APPLE__)
        nob_cmd_append(cmd, "-dynamiclib", "-o", "libplug.dylib", "plug.c");
        libs(cmd);
    #else
        nob_cmd_append(cmd, "-shared", "-fPIC", "-o", "libplug.so", "plug.c");
        libs(cmd);
    #endif
    return nob_cmd_run_sync(*cmd);
}

bool build_main(Nob_Cmd *cmd) {
    cmd->count = 0;
    cc(cmd);
    nob_cmd_append(cmd, "-o", "main", "main.c", "platform.c");
    libs(cmd);
    return nob_cmd_run_sync(*cmd);
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_Cmd cmd = {0};

    if (!build_plug(&cmd)) return 1;
    if (!build_main(&cmd)) return 1;

    return 0;
}
