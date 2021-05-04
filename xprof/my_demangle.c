#include "my_demangle.h"
#include <demangle.h>

char * my_demangle(const char * name) {
    return cplus_demangle(name, DMGL_AUTO);
}
