#include <sys/types.h>
#include <sys/un.h>

#include <mqueue.h>
#include <mutex>
#include <unistd.h>

namespace eipc {
    struct endpoint_inner {
        mqd_t message_queue;
    };
}