#include "atcboxes/migrate.h"

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

  /*struct*/

  return 0;
}
} // namespace atcboxes::migrate
