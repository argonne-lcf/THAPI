#if !defined(THAPI)
#define THAPI

void thapi_start(void);
void thapi_stop(void) __attribute__((constructor));

#endif // THAPI
