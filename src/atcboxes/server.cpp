#include "atcboxes/server.h"
#include "atcboxes/atcboxes.h"
#include "atcboxes/commands.h"
#include "uWebSockets/src/App.h"
#include <csignal>
#include <cstdint>
#include <string>

#define PORT 3000

namespace atcboxes::server {

using App = uWS::App;

enum ws_data_flags_e : long { WSDF_NONE = 0, WSDF_C = 1 };

struct ws_data_t {
  char n_o;
  char n_i;
  long flags;
  std::string cached;

  long long last_ts;
  // int inv_p;
};

using WS = uWS::WebSocket<false, true, ws_data_t>;

App *_app_ptr = nullptr;
uWS::Loop *_loop_ptr = nullptr;
std::atomic<bool> shutting_down = false;
std::atomic<int> status = 0;
std::atomic<int> int_count = 0;

static long long get_current_ts() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

static long long get_current_ts_seed() {
  return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}

static void on_sigint(int) {
  constexpr char msg[] = "Received SIGINT\n";
  constexpr size_t msgsiz = (sizeof(msg) / sizeof(*msg)) - 1;
  write(STDIN_FILENO, msg, msgsiz);
  shutdown();

  int_count++;
  if (int_count >= 5) {
    constexpr char msg2[] = "Received 5 SIGINT\nHard exiting...";
    constexpr size_t msgsiz2 = (sizeof(msg2) / sizeof(*msg2)) - 1;
    write(STDIN_FILENO, msg2, msgsiz2);
    exit(-1);
  }
}

static int get_rand() {
  srand(get_current_ts_seed());
  return rand();
}

static int get_rand_modulo(size_t mod, size_t thres = 0) {
  size_t r = get_rand();
  return r > thres ? r % mod : r;
}

constexpr const char alphanums[] =
    "1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz=_";
constexpr const size_t apn_siz = (sizeof(alphanums) / sizeof(*alphanums)) - 1;

static char get_rand_char() {
  int rs = get_rand_modulo(apn_siz, apn_siz);
  return alphanums[rs];
}

static void rand_chars(char *__restrict p, size_t len) {
  int pd = len <= 3 ? 1 : 3;
  for (size_t i = 0; i < (len - pd); i++) {
    p[i] = get_rand_char();
  }

  if (pd == 3) {
    p[len - 3] = '=';
    p[len - 2] = '=';
  }

  p[len - 1] = '\0';
}

static void inc(WS *ws, std::string_view data) {
  auto *ud = ws->getUserData();
  ud->n_i++;
  if (ud->n_i > ud->n_o) {
    ud->n_i = 0;

    ud->cached = std::string(data);
  }

  // printf("n_o(%d) n_i(%d) cached(%s) data(%s)\n", ud->n_o, ud->n_i,
  //        ud->cached.c_str(), std::string(data).c_str());
}

static void ws_send(WS *ws, std::string_view msg) {
  ws->send(msg);

  inc(ws, msg);
}

static void publish_global(WS *ws, std::string_view data) {
  ws->publish("global", data);
  // inc(ws, data);
}

uint64_t uc = 0;

static std::string p_uc() { return std::string("uc;") + std::to_string(uc); }

static void publish_user_count(WS *ws) { publish_global(ws, p_uc()); }

static void send_user_count(WS *ws) { ws->send(p_uc()); }

static void increment_user_count(WS *ws) {
  uc++;
  publish_user_count(ws);
}

static void decrement_user_count(WS *ws) {
  uc--;
  publish_user_count(ws);
}

static void ws_end(WS *ws, int code = 0, std::string_view msg = {}) {
  // decrement_user_count(ws);
  ws->end(code, msg);
}

using ws_list_t = std::vector<WS *>;
ws_list_t connected_wses;

static ws_list_t::iterator find_cws(WS *ws) {
  if (connected_wses.empty())
    return connected_wses.end();

  auto i = connected_wses.begin();
  while (i != connected_wses.end()) {
    if (*i == ws)
      return i;
    i++;
  }

  return connected_wses.end();
}

static void add_cws(WS *ws) {
  if (find_cws(ws) != connected_wses.end())
    return;
  connected_wses.push_back(ws);
}

static void remove_cws(WS *ws) {
  auto i = find_cws(ws);
  if (i == connected_wses.end())
    return;

  connected_wses.erase(i);
}

static void handle_ws_command_outs(WS *ws, commands::command_outs_t &out) {
  for (const auto &i : out) {
    if (i.flags & 1) {
      ws->send(i.out);
    } else if ((i.flags & 1) == 0) {
      ws_send(ws, i.out);
    } else {
      fprintf(stderr,
              "[server::handle_ws_command_outs ERROR] Unknown command_out_t "
              "flags: %zu\nout: `%s`",
              i.flags, i.out.c_str());
    }
  }
}

int run() {
  signal(SIGINT, on_sigint);

  App app;

  App::WebSocketBehavior<ws_data_t> behavior;
  // leave every other option to its default for now
  behavior.open = [](WS *ws) {
    add_cws(ws);
    auto *ud = ws->getUserData();

    // ud->inv_p = 0;
    ud->n_o = 4;
    ud->n_i = 0;
    ud->flags = WSDF_NONE;
    ud->last_ts = get_current_ts();
    ud->cached.clear();

    ws->subscribe("global");
    increment_user_count(ws);
    send_user_count(ws);
  };

  behavior.close = [](WS *ws, int code, std::string_view msg) {
    remove_cws(ws);

    bool e = connected_wses.empty();
    auto *u = e ? ws : connected_wses.front();
    decrement_user_count(u);
    if (!e)
      send_user_count(u);
  };

  // handle dropped too?
  // behavior.dropped = ;

  behavior.message = [](WS *ws, std::string_view msg, uWS::OpCode op) {
    auto *ud = ws->getUserData();

    try {
      if (ud->flags & WSDF_C) {
        // printf("SHOULD BE: n_o(%d) n_i(%d) cached(%s) data(%s)\n", ud->n_o,
        //        ud->n_i, ud->cached.c_str(), std::string(msg).c_str());

        if (!ud->cached.empty() && msg != ud->cached) {
          ws_end(ws, 69);
          return;
        }

        ud->flags &= (~WSDF_C);
        return;
      }

      commands::command_outs_t out;
      int status = commands::run(msg, out);

      if (status < 0) {
        ws_end(ws, 69);
        return;
      }

      switch (status) {
      case 0:
        handle_ws_command_outs(ws, out);
        break;
      case 1: {
        int r = switch_state(std::stoull(std::string(msg)));

        if (r == -1) {
          ws_end(ws, 69);
          return;
        }

        publish_global(ws, commands::p_state(std::string(msg), r));

        const long long cur = get_current_ts();
        if ((cur - ud->last_ts) < (((cur & 1) == 0) ? 90 : 155)) {
          srand(cur);
          int rs = get_rand() & 1;
          if (rs) {
            ws_send(ws, "l;");
            char b[3];
            rand_chars(b, 3);
            int n = get_rand_modulo(10);
            ws_send(ws, std::string(b) + std::to_string(n));
            ud->n_o = n > 0 ? n : ud->n_o;
          }

          size_t x = get_rand_modulo(ud->n_o) * 2 + ud->n_o;
          for (size_t i = 0; i < x; i++) {
            char b[9];
            rand_chars(b, 9);
            ws_send(ws, std::string(b));
          }

          ws_send(ws, "h;");

          ud->flags |= WSDF_C;
        }

        ud->last_ts = cur;
        break;
      }
      } // switch
    } catch (...) {
      ws_end(ws, 420);
      std::cerr << "[message ERROR]: `" << msg << "`\n";

      return;
    }
  };

  app.ws<ws_data_t>("/game", std::move(behavior));

  app.listen(PORT, [](us_listen_socket_t *listen_socket) {
    if (listen_socket)
      fprintf(stderr, "[server] Listening on port %d \n", PORT);
    else {
      fprintf(stderr, "[server ERROR] Listening socket is null\nPORT might "
                      "already in use\n");
      status = 1;
      shutdown();
    }
  });

  _app_ptr = &app;
  _loop_ptr = uWS::Loop::get();

  app.run();

  _app_ptr = nullptr;
  shutting_down = false;

  signal(SIGINT, SIG_DFL);

  int temp_s = status;
  status = 0;
  return temp_s;
}

int shutdown() {
  if (!_loop_ptr || !_app_ptr)
    return -1;

  if (shutting_down)
    return 1;

  shutting_down = true;

  _loop_ptr->defer([]() {
    if (!_app_ptr) {
      fprintf(stderr, "[server::shutdown ERROR] _app_ptr "
                      "is null on callback\n");
      return;
    }

    fprintf(stderr, "[server] Shutting down...\n");

    _app_ptr->close();
  });

  constexpr char m[] = "[server] Shutting down callback dispatched\n";
  constexpr size_t ms = (sizeof(m) / sizeof(*m)) - 1;
  write(STDERR_FILENO, m, ms);

  return 0;
}

} // namespace atcboxes::server
