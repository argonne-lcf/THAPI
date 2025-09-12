#ifndef ITTNOTIFY_H
#define ITTNOTIFY_H

/* Public header for a minimal ITT notify stub. */

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Export / calling convention macros --- */
#ifndef ITTAPI
#  define ITTAPI /* default calling convention */
#endif

#ifndef EXPORT
#  define EXPORT __attribute__((visibility("default")))
#endif

/* ========================= Minimal ABI ========================= */
#pragma pack(push, 8)

typedef enum __itt_group_id {
    __itt_group_none = 0,
    __itt_group_all  = -1
} __itt_group_id;

typedef struct __itt_domain {
    const char *name;    /* Only valid if created by this stub */
    uint64_t    id;
    uint32_t    flags;
    struct __itt_domain *next;
} __itt_domain;

typedef struct __itt_string_handle {
    const char *str;     /* Only valid if created by this stub */
    uint64_t    id;
    struct __itt_string_handle *next;
} __itt_string_handle;

typedef struct __itt_id { uint64_t d1, d2, d3; } __itt_id;

typedef enum __itt_scope {
    __itt_scope_unknown = 0,
    __itt_scope_global  = 1,
    __itt_scope_track   = 2,
    __itt_scope_task    = 3,
} __itt_scope;

typedef struct __itt_event { uint32_t id; } __itt_event;

typedef enum __itt_metadata_type {
    __itt_metadata_u64 = 1, __itt_metadata_s64 = 2, __itt_metadata_f64 = 3,
    __itt_metadata_u32 = 4, __itt_metadata_s32 = 5, __itt_metadata_f32 = 6,
} __itt_metadata_type;

typedef struct __itt_api_info {
    const char *name;
    void      **func_ptr;
    void       *init_func;
    void       *null_func;
    __itt_group_id group;
} __itt_api_info;

/* NOTE: This type is passed only by pointer; definition retained for ABI parity. */
typedef struct __itt_global {
    unsigned char magic[8];
    unsigned long version_major, version_minor, version_build;
    volatile long api_initialized, mutex_initialized, atomic_counter;
    pthread_mutex_t mutex;
    void *lib;
    void *error_handler;
    const char **dll_path_ptr;
    __itt_api_info *api_list_ptr;
    struct __itt_global *next;
    void *thread_list;
    void *domain_list;
    void *string_list;
    int state;
} __itt_global;

#pragma pack(pop)

/* ========================= Public API ========================= */

/* Creation */
EXPORT struct __itt_domain* ITTAPI __itt_domain_create(const char* name);
EXPORT struct __itt_domain* ITTAPI __itt_domain_createA(const char* name);

EXPORT struct __itt_string_handle* ITTAPI __itt_string_handle_create(const char* str);
EXPORT struct __itt_string_handle* ITTAPI __itt_string_handle_createA(const char* str);

/* Tasks & markers */
EXPORT void ITTAPI __itt_task_begin(struct __itt_domain *domain,
                                    struct __itt_id taskid,
                                    struct __itt_id parent,
                                    struct __itt_string_handle *name);
EXPORT void ITTAPI __itt_task_beginA(struct __itt_domain *domain,
                                     struct __itt_id taskid,
                                     struct __itt_id parent,
                                     const char *name);
EXPORT void ITTAPI __itt_task_end(struct __itt_domain *domain);

EXPORT void ITTAPI __itt_marker(struct __itt_domain *domain,
                                struct __itt_id id,
                                struct __itt_string_handle *name,
                                enum __itt_scope scope);
EXPORT void ITTAPI __itt_markerA(struct __itt_domain *domain,
                                 struct __itt_id id,
                                 const char *name,
                                 enum __itt_scope scope);

EXPORT int  ITTAPI __itt_thread_set_name(const char* name);
EXPORT int  ITTAPI __itt_thread_set_nameA(const char* name);

EXPORT void ITTAPI __itt_pause(void);
EXPORT void ITTAPI __itt_resume(void);
EXPORT void ITTAPI __itt_detach(void);

/* Events & metadata */
EXPORT struct __itt_event ITTAPI __itt_event_create(const char* name, int namelen);
EXPORT void ITTAPI __itt_event_start(struct __itt_event ev);
EXPORT void ITTAPI __itt_event_end(struct __itt_event ev);

EXPORT void ITTAPI __itt_metadata_add(struct __itt_domain *domain,
                                      struct __itt_id id,
                                      struct __itt_string_handle *key,
                                      enum __itt_metadata_type type,
                                      size_t count,
                                      const void *data);

/* Handshake */
EXPORT void ITTAPI __itt_api_init(struct __itt_global* g, __itt_group_id group);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ITTNOTIFY_H */

