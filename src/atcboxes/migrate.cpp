#include "atcboxes/migrate.h"
#include "atcboxes/atcboxes.h"
#include "atcboxes/util.h"
#include <cstdint>
#include <sys/stat.h>

namespace atcboxes::migrate {

static void no_migration_needed() {
  fprintf(stderr, "State file is valid and compatible, no migration needed.\n");
}

static void print_state_file_type(const char *file, int type) {
  const char *t = "unknown";
  switch (type) {
  case 0:
    t = "BILLION_NO_COLOR";
    break;
  case 1:
    t = "BILLION_WITH_COLOR";
    break;
  case 2:
    t = "TRILLION_NO_COLOR";
    break;
  case 3:
    t = "TRILLION_WITH_COLOR";
    break;
  }

  fprintf(stderr, "State file: %s\n", file);
  fprintf(stderr, "Format: %s\n", t);
}

static void print_migrate_target() {
#ifdef BUILD_BILLION_NO_COLOR
#define MIGRATE_TARGET "BILLION_NO_COLOR"
#endif // BUILD_BILLION_NO_COLOR

#ifdef BUILD_BILLION_WITH_COLOR
#define MIGRATE_TARGET "BILLION_WITH_COLOR"
#endif // BUILD_BILLION_WITH_COLOR

#ifdef BUILD_TRILLION_NO_COLOR
#define MIGRATE_TARGET "TRILLION_NO_COLOR"
#endif // BUILD_TRILLION_NO_COLOR

#ifdef BUILD_TRILLION_WITH_COLOR
#define MIGRATE_TARGET "TRILLION_WITH_COLOR"
#endif // BUILD_TRILLION_WITH_COLOR

  fprintf(stderr, "Migrating to " MIGRATE_TARGET "\n");
#undef MIGRATE_TARGET
}

int run(const std::string &file) {
  if (file.empty()) {
    fprintf(stderr, "No state file specified to migrate!\nExiting...");
    return -1;
  }

  // detect whether the state file has the size of 5 possible combination
  // 1. billion no color
  // 2. billion with color
  // 3. trillion no color
  // 4. trillion with color
  // 5. broken state, should reset everything

  FILE *f = util::try_open(file.c_str(), "rb");
  if (f == NULL)
    return -1;

  int status = -1;
  // state file type
  int state_file_t = -1;

  size_t fsiz = 0;
  {
    struct stat s;
    if (fstat(fileno(f), &s) != 0) {
      perror("[migrate::run ERROR] fstat");
      goto ferr;
    }

    fsiz = s.st_size;
    fprintf(stderr, "%s: File size: %ld byte(s)\n", file.c_str(), fsiz);
  }

  switch (fsiz) {
  case 1'000'000'000 / (sizeof(uint64_t) * CHAR_BIT) * sizeof(uint64_t):
    // 1. billion no color
#ifdef BUILD_BILLION_NO_COLOR
    no_migration_needed();
    goto end;
#endif
    state_file_t = 0;
    break;
  case 1'000'000'000 * (sizeof(cbox_t)):
    // 2. billion with color
#ifdef BUILD_BILLION_WITH_COLOR
    no_migration_needed();
    goto end;
#endif
    state_file_t = 1;
    break;
  case 1'000'000'000'000 / (sizeof(uint64_t) * CHAR_BIT) * sizeof(uint64_t):
    // 3. trillion no color
#ifdef BUILD_TRILLION_NO_COLOR
    no_migration_needed();
    goto end;
#endif
    state_file_t = 2;
    break;
  case 1'000'000'000'000 * (sizeof(cbox_t)):
    // 4. trillion with color
#ifdef BUILD_TRILLION_WITH_COLOR
    no_migration_needed();
    goto end;
#endif
    state_file_t = 3;
    // fprintf(
    //     stderr,
    //     "Recognized state file but migration isn't implemented,
    //     exiting...\n");
    // goto ferr;
    break;
  default:
    // 5. broken state, should reset everything
    fprintf(stderr, "Unrecognized/corrupted state file, resetting...\n");
  }

  print_state_file_type(file.c_str(), state_file_t);
  print_migrate_target();

  // !TODO

end:
  fclose(f);
  f = NULL;

  return 0;

ferr:
  fclose(f);
  return status;
}
} // namespace atcboxes::migrate
