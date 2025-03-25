#include "eipc.hpp"

#include "common.hpp"

namespace eipc {
    std::string root(DEFAULT_SOCKET_ROOT);

    std::string get_root() {
        return root;
    }

    void set_root(const char* path) {
        root = path;
    }
}