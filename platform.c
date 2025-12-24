#include <stdio.h>
#include "platform.h"

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

void *platform_load_library(const char *path) {
    #ifdef _WIN32
        HMODULE handle = LoadLibrary(path);
        if (handle == NULL) {
            fprintf(stderr, "ERROR: could not load %s\n", path);
            return NULL;
        }
        return (void*)handle;
    #else
        void *handle = dlopen(path, RTLD_NOW);
        if (handle == NULL) {
            fprintf(stderr, "ERROR: could not load %s: %s\n", path, dlerror());
            return NULL;
        }
        return handle;
    #endif
}

void platform_free_library(void *handle) {
    if (handle == NULL) return;
    #ifdef _WIN32
        FreeLibrary((HMODULE)handle);
    #else
        dlclose(handle);
    #endif
}

void *platform_get_symbol(void *handle, const char *symbol) {
    if (handle == NULL) return NULL;
    
    #ifdef _WIN32
        void *sym = (void*)GetProcAddress((HMODULE)handle, symbol);
        if (sym == NULL) {
             fprintf(stderr, "ERROR: could not find symbol %s\n", symbol);
             return NULL;
        }
        return sym;
    #else
        void *sym = dlsym(handle, symbol);
        if (sym == NULL) {
            fprintf(stderr, "ERROR: could not find symbol %s: %s\n", symbol, dlerror());
            return NULL;
        }
        return sym;
    #endif
}
