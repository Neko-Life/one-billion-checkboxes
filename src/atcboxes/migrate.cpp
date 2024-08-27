#include "atcboxes/migrate.h"
#include "atcboxes/util.h"
#include <sys/stat.h>

namespace atcboxes::migrate {

int run(const std::string &file) {
  if (file.empty()) {
    fprintf(stderr, "No state file specified to migrate!\nExiting...");
    return -1;
  }

  // detect whether the state file has the size of 5 possible combination
  // 1. billion no color
  // 2. billion with color
  // 3. trillion no color (not implemented yet)
  // 4. trillion with color (not implemented yet)
  // 5. broken state, should reset everything

  FILE *f = util::try_open(file.c_str(), "rb");
  if (f == NULL)
    return -1;

  int status = -1;

  struct stat s;
  if (fstat(fileno(f), &s) != 0) {
    perror("[migrate::run ERROR] fstat");
    goto ferr;
  }

  return 0;

ferr:
  fclose(f);
  return status;
}
} // namespace atcboxes::migrate
