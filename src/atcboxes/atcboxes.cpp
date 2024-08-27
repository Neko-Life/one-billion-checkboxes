#include "atcboxes/atcboxes.h"
#include "atcboxes/migrate.h"
#include "atcboxes/runtime_cli.h"
#include "atcboxes/server.h"
#include "atcboxes/test.h"
#include "atcboxes/util.h"
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <threads.h>

#define ARGV_LOOP(x)                                                           \
  for (int i = 1; i < argc; i++)                                               \
  x

#define ARGCMP(x) (strcmp(argv[i], x) == 0)
#define ARGVAL argv[i]

namespace atcboxes {

constexpr size_t _s64 = sizeof(CBOX_T);
#ifdef WITH_COLOR
constexpr uint64_t cbsiz = A_TRILLION;
constexpr size_t _r = 1;
#else
constexpr size_t _r = _s64 * CHAR_BIT;
constexpr uint64_t cbsiz = A_TRILLION / _r;
#endif
constexpr uint64_t max_idx = cbsiz - 1;
constexpr uint64_t cbmem_s = _s64 * cbsiz;

const char *statefile = STATE_FILE;
const char *runbin = "./atcboxes";

static void print_spec() {
  fprintf(stderr,
          "uint64_t_size(%zu) char_bit(%d) " A_TRILLION_STR
          "/%lu = cbsiz(%lu), cbmem_s(%zu)\n",
          _s64, CHAR_BIT, _r, cbsiz, cbmem_s);
}

#ifdef USE_MALLOC
static CBOX_T *cboxes = NULL;
#else
// yes, 125MB on the data segment
static CBOX_T cboxes[cbsiz] = {{}};
#endif // USE_MALLOC

uint64_t gv = 0;

std::mutex cb_m;
std::mutex gv_m;

cbox_lock_guard_t::cbox_lock_guard_t() : lk(cb_m) {}

static int load_state(const char *filepath) {
  fprintf(stderr, "[load_state] Loading `%s`\n", filepath);

  std::lock_guard lk(cb_m);
  std::lock_guard lj(gv_m);

  FILE *f = util::try_open(filepath, "rb");
  int status = 0;

  if (!f)
    return -1;

  gv = 0;
  constexpr size_t bufsiz = 1024;
  CBOX_T temp[bufsiz] = {{}};

  size_t total_el = 0;
  ssize_t read = 0;
  while ((read = fread(temp, _s64, bufsiz, f)) > 0) {
    size_t synched = 0;
    for (size_t i = 0; i < read && (i + total_el) < cbsiz; i++) {
      cboxes[i + total_el] = temp[i];

#ifdef WITH_COLOR
      if (temp[i].a & 1)
        gv++;
#else
      uint64_t l = 1;
      while (l) {
        if (temp[i] & l)
          gv++;

        l <<= 1;
      }
#endif // WITH_COLOR

      synched++;
    }

    if (synched != read) {
      fprintf(stderr, "[load_state ERROR] Corrupted state file (synched != "
                      "read), probably code error.\nExiting...\n");

      exit(2);
    }

    total_el += read;
  }

  fprintf(stderr, "[load_state] Read %zu elements from `%s`\n", total_el,
          filepath);

  if (total_el != cbsiz) {
    fprintf(stderr, "[load_state FATAL] Corrupted state file (total_el != "
                    "cbsiz)\n");
    fprintf(stderr, "If this state file ever valid before, try running the "
                    "migrate command:\n");
    fprintf(stderr, "\t%s migrate '%s'\n\n", runbin, filepath);
    fprintf(stderr, "Exiting...\n");

    exit(3);
  }

  fprintf(stderr, "[load_state] Loaded state `%s`\n", filepath);

  fclose(f);
  f = NULL;

  return status;
}

static int save_state(const char *filepath) {
  std::lock_guard lk(cb_m);

  FILE *f = util::try_open(filepath, "wb");
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

static int reset_state() {
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
static int switch_c(uint64_t c, uint64_t bit) {
  if (c > max_idx)
    return -1;

  uint64_t b = 1;
#ifdef WITH_COLOR
#define CMP cboxes[c].a
#else
#define CMP cboxes[c]
  b <<= bit;

  std::lock_guard lk(cb_m);
  std::lock_guard lj(gv_m);
#endif // WITH_COLOR

  bool had = (CMP & b) != 0;

  CMP = had ? CMP & (~b) // remove if had it
            : CMP | b;   // else add

#undef CMP

  int ret = !had ? 1 : 0;

  ret ? gv++ : gv--;

  return ret;
}

static std::pair<uint64_t, uint64_t> get_cb(uint64_t i) {
#ifdef WITH_COLOR
  // is this function even used??
  // lets keep it here for comedy, muhaha
  return {i, 1};
#else
  uint64_t c = i > 0 ? i / 64 : 0;
  uint64_t b = i > 0 ? i % 64 : 0;

  return {c, b};
#endif
}

static void init_main() {
#ifdef USE_MALLOC
  fprintf(stderr, "[init_main] Allocating %zu bytes...\n", cbmem_s);
  cboxes = (CBOX_T *)malloc(cbmem_s);

  if (cboxes == NULL) {
    // dont have enough ram? die
    perror("[init_main FATAL]");
    exit(1);
  }
#endif // USE_MALLOC

  load_state(statefile);
}

static void free_main() {
#ifdef USE_MALLOC
  if (cboxes == NULL) {
    fprintf(stderr, "[free_main ERROR] State freed\n");
    return;
  }

  save_state(statefile);

  free(cboxes);
  cboxes = NULL;
#else
  save_state(statefile);
#endif // USE_MALLOC
}

uint64_t get_gv() {
  std::lock_guard lj(gv_m);

  return gv;
}

#ifdef WITH_COLOR
static int get_cv(uint64_t c, cbox_t &s) {
  if (c > max_idx)
    return -1;

  std::lock_guard lk(cb_m);
  s = cboxes[c];

  return (cboxes[c].a & 1) ? 1 : 0;
}

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @param s color state
 * @return 0 off, 1 on, -1 err
 */
int switch_state(uint64_t i, const CBOX_T &s) {
  if (i > max_idx)
    return -1;

  std::lock_guard lk(cb_m);
  std::lock_guard lj(gv_m);

  uint8_t prev = cboxes[i].a & 1 ? 0xFF : 0xFE;
  cboxes[i] = s;
  cboxes[i].a &= prev;

  return switch_c(i, 1);
}

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @param s color
 * @return 0 off, 1 on, -1 err
 */
int get_state(uint64_t i, cbox_t &s) {
  std::lock_guard lk(cb_m);

  return get_cv(i, s);
}

#else

static int get_cv(uint64_t c, uint64_t bit) {
  if (c > max_idx)
    return -1;

  uint64_t b = 1;
  b <<= bit;

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
#endif // WITH_COLOR

std::pair<CBOX_T const *, size_t> get_state_page(uint64_t page) {
  constexpr const size_t el_per_page = SIZE_PER_PAGE / _r;
  constexpr const size_t max_page = (A_TRILLION / SIZE_PER_PAGE) - 1;

  if (page > max_page)
    return {NULL, 0};

  return {cboxes + (page * el_per_page), el_per_page};
}

////////////////////

static void print_help() {
  fprintf(stderr, "%s Checkboxes - Server\n", A_TRILLION_STR);
#if defined(ATCB_VERSION_MAJOR) && defined(ATCB_VERSION_MINOR) &&              \
    defined(ATCB_VERSION_PATCH)
  fprintf(stderr, "version %d.%d.%d\n", ATCB_VERSION_MAJOR, ATCB_VERSION_MINOR,
          ATCB_VERSION_PATCH);
#endif // ATCB_VERSION_

  fprintf(stderr, "Usage: %s [COMMAND] [OPTION...]\n\n", runbin);
  fprintf(stderr, "Commands:\n");
  fprintf(stderr, "Commands:\n");
}

int run(const int argc, const char *const argv[]) {
  runbin = argv[0];
  print_spec();

  // cli args
  bool testing = false;
  bool migrating = false;
  bool getstatefile = false;
  std::string migratefile = "";

  ARGV_LOOP({
    if (ARGCMP("--help") || ARGCMP("-h")) {
      print_help();
      return 0;
    } else if (ARGCMP("test")) {
      testing = true;
    } else if (ARGCMP("migrate")) {
      migrating = true;
    } else if (ARGCMP("--state") || ARGCMP("-s")) {
      getstatefile = true;
    } else if (migrating) {
      migratefile = ARGVAL;
      migrating = false;
    } else if (getstatefile) {
      statefile = ARGVAL;
      getstatefile = false;
    }
  })

  if (migrating || !migratefile.empty())
    return migrate::run(migratefile);

  ////////////////////

  init_main();

  int status = 0;
  if (testing)
    status = test::run(cboxes);
  else {
    runtime_cli::run();
    status = server::run();
  }

  free_main();
  runtime_cli::shutdown();

  return status;
}

} // namespace atcboxes
