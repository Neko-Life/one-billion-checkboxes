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

#ifdef WITH_COLOR
static void parse_cbox_wc(const std::string &msg, uint64_t &i, cbox_t &s) {

}

static void cbox_t_to_str(uint64_t &i, cbox_t &s, std::string &str) {

}
#endif // WITH_COLOR


} // namespace atcboxes::util
