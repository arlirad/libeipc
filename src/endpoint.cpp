#include "common.hpp"
#include "eipc.hpp"
#include "endpoint_inner.hpp"
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>

#include <algorithm>
#include <chrono>
#include <errno.h>
#include <fcntl.h>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <vector>

namespace eipc {
    endpoint::endpoint(const char* name) : _inner(std::make_shared<endpoint_inner>()), _path(name) {
        this->_pid = getpid();
    }

    endpoint::~endpoint() {
        mq_close(this->_inner->message_queue);
    }

    bool endpoint::init() {
        mq_attr attr;

        attr.mq_flags   = 0;
        attr.mq_maxmsg  = 128;
        attr.mq_msgsize = MAX_FRAME_LEN;
        attr.mq_curmsgs = 0;

        int fd = mq_open(this->_path.c_str(), O_RDWR | O_NONBLOCK | O_CREAT, 0700, &attr);
        if (fd == -1)
            return false;

        this->_inner->message_queue = fd;
        return true;
    }

    std::future<std::optional<response>> endpoint::send(int function, const char* data,
                                                        size_t len) {
        if (len > MAX_DATA_LEN)
            return {};

        request req;

        req.raw_frame.function = function;

        memcpy(req.raw_frame.data, data, len);
        mq_send(this->_inner->message_queue, reinterpret_cast<char*>(&req.raw_frame),
                HEADER_OVERHEAD + len, 0);

        return {};
    }

    bool endpoint::poll(request& dst) {
        unsigned int prio;

        if (mq_receive(this->_inner->message_queue, reinterpret_cast<char*>(&dst.raw_frame),
                       sizeof(dst.raw_frame), &prio) == -1)
            return false;

        return true;
    }
}