#define NOB_IMPLEMENTATION
#include "nob.h"

void cc(Nob_Cmd *cmd) {
    nob_cmd_append(cmd, "cc");
    nob_cmd_append(cmd, "-Wall", "-Wextra", "-g");

    // raylib headers
    nob_cmd_append(cmd, "-I./raylib/raylib-5.5_macos/include");
}

void frameworks(Nob_Cmd *cmd) {
    nob_cmd_append(cmd,
        "-framework", "OpenGL",
        "-framework", "Cocoa",
        "-framework", "IOKit",
        "-framework", "CoreVideo"
    );
}

void libs(Nob_Cmd *cmd) {
    nob_cmd_append(cmd, "-L./raylib/raylib-5.5_macos/lib");

    // runtime search path (portable)
    nob_cmd_append(cmd, "-Wl,-rpath,@loader_path/raylib/raylib-5.5_macos/lib");
    nob_cmd_append(cmd, "-Wl,-rpath,@loader_path/");

    nob_cmd_append(cmd, "-lraylib");
}

bool build_plug(Nob_Cmd *cmd) {
    cmd->count = 0;

    cc(cmd);
    nob_cmd_append(cmd,
        "-dynamiclib",
        "-o", "libplug.dylib",
        "plug.c"
    );

    libs(cmd);
    frameworks(cmd);

    return nob_cmd_run_sync(*cmd);
}

bool build_main(Nob_Cmd *cmd) {
    cmd->count = 0;

    cc(cmd);
    nob_cmd_append(cmd, "-o", "main", "main.c");

    libs(cmd);
    frameworks(cmd);

    return nob_cmd_run_sync(*cmd);
}

int main(int argc, char **argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);

    Nob_Cmd cmd = {0};

    if (!build_plug(&cmd))
        return 1;

    if (!build_main(&cmd))
        return 1;

    return 0;
}

