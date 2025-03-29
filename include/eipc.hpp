#pragma once

#include <eipc/coro.hpp>
#include <sys/un.h>

#include <atomic>
#include <coroutine>
#include <future>
#include <optional>
#include <vector>

namespace eipc {
    constexpr auto MAX_FRAME_LEN   = 512;
    constexpr auto HEADER_OVERHEAD = sizeof(int) * 6;
    constexpr auto MAX_DATA_LEN    = MAX_FRAME_LEN - HEADER_OVERHEAD;
    constexpr auto FLAG_INV        = 1 << 3;
    constexpr auto FLAG_REP        = 1 << 2;
    constexpr auto FLAG_SYN        = 1 << 1;
    constexpr auto FLAG_ACK        = 1 << 0;

    struct endpoint_inner;
    struct request_inner;
    struct pending;

    struct frame {
        uint32_t function;
        uint32_t len;
        uint32_t flags;
        uint32_t pid;
        uint32_t seq;
        uint32_t reserved;
        char     data[MAX_DATA_LEN];
    };

    class request {
        public:
        frame          raw_frame;
        request_inner* inner;

        int function() {
            return raw_frame.function;
        }

        template <typename T>
        T get() {
            return *reinterpret_cast<T*>(this->raw_frame.data);
        }

        bool operator==(const request& other) {
            return memcmp(this, &other, sizeof(request)) == 0;
        }
    };

    class response {
        public:
        frame raw_frame;

        response() {}

        response(const frame& r) {
            this->raw_frame = r;
        }

        template <typename T>
        response(const T& val) {
            *reinterpret_cast<T*>(this->raw_frame.data) = val;
            this->raw_frame.len                         = sizeof(val);
        }

        template <typename T>
        std::optional<T> get() {
            return *reinterpret_cast<T*>(this->raw_frame.data);
        }
    };

    class endpoint {
        public:
        int native_handle;

        endpoint(const char* name);
        ~endpoint();

        bool init(const char* remote);
        void ready();
        void on(uint32_t function, std::function<response(request&)> func);

        bool try_receive();
        void retransmit_all();

        template <typename T>
        coro<response> request_async(int function, const T& data) {
            const char* ptr    = reinterpret_cast<const char*>(&data);
            auto        result = co_await request_async(function, ptr, sizeof(data));

            co_return result;
        }

        coro<response> request_async(int function, const char* data, size_t len);

        coro<std::optional<request>> recv_request();
        void                         respond(request&, response&);

        private:
        struct await_handle {
            std::coroutine_handle<> chained;

            await_handle() {}

            bool await_ready() {
                // return _owner.response.has_value();
                return false;
            }

            template <typename U>
            void await_suspend(std::coroutine_handle<U> caller) {
                this->chained = caller;
            }

            void await_resume() {}
        };

        int                                   _pid           = 0;
        uint32_t                              _seq           = 0;
        std::function<response(request&)>     _handlers[256] = {};
        std::shared_ptr<endpoint_inner>       _inner;
        std::string                           _path;
        std::string                           _remote_path;
        std::vector<std::shared_ptr<pending>> _pending;
        std::vector<request>                  _requests;
        await_handle                          _request_received;

        int  put(frame& fr);
        void handle_request(frame& fr);
        void received_response(frame& fr);
    };

    std::string get_root();
    void        set_root(const char* path);
}