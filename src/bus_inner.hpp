#include <mutex>
#include <unistd.h>

namespace eipc {
    struct bus_inner {
        std::mutex mutex;
        int        sock_handle;
    };
}