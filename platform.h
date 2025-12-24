#ifndef PLATFORM_H_
#define PLATFORM_H_

void *platform_load_library(const char *path);
void platform_free_library(void *handle);
void *platform_get_symbol(void *handle, const char *symbol);

#endif // PLATFORM_H_
