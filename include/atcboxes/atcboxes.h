#ifndef ATCBOXES_H
#define ATCBOXES_H

#include <cstdint>
#include <mutex>

#define STATE_FILE "state.atcb"

#ifdef WITH_COLOR
#define SIZE_PER_PAGE 250'000
#define CBOX_T atcboxes::cbox_t
#else
#define SIZE_PER_PAGE 1'000'000
#define CBOX_T uint64_t
#endif // WITH_COLOR

#ifdef ACTUALLY_A_TRILLION

#define A_TRILLION 1'000'000'000'000
#define A_TRILLION_STR "1'000'000'000'000"
// this will always be too huge (125GB without color)
// and color support requires 4000GB
#define USE_MALLOC

#ifdef WITH_COLOR
#define BUILD_TRILLION_WITH_COLOR
#else
#define BUILD_TRILLION_NO_COLOR
#endif // WITH_COLOR

#else

#define A_TRILLION 1'000'000'000
#define A_TRILLION_STR "1'000'000'000"

#ifdef WITH_COLOR
// we cant put 4GB in the data segment!
#define USE_MALLOC
#define BUILD_BILLION_WITH_COLOR
#else
#define BUILD_BILLION_NO_COLOR
#endif // WITH_COLOR

#endif // ACTUALLY_A_TRILLION

namespace atcboxes {

struct cbox_t {
  uint8_t r;
  uint8_t g;
  uint8_t b;
  // bit 0 is for active state
  uint8_t a;
};

struct cbox_lock_guard_t {
  std::lock_guard<std::mutex> lk;

  cbox_lock_guard_t();
  ~cbox_lock_guard_t() = default;
};

uint64_t get_gv();

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @param s state out
 * @return 0 off, 1 on, -1 err
 */
int switch_state(uint64_t i
#ifdef WITH_COLOR
                 ,
                 const CBOX_T &s
#endif // WITH_COLOR
);

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @param s state out
 * @return 0 off, 1 on, -1 err
 */
int get_state(uint64_t i
#ifdef WITH_COLOR
              ,
              CBOX_T &s
#endif // WITH_COLOR
);

/**
 * @brief Caller should lock cbox mutex by constructing cbox_lock_guard_t before
 *        calling this function and keeping it alive as long as the return value
 *        is gonna be used.
 */
std::pair<CBOX_T const *, size_t> get_state_page(uint64_t page);

int run(const int argc, const char *const argv[]);

} // namespace atcboxes

#endif // ATCBOXES_H
