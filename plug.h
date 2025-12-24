#ifndef PLUG_H_
#define PLUG_H_

#ifdef _WIN32
    #define PLUG_EXPORT __declspec(dllexport) 
#else 
    #define PLUG_EXPORT
#endif

typedef void (*plug_init_t)(void);
typedef void *(*plug_pre_reload_t)(void);
typedef void (*plug_post_reload_t)(void*);
typedef void (*plug_update_t)(void);
typedef bool (*plug_finished_t)(void);

#ifdef PLUG_IMPL
    PLUG_EXPORT void plug_init(void);
    PLUG_EXPORT void *plug_pre_reload(void);
    PLUG_EXPORT void plug_post_reload(void *state);
    PLUG_EXPORT void plug_update(void);
    PLUG_EXPORT bool plug_finished(void);
#endif


#endif // !PLUG_H_
