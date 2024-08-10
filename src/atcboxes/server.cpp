#include "atcboxes/server.h"
#include "atcboxes/atcboxes.h"
#include "uWebSockets/src/App.h"
#include <csignal>
#include <cstdint>
#include <regex>
#include <string>

#define PORT 3000

namespace atcboxes::server {

using App = uWS::App;

struct ws_data_t {
  long long last_ts;
  // int inv_p;
};

using WS = uWS::WebSocket<false, true, ws_data_t>;

App *_app_ptr = nullptr;
uWS::Loop *_loop_ptr = nullptr;
std::atomic<bool> shutting_down = false;
std::atomic<int> status = 0;
std::atomic<int> int_count = 0;

long long get_current_ts() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::high_resolution_clock::now().time_since_epoch())
      .count();
}

void on_sigint(int) {
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

std::pair<int64_t, int64_t> get_subs_pc(const std::string &s) {
  std::regex word_regex("(\\d+)");
  auto begin = std::sregex_iterator(s.begin(), s.end(), word_regex);

  auto end = std::sregex_iterator();
  if (begin == end)
    return {-1, -1};

  std::string p = (*begin).str();
  begin++;

  if (begin == end)
    return {-1, -1};

  std::string c = (*begin).str();

  return {std::stoll(p), std::stoll(c)};
}

int subs(const std::string &s) {
  auto pc = get_subs_pc(s);

  if (pc.first == -1 || pc.second == -1)
    return -1;

  // !TODO: subs per page

  return 0;
}

int gcv(const std::string &s) {
  size_t idx = 0;
  int r = get_state(std::stoull(s, &idx));
  return idx > 0 ? r : -1;
}

void publish_global(WS *ws, std::string_view data) {
  ws->publish("global", data);
}

std::string p_state(const std::string &n, int s) {
  return "s;" + n + ';' + (s ? "1" : "0");
}

std::string p_gv() { return "v;" + std::to_string(get_gv()); }

uint64_t uc = 0;

std::string p_uc() { return std::string("uc;") + std::to_string(uc); }

void publish_user_count(WS *ws) { publish_global(ws, p_uc()); }

void send_user_count(WS *ws) { ws->send(p_uc()); }

void increment_user_count(WS *ws) {
  uc++;
  publish_user_count(ws);
}

void decrement_user_count(WS *ws) {
  uc--;
  publish_user_count(ws);
}

void ws_end(WS *ws, int code = 0, std::string_view msg = {}) {
  // decrement_user_count(ws);
  ws->end(code, msg);
}

using ws_list_t = std::vector<WS *>;
ws_list_t connected_wses;

ws_list_t::iterator find_cws(WS *ws) {
  if (connected_wses.empty())
    return connected_wses.end();

  auto i = connected_wses.begin();
  while (i != connected_wses.end()) {
    if (*i == ws)
      return i;
  }

  return connected_wses.end();
}

void add_cws(WS *ws) {
  if (find_cws(ws) != connected_wses.end())
    return;
  connected_wses.push_back(ws);
}

void remove_cws(WS *ws) {
  auto i = find_cws(ws);
  if (i == connected_wses.end())
    return;

  connected_wses.erase(i);
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
    ud->last_ts = get_current_ts();

    ws->subscribe("global");
    increment_user_count(ws);
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
      if (msg.find("sc;") == 0) {
        if (msg.length() < 6 || subs(std::string(msg.substr(3))) == -1) {
          ws_end(ws, 69);
          return;
        }
      }

      else if (msg.find("gcv;") == 0) {
        int s = 0;
        auto n = std::string(msg.substr(4));

        if (msg.length() < 5 || (s = gcv(n)) == -1) {
          ws_end(ws, 69);
          return;
        }

        ws->send(p_state(n, s));
      }

      else if (msg.find("gv;") == 0) {
        if (msg.length() != 3) {
          ws_end(ws, 69);
          return;
        }

        ws->send(p_gv());
      }

      else {
        int r = switch_state(std::stoull(std::string(msg)));

        if (r == -1) {
          ws_end(ws, 69);
          return;
        }

        publish_global(ws, p_state(std::string(msg), r));

        ud->last_ts = get_current_ts();
      }
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
