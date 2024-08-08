#include "atcboxes/atcboxes.h"
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <threads.h>

namespace atcboxes {

constexpr size_t _s64 = sizeof(uint64_t);
constexpr size_t _r = _s64 * CHAR_BIT;

constexpr size_t cbsiz = 1'000'000'000 / _r;
constexpr size_t cbmem_s = _s64 * cbsiz;

void print_spec() {
  fprintf(
      stderr,
      "uint64_t_size(%zu) char_bit(%d) 1'000'000'000'000/%lu = cbsiz(%lu)\n",
      _s64, CHAR_BIT, _r, cbsiz);
}

static uint64_t cboxes[cbsiz] = {0};
std::mutex cb_m;

FILE *try_open(const char *filepath, const char *mode) {
  FILE *f = fopen(filepath, mode);
  if (!f) {
    perror("[try_open ERROR]");
    return NULL;
  }

  return f;
}

int load_state(const char *filepath) {
  std::lock_guard lk(cb_m);

  FILE *f = try_open(filepath, "rb");
  int status = 0;

  if (!f)
    return -1;

  constexpr size_t bufsiz = 1024;
  uint64_t temp[bufsiz] = {0};

  size_t total_el = 0;
  ssize_t read = 0;
  while ((read = fread(temp, _s64, bufsiz, f)) > 0) {
    size_t synched = 0;
    for (size_t i = 0; i < read && (i + total_el) < cbsiz; i++) {
      cboxes[i + total_el] = temp[i];

      synched++;
    }

    if (synched != read) {
      fprintf(stderr, "[load_state ERROR] Corrupted state file (synched != "
                      "read), exiting\n");

      exit(2);
    }

    total_el += read;
  }

  fprintf(stderr, "[load_state] Read %zu elements from `%s`\n", total_el,
          filepath);

  if (total_el != cbsiz) {
    fprintf(stderr, "[load_state ERROR] Corrupted state file (total_el != "
                    "cbsiz), exiting\n");

    exit(3);
  }

  fprintf(stderr, "[load_state] Loaded state `%s`\n", filepath);

  fclose(f);
  f = NULL;

  return status;
}

int save_state(const char *filepath) {
  std::lock_guard lk(cb_m);

  FILE *f = try_open(filepath, "wb");
  int status = 0;

  if (!f)
    return -1;

  ssize_t wrote = fwrite(cboxes, _s64, cbsiz, f);
  fprintf(stderr, "[save_state] Wrote %zu elements to `%s`\n", wrote, filepath);

  if (wrote != cbsiz) {
    fprintf(stderr, "[save_state ERROR] Failed saving state to `%s`\n",
            filepath);

    status = 1;
  }

  fclose(f);
  f = NULL;

  return status;
}

int reset_state() {
  std::lock_guard lk(cb_m);

  memset(cboxes, 0, cbmem_s);

  fprintf(stderr, "[reset_state] State resetted\n");

  return 0;
}

/**
 * @param c column (index)
 * @param bit zero based (0-63)
 */
int switch_c(size_t c, short bit) {
  constexpr size_t max_idx = cbsiz - 1;
  if (c > max_idx)
    return 1;

  uint64_t b = 1 << bit;

  std::lock_guard lk(cb_m);

  cboxes[c] = cboxes[c] & b ? cboxes[c] & (~b) // remove if had it
                            : cboxes[c] | b;   // else add

  return 0;
}

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 */
int switch_state(uint64_t i) {
  size_t c = i > 0 ? i / 64 : 0;
  short b = i > 0 ? i % 64 : 0;

  return switch_c(c, b);
}

int run(const int argc, const char *const argv[]) {
  printf("Hello World!\n");
  print_spec();

#define CHECK
#ifdef CHECK
  size_t li = cbsiz - 1;
  cboxes[li] = 28765284;
  printf("%zu\n", cboxes[li]);
  save_state("state1");
  reset_state();
  printf("%zu\n", cboxes[li]);
  load_state("state1");
  printf("%zu\n", cboxes[li]);

  struct timespec t = {1, 0};
  thrd_sleep(&t, NULL);
#endif // CHECK

  // constexpr int bufsiz = 4096;
  // char buf[bufsiz] = {0};
  // while (fgets(buf, bufsiz, stdin) != NULL) {
  //   fprintf(stderr, "Read: `%s`\n", buf);
  //   memset(buf, 0, sizeof(char) * bufsiz);
  // }

  return 0;
}

} // namespace atcboxes
