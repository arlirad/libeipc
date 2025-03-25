#pragma once

#include <coroutine>
#include <utility>

namespace eipc {
    template <typename T>
    struct coro {
        struct promise_type {
            T                       value;
            std::coroutine_handle<> chained;

            coro<T> get_return_object() {
                return {std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            std::suspend_always final_suspend() noexcept {
                if (chained)
                    chained.resume();

                return {};
            }

            void return_value(T v) {
                value = std::move(v);
            }

            void unhandled_exception() {}
        };

        struct awaiter {
            coro<T>& _coro;

            bool await_ready() {
                return false;
            }

            template <typename U>
            void await_suspend(std::coroutine_handle<U> caller) {
                _coro.handle.promise().chained = caller;
                _coro.handle.resume();
            }

            T await_resume() {
                return std::move(_coro.handle.promise().value);
            }
        };

        std::coroutine_handle<promise_type> handle;

        coro(std::coroutine_handle<promise_type> h) : handle(h) {}
        ~coro() {
            if (handle)
                handle.destroy();
        }

        T get_value() {
            return std::move(handle.promise().value);
        }

        auto operator co_await() {
            return awaiter{*this};
        }
    };
}