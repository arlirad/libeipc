#include "eipc/coro.hpp"
#include <sys/types.h>
#include <sys/un.h>

#include <atomic>
#include <optional>
#include <unistd.h>

namespace eipc {
    struct request_inner {
        sockaddr_un addr;
    };

    struct endpoint_inner {
        int              socket;
        sockaddr_un      local_addr;
        socklen_t        local_len;
        sockaddr_un      remote_addr;
        socklen_t        remote_len;
        std::atomic<int> local_seq;
    };

    struct pending {
        struct await_handle {
            pending&                _owner;
            std::coroutine_handle<> chained;

            await_handle(pending& owner) : _owner(owner) {}

            bool await_ready() {
                // return _owner.response.has_value();
                return false;
            }

            template <typename U>
            void await_suspend(std::coroutine_handle<U> caller) {
                this->chained = caller;
            }

            std::optional<frame> await_resume() {
                return std::move(_owner.response);
            }
        };

        frame                request;
        std::optional<frame> response;
        await_handle         handle;

        pending() : handle(*this) {};
    };
}