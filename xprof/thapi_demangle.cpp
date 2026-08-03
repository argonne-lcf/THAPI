// Tiny filter around my_demangle(): reads one mangled name per line from
// stdin and writes its demangled form (one per line) to stdout. Installed so
// it can be scripted (e.g. by integration tests) via the same demangler THAPI
// uses internally. A name that cannot be demangled is echoed back unchanged.
//
// With --features it instead prints the demangling backends compiled in (one
// per line), so callers can tell what this build supports.
#include "config.h"
#include "my_demangle.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  if (argc > 1 && strcmp(argv[1], "--features") == 0) {
#if defined(HAVE_CXXABI_H)
    std::cout << "cxxabi\n";
#endif
#if defined(HAVE_LLVM_DEMANGLE)
    std::cout << "llvm-demangle\n";
#endif
    return 0;
  }

  std::string line;
  while (std::getline(std::cin, line)) {
    char *d = my_demangle(line.c_str());
    std::cout << (d ? d : line.c_str()) << "\n";
    free(d);
  }
  return 0;
}
