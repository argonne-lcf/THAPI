#include "my_demangle.h"
#include "config.h"
#include <cstdlib>
#include <cstring>

// Demangling backends, in order of preference:
//
//   1. abi::__cxa_demangle (<cxxabi.h>) -- always available from the C++
//      runtime (libstdc++/libc++), no extra dependency. Handles the vast
//      majority of symbols.
//   2. llvm::demangle (llvm-demangle) -- optional, detected at configure time
//      (HAVE_LLVM_DEMANGLE). Used as a fallback because it demangles some
//      Itanium symbols that __cxa_demangle rejects (e.g. nested lambdas
//      referencing an enclosing template parameter under a "typeinfo name for"
//      / _ZTS context, as emitted by AMReX/SYCL kernels).
//   3. Passthrough -- if nothing demangles it, return a copy of the input.
//
// Contract: always returns a malloc()'d C string the caller must free() (never
// NULL); if nothing can demangle the name, that string is the unmangled input.
// __cxa_demangle already returns a malloc()'d buffer; the other branches use
// strdup() so ownership is uniform. A NULL input is treated as "".

#if defined(HAVE_CXXABI_H)
#include <cxxabi.h>
#endif

#if defined(HAVE_LLVM_DEMANGLE)
#include <llvm/Demangle/Demangle.h>
#endif

extern "C" char *my_demangle(const char *name) {
  if (!name)
    name = "";

#if defined(HAVE_CXXABI_H)
  // 1. C++ runtime demangler. On success it returns a malloc()'d buffer; on
  // failure it returns NULL without allocating, so there is nothing to free.
  int status = 0;
  char *out = abi::__cxa_demangle(name, NULL, NULL, &status);
  if (status == 0 && out)
    return out;
#endif

#if defined(HAVE_LLVM_DEMANGLE)
  // 2. LLVM fallback. llvm::demangle() returns the input unchanged when it
  // cannot demangle, so treat "unchanged" as failure.
  {
    std::string s = llvm::demangle(name);
    if (s != name)
      return strdup(s.c_str());
  }
#endif

  // 3. Passthrough.
  return strdup(name);
}
