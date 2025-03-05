#pragma once

#include <atomic>
#include <future>
#include <optional>

namespace eipc {
    constexpr auto MAX_FRAME_LEN   = 512;
    constexpr auto HEADER_OVERHEAD = sizeof(int) * 4;
    constexpr auto MAX_DATA_LEN    = MAX_FRAME_LEN - HEADER_OVERHEAD;

    struct endpoint_inner;

    struct frame {
        int  function;
        int  len;
        int  flags;
        int  reserved0;
        char data[MAX_DATA_LEN];
    };

    class request {
        public:
        frame raw_frame;

        template <typename T>
        T get() {
            return *reinterpret_cast<T*>(this->raw_frame.data);
        }
    };

    class response {
        public:
        frame raw_frame;

        template <typename T>
        std::optional<T> get() {
            return *reinterpret_cast<T*>(this->raw_frame.data);
        }
    };

    class endpoint {
        public:
        endpoint(const char* name);
        ~endpoint();

        bool init();

        template <typename T>
        std::future<std::optional<response>> send(int function, const T& value) {
            return this->send(function, reinterpret_cast<const char*>(&value), sizeof(T));
        }

        std::future<std::optional<response>> send(int function, const char* data, size_t len);
        bool                                 poll(request& dst);

        private:
        std::atomic<int>                _sequence = 0;
        int                             _pid      = 0;
        std::shared_ptr<endpoint_inner> _inner;
        std::string                     _path;
    };
}