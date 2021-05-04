#include "my_demangle.h"
#include "config.h"
#if defined(HAVE_DEMANGLE_H)
#include <demangle.h>
#elif defined(HAVE_LIBIBERTY_DEMANGLE_H)
#include <libiberty/demangle.h>
#endif

char * my_demangle(const char * name) {
    return cplus_demangle(name, DMGL_AUTO);
}
