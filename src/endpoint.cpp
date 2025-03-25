#include "common.hpp"
#include "eipc.hpp"
#include "inner.hpp"
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
    int get_sockaddr(sockaddr_un& addr, std::string name) {
        addr            = {};
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, name.c_str(), sizeof(addr.sun_path) - 1);

        return sizeof(addr.sun_family) + name.size();
    }

    endpoint::endpoint(const char* name) : _inner(std::make_shared<endpoint_inner>()) {
        this->_path = get_root() + std::string(name);
        this->_pid  = getpid();
    }

    endpoint::~endpoint() {
        close(this->_inner->socket);
        unlink(this->_path.c_str());
    }

    bool endpoint::init(const char* remote_name) {
        mkdir(get_root().c_str(), 0700);

        this->_remote_path = get_root() + remote_name;
        sockaddr_un local, remote;

        int local_len  = get_sockaddr(local, this->_path);
        int remote_len = get_sockaddr(remote, this->_remote_path);

        unlink(this->_path.c_str());

        int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
        if (sock == -1)
            return false;

        if (bind(sock, reinterpret_cast<sockaddr*>(&local), local_len) == -1)
            return false;

        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

        this->_inner->socket      = sock;
        this->_inner->local_addr  = local;
        this->_inner->local_len   = local_len;
        this->_inner->remote_addr = remote;
        this->_inner->remote_len  = remote_len;
        this->native_handle       = sock;

        return true;
    }

    void endpoint::ready() {
        frame syn = {};

        syn.function = 0;
        syn.seq      = this->_seq++;
        syn.flags    = FLAG_SYN;

        this->put(syn);
    }

    void endpoint::on(uint32_t function, std::function<response(request&)> func) {
        this->_handlers[function % 256] = func;
    }

    bool endpoint::try_receive() {
        frame       fr;
        sockaddr_un from;
        socklen_t   addrlen;
        ssize_t     len;

        if ((len = recvfrom(this->_inner->socket, &fr, sizeof(frame), 0,
                            reinterpret_cast<sockaddr*>(&from), &addrlen)) == -1)
            return false;

        if (fr.flags & FLAG_SYN)
            this->retransmit_all();
        else if (fr.flags & FLAG_REP)
            this->received_response(fr);
        else
            this->handle_request(fr);

        return true;
    }

    void endpoint::retransmit_all() {
        for (auto p : this->_pending)
            this->put(p->request);
    }

    coro<response> endpoint::request_async(int function, const char* data, size_t len) {
        len = std::min(len, MAX_DATA_LEN);

        std::shared_ptr<pending> p = std::make_shared<pending>();

        p->request.function = function;
        p->request.len      = len;
        p->request.pid      = this->_pid;
        p->request.seq      = this->_seq++;
        p->request.flags    = 0;

        memcpy(p->request.data, data, len);

        this->_pending.push_back(p);
        this->put(p->request);

        std::optional<frame> result = co_await p->handle;

        if (!result.has_value())
            co_return {};

        co_return response(result.value());
    }

    int endpoint::put(frame& fr) {
        return sendto(this->_inner->socket, &fr, HEADER_OVERHEAD + fr.len, 0,
                      reinterpret_cast<sockaddr*>(&this->_inner->remote_addr),
                      this->_inner->remote_len);
    }

    void endpoint::handle_request(frame& fr) {
        request req;

        req.raw_frame = fr;

        response res = this->_handlers[fr.function % 256](req);

        res.raw_frame.function = fr.function;
        res.raw_frame.pid      = this->_pid;
        res.raw_frame.seq      = fr.seq;
        res.raw_frame.flags    = FLAG_REP;

        this->put(res.raw_frame);
    }

    void endpoint::received_response(frame& fr) {
        for (auto p : this->_pending) {
            if (fr.seq != p->request.seq)
                continue;

            p->response = fr;

            if (p->handle.chained)
                p->handle.chained.resume();

            this->_pending.erase(std::remove(this->_pending.begin(), this->_pending.end(), p),
                                 this->_pending.end());

            break;
        }
    }
}