#include "atcboxes/atcboxes.h"
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <threads.h>

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

FILE *try_open(const char *filepath, const char *mode) {
  FILE *f = fopen(filepath, mode);
  if (!f) {
    perror("[try_open ERROR]");
    return NULL;
  }

  return f;
}

int load_state(const char *filepath) {
  FILE *f = try_open(filepath, "rb");
  int status = 0;

  if (!f)
    return -1;

  constexpr size_t bufsiz = 1024;
  uint64_t temp[bufsiz] = {0};

  size_t total_el = 0;
  size_t read = 0;
  while ((read = fread(temp, _s64, bufsiz, f)) > 0) {
    size_t synched = 0;
    for (size_t i = 0; i < read && (i + total_el) < cbsiz; i++) {
      cboxes[i + total_el] = temp[i];

      synched++;
    }

    if (synched != read) {
      fprintf(stderr, "[load_state ERROR] Corrupted state file (synched != "
                      "read), exiting\n");

      exit(1);
    }

    total_el += read;
  }

  fprintf(stderr, "[load_state] Read %zu elements from `%s`\n", total_el,
          filepath);

  if (total_el != cbsiz) {
    fprintf(stderr, "[load_state ERROR] Corrupted state file (total_el != "
                    "cbsiz), exiting\n");

    exit(2);
  }

  fprintf(stderr, "[load_state] Loaded state `%s`\n", filepath);

  fclose(f);
  f = NULL;

  return status;
}

int save_state(const char *filepath) {
  FILE *f = try_open(filepath, "wb");
  if (!f)
    return -1;

  size_t wrote = fwrite(cboxes, _s64, cbsiz, f);
  fprintf(stderr, "[save_state] Wrote %zu elements to `%s`\n", wrote, filepath);

  fclose(f);
  f = NULL;

  return 0;
}

int reset_state() {
  constexpr size_t _s = _s64 * cbsiz;
  memset(cboxes, 0, _s);

  fprintf(stderr, "[reset_state] State resetted\n");

  return 0;
}

int run(const int argc, const char *const argv[]) {
  printf("Hello World!\n");

  // size_t li = cbsiz - 1;
  // cboxes[li] = 28765284;
  // printf("%zu\n", cboxes[li]);
  // save_state("state1");
  // reset_state();
  // printf("%zu\n", cboxes[li]);
  // load_state("state1");
  // printf("%zu\n", cboxes[li]);

  // struct timespec t = {1, 0};
  // thrd_sleep(&t, NULL);

  return 0;
}

} // namespace atcboxes
