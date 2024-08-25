#include "atcboxes/runtime_cli.h"
#include "atcboxes/atcboxes.h"
#include "atcboxes/commands.h"
#include <sys/poll.h>
#include <thread>
#include <unistd.h>

namespace atcboxes::runtime_cli {

std::atomic<bool> running = false;
std::thread runt;

static void run_thread() {
  struct pollfd p[1];
  p[0].fd = STDIN_FILENO;
  p[0].events = POLLIN;

  bool getline_called = false;
  char *line = NULL;
  size_t len = 0;
  while (running) {
    int evc = poll(p, 1, 500);
    if (evc == -1) {
      fprintf(stderr, "[runtime_cli::run ERROR] poll:\n");
      goto err;
    }

    if (evc < 1)
      continue;

    ssize_t nread = getline(&line, &len, stdin);
    getline_called = true;
    if (nread == -1) {
      fprintf(stderr, "[runtime_cli::run ERROR] getline:\n");
      goto err;
    }

    if (line == NULL)
      continue;

    if (len > 0 && line[nread - 1] == '\n') {
      line[nread - 1] = '\0';
    }

    if (strlen(line) == 0)
      continue;

    commands::command_outs_t out;
    int status = commands::run(line, out);
    if (status < 0) {
      goto cmd_cont;
    } else
      switch (status) {
      case 0: {
        bool pstate = false;
        for (const auto &i : out) {
          if (i.out.find("ws;") == 0) {
            pstate = true;
            continue;
          }
          if (pstate) {
            for (size_t j = 0; j < i.out.size(); j++) {
              uint8_t s = i.out[j];
              fprintf(stderr, "%ld(%d) ", j, s);
            }
            fprintf(stderr, "\n");

            pstate = false;
            continue;
          }

          fprintf(stderr, "%s\n", i.out.c_str());
        }
      } break;
      case 1: {
        int64_t g = -1;
        try {
          g = std::stoll(line);
        } catch (const std::exception &e) {
          fprintf(stderr, "[runtime_cli::run ERROR]: %s\n", e.what());
          goto cmd_cont;
        }

        if (g < 0)
          goto cmd_cont;

        fprintf(stderr, "%d\n", get_state(g));
      } break;
      }

  cmd_cont:
    fprintf(stderr, "$%d\n", status);
  }

end:
  if (getline_called == true) {
    free(line);
    line = NULL;
  }
  return;
err:
  perror("[runtime_cli::run ERROR]");
  goto end;
}

int run() {
  if (running)
    return 1;

  struct pollfd p[1];
  p[0].fd = STDIN_FILENO;
  p[0].events = POLLIN;

  int evc = poll(p, 1, 500);
  if (evc == -1) {
    perror("[runtime_cli::run ERROR]");
    fprintf(stderr, "[runtime_cli::run ERROR] Abort init\n");
    return -1;
  }

  running = true;
  runt = std::thread(run_thread);

  return 0;
}

int shutdown() {
  running = false;

  if (runt.joinable())
    runt.join();

  return 0;
}

} // namespace atcboxes::runtime_cli
