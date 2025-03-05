#pragma once

#include <atomic>
#include <future>
#include <optional>

namespace eipc {
    constexpr auto MAX_DATA_LEN = 512 - sizeof(int) * 4;

    struct bus_inner;

    struct frame {
        int  function;
        int  pid;
        int  flags;
        int  reserved1;
        char data[MAX_DATA_LEN];
    };

    class response {
        public:
        template <typename T>
        std::optional<T> get() {
            return *static_cast<T*>(this->_frame->data);
        }

        private:
        std::unique_ptr<frame> _frame;
    };

    class bus {
        public:
        bus(const char* name);
        ~bus();

        bool init();

        template <typename T>
        std::future<std::optional<response>> send(int function, const T* value) {
            return this->send(function, static_cast<const char*>(value), sizeof(T));
        }

        std::future<std::optional<response>> send(int function, const char* data, int len);

        private:
        std::atomic<int>           _sequence = 0;
        int                        _pid      = 0;
        std::shared_ptr<bus_inner> _inner;
        std::string                _path;
    };
}