#include "atcboxes/atcboxes.h"
#include <climits>
#include <cstdint>
#include <cstdio>

namespace atcboxes {

constexpr size_t _s64 = sizeof(uint64_t);
constexpr size_t _r = _s64 * CHAR_BIT;

constexpr size_t cbsiz = 1'000'000'000 / _r;

void print_spec() {
  fprintf(
      stderr,
      "uint64_t_size(%zu) char_bit(%d) 1'000'000'000'000/%lu = cbsiz(%lu)\n",
      _s64, CHAR_BIT, _r, cbsiz);
}

static uint64_t cboxes[cbsiz] = {0};

int load_state(const char *filepath) { return 0; }

int save_state(const char *filepath) { return 0; }

int reset_state() {
  for (size_t i = 0; i < cbsiz; i++) {
    cboxes[i] = 0;
  }

  return 0;
}

int run(const int argc, const char *const argv[]) {
  printf("Hello World!\n");

  return 0;
}

} // namespace atcboxes
