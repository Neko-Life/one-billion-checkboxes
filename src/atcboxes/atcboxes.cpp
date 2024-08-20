#include "atcboxes/atcboxes.h"
#include "atcboxes/server.h"
#include <cassert>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <threads.h>

#define STATE_FILE "state.atcb"

#define ARGV_LOOP(x)                                                           \
  for (int i = 1; i < argc; i++)                                               \
  x

#define ARGCMP(x) strcmp(argv[i], x) == 0

#ifdef ACTUALLY_A_TRILLION

#define A_TRILLION 1'000'000'000'000
#define A_TRILLION_STR "1'000'000'000'000"

#else

#define A_TRILLION 1'000'000'000
#define A_TRILLION_STR "1'000'000'000"

#endif // ACTUALLY_A_TRILLION

namespace atcboxes {

constexpr size_t _s64 = sizeof(uint64_t);
constexpr size_t _r = _s64 * CHAR_BIT;

constexpr size_t cbsiz = A_TRILLION / _r;
constexpr size_t max_idx = cbsiz - 1;
constexpr size_t cbmem_s = _s64 * cbsiz;

void print_spec() {
  fprintf(stderr,
          "uint64_t_size(%zu) char_bit(%d) " A_TRILLION_STR
          "/%lu = cbsiz(%lu), cbmem_s(%zu)\n",
          _s64, CHAR_BIT, _r, cbsiz, cbmem_s);
}

#ifdef ACTUALLY_A_TRILLION
static uint64_t *cboxes = NULL;
#else
// yes, 125MB on the data segment
static uint64_t cboxes[cbsiz] = {0};
#endif // ACTUALLY_A_TRILLION

uint64_t gv = 0;

std::mutex cb_m;
std::mutex gv_m;

cbox_lock_guard_t::cbox_lock_guard_t() : lk(cb_m) {}

FILE *try_open(const char *filepath, const char *mode) {
  FILE *f = fopen(filepath, mode);
  if (!f) {
    perror("[try_open ERROR]");
    return NULL;
  }

  return f;
}

uint64_t get_gv() {
  std::lock_guard lj(gv_m);

  return gv;
}

int load_state(const char *filepath) {
  fprintf(stderr, "[load_state] Loading `%s`\n", filepath);

  std::lock_guard lk(cb_m);
  std::lock_guard lj(gv_m);

  FILE *f = try_open(filepath, "rb");
  int status = 0;

  if (!f)
    return -1;

  gv = 0;
  constexpr size_t bufsiz = 1024;
  uint64_t temp[bufsiz] = {0};

  size_t total_el = 0;
  ssize_t read = 0;
  while ((read = fread(temp, _s64, bufsiz, f)) > 0) {
    size_t synched = 0;
    for (size_t i = 0; i < read && (i + total_el) < cbsiz; i++) {
      cboxes[i + total_el] = temp[i];

      uint64_t l = 1;
      while (l) {
        if (temp[i] & l)
          gv++;

        l <<= 1;
      }

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
  std::lock_guard lj(gv_m);

  memset(cboxes, 0, cbmem_s);
  gv = 0;

  fprintf(stderr, "[reset_state] State resetted\n");

  return 0;
}

/**
 * @param c column (index)
 * @param bit zero based (0-63)
 * @return 0 off, 1 on, -1 err
 */
int switch_c(size_t c, short bit) {
  if (c > max_idx)
    return -1;

  uint64_t b = 1 << bit;

  std::lock_guard lk(cb_m);
  std::lock_guard lj(gv_m);

  cboxes[c] = (cboxes[c] & b) ? cboxes[c] & (~b) // remove if had it
                              : cboxes[c] | b;   // else add

  int ret = (cboxes[c] & b) ? 1 : 0;

  ret ? gv++ : gv--;

  return ret;
}

std::pair<size_t, short> get_cb(uint64_t i) {
  size_t c = i > 0 ? i / 64 : 0;
  short b = i > 0 ? i % 64 : 0;

  return {c, b};
}

int get_cv(size_t c, short bit) {
  if (c > max_idx)
    return -1;

  uint64_t b = 1 << bit;

  std::lock_guard lk(cb_m);

  return (cboxes[c] & b) ? 1 : 0;
}

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @return 0 off, 1 on, -1 err
 */
int switch_state(uint64_t i) {
  auto cb = get_cb(i);

  return switch_c(cb.first, cb.second);
}

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @return 0 off, 1 on, -1 err
 */
int get_state(uint64_t i) {
  auto cb = get_cb(i);

  return get_cv(cb.first, cb.second);
}

std::pair<uint64_t const *, size_t> get_state_page(uint64_t page) {
  constexpr const size_t el_per_page = SIZE_PER_PAGE / _r;
  constexpr const size_t max_page = (A_TRILLION / SIZE_PER_PAGE) - 1;

  if (page > max_page)
    return {NULL, 0};

  return {cboxes + (page * el_per_page), el_per_page};
}

void init_main() {
#ifdef ACTUALLY_A_TRILLION
  fprintf(stderr, "[init_main] Allocating %zu bytes...\n", cbmem_s);
  cboxes = (uint64_t *)malloc(cbmem_s);

  if (!cboxes) {
    // dont have enough ram? die
    perror("[init_main FATAL]");
    exit(1);
  }
#endif // ACTUALLY_A_TRILLION

  load_state(STATE_FILE);
}

void free_main() {
#ifdef ACTUALLY_A_TRILLION
  if (!cboxes) {
    fprintf(stderr, "[free_main ERROR] State freed\n");
    return;
  }

  free(cboxes);
  cboxes = NULL;
#endif // ACTUALLY_A_TRILLION
  save_state(STATE_FILE);
}

int test();

int run(const int argc, const char *const argv[]) {
  printf("Hello World!\n");
  print_spec();

  bool testing = false;
  ARGV_LOOP({
    if (ARGCMP("test")) {
      testing = true;
    }
    /*
       if (ARGCMP("")) {
       } */
  })

  init_main();

  int status = 0;
  if (testing)
    status = test();
  else {
    status = server::run();
  }

  free_main();

  return status;
}

int test() {
  size_t li = cbsiz - 1;
  cboxes[li] = 28765284;
  assert(cboxes[li] ==
         0b0000000000000000000000000000000000000001101101101110110001100100);

  printf("%zu\n", cboxes[li]);
  save_state("state1");
  reset_state();
  printf("%zu\n", cboxes[li]);

  load_state("state1");
  printf("%zu\n", cboxes[li]);
  assert(cboxes[li] ==
         0b0000000000000000000000000000000000000001101101101110110001100100);
  reset_state();

  auto start =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  switch_state(999999960);
  switch_state(999999959);
  switch_state(999999957);
  switch_state(999999956);
  switch_state(999999954);
  switch_state(999999953);
  switch_state(999999951);
  switch_state(999999950);
  switch_state(999999949);
  switch_state(999999947);
  switch_state(999999946);
  printf("%zu\n", gv);
  assert(gv == 11);
  switch_state(999999942);
  switch_state(999999941);
  switch_state(999999938);
  printf("%zu\n", gv);
  assert(gv == 14);
  assert(cboxes[li] ==
         0b0000000000000000000000000000000000000001101101101110110001100100);
  switch_state(999999938);
  printf("%zu\n", gv);
  assert(gv == 13);
  assert(cboxes[li] ==
         0b0000000000000000000000000000000000000001101101101110110001100000);
  switch_state(999999938);
  printf("%zu\n", gv);
  assert(gv == 14);
  auto end =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  printf("took %lld picosecond\n", end - start);

  assert(cboxes[li] ==
         0b0000000000000000000000000000000000000001101101101110110001100100);

  struct timespec t = {1, 0};
  thrd_sleep(&t, NULL);

  return 0;
}

} // namespace atcboxes
