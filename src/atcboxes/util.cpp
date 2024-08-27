#include "atcboxes/util.h"

namespace atcboxes::util {

FILE *try_open(const char *filepath, const char *mode) {
  FILE *f = fopen(filepath, mode);
  if (!f) {
    perror("[util::try_open ERROR]");
    return NULL;
  }

  return f;
}

} // namespace atcboxes::util
