#ifndef SERVER_H
#define SERVER_H

#include <string>

namespace atcboxes::server {

int subs(const std::string &s);

int run();
int shutdown();

} // namespace atcboxes::server

#endif // SERVER_H
