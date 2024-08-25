#ifndef ATCBOXES_H
#define ATCBOXES_H

#include <cstdint>
#include <mutex>

#define SIZE_PER_PAGE 1'000'000

#define STATE_FILE "state.atcb"

#ifdef ACTUALLY_A_TRILLION

#define A_TRILLION 1'000'000'000'000
#define A_TRILLION_STR "1'000'000'000'000"

#else

#define A_TRILLION 1'000'000'000
#define A_TRILLION_STR "1'000'000'000"

#endif // ACTUALLY_A_TRILLION

namespace atcboxes {

// unused for now, i havent even start the frontend
struct cbox_t {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct cbox_lock_guard_t {
  std::lock_guard<std::mutex> lk;

  cbox_lock_guard_t();
  ~cbox_lock_guard_t() = default;
};

uint64_t get_gv();

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @return 0 off, 1 on, -1 err
 */
int switch_state(uint64_t i);

/**
 * @param i zero based global bit idx (0-(1'000'000'000'000-1))
 * @return 0 off, 1 on, -1 err
 */
int get_state(uint64_t i);

/**
 * @brief Caller should lock cbox mutex by constructing cbox_lock_guard_t before
 *        calling this function and keeping it alive as long as the return value
 *        is gonna be used.
 */
std::pair<uint64_t const *, size_t> get_state_page(uint64_t page);

int run(const int argc, const char *const argv[]);

} // namespace atcboxes

#endif // ATCBOXES_H
