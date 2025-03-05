#include "bus_inner.hpp"
#include "common.hpp"
#include "eipc.hpp"
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <algorithm>
#include <chrono>
#include <errno.h>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <vector>

namespace eipc {
    bool exchanger();

    bus::bus(const char* name)
        : _inner(std::make_shared<bus_inner>()), _path(std::string(SOCKET_ROOT) + name) {
        this->_pid = getpid();
    }

    bus::~bus() {}

    bool bus::init() {
        int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (fd == -1)
            return false;

        struct sockaddr_un addr = {};
        size_t             len  = this->_path.size() + sizeof(addr.sun_family);

        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path + 1, this->_path.c_str(), sizeof(addr.sun_path) - 2);

        this->_inner->sock_handle = fd;

        if (mkdir(SOCKET_ROOT, 0777) == -2)
            if (errno != EEXIST)
                return false;

        if (bind(fd, (sockaddr*) (&addr), len) == -1) {
            if (connect(fd, (sockaddr*) (&addr), len) == -1)
                return false;
        }

        return true;
    }

    std::future<std::optional<response>> bus::send(int function, const char* data, int len) {
        if (len > MAX_DATA_LEN)
            return {};

        return {};
    }
}