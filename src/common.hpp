#include <chrono>

using namespace std::chrono_literals;

namespace eipc {
    constexpr auto RETRANSMIT_TIME     = 1000ms;
    constexpr auto RETRANSMIT_LOOP     = 10ms;
    constexpr auto DEFAULT_SOCKET_ROOT = "/tmp/.eipc-unix/";
}