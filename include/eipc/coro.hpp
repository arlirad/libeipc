#pragma once

#include <coroutine>
#include <utility>

namespace eipc {
    template <typename T>
    struct coro {
        struct final_awaiter {
            bool await_ready() noexcept {
                return false;
            }

            template <typename U>
            auto await_suspend(std::coroutine_handle<U> caller) noexcept {
                return caller.promise().next;
            }

            void await_resume() noexcept {}
        };

        struct promise_type {
            T                       value;
            std::coroutine_handle<> next;
            bool                    done = false;

            coro<T> get_return_object() {
                return {std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            std::suspend_always initial_suspend() noexcept {
                return {};
            }

            final_awaiter final_suspend() noexcept {
                this->done = true;
                return {};
            }

            void return_value(T v) noexcept {
                value = std::move(v);
            }

            void unhandled_exception() noexcept {}
        };

        struct awaiter {
            std::coroutine_handle<promise_type> handle;

            bool await_ready() noexcept {
                return this->handle.promise().done;
            }

            template <typename U>
            auto await_suspend(std::coroutine_handle<U> caller) noexcept {
                this->handle.promise().next = caller;
                return this->handle;
            }

            T await_resume() noexcept {
                return std::move(this->handle.promise().value);
            }
        };

        std::coroutine_handle<promise_type> handle;

        coro(std::coroutine_handle<promise_type> h) : handle(h) {}

        ~coro() {
            if (this->handle)
                this->handle.destroy();
        }

        T get_value() {
            return std::move(this->handle.promise().value);
        }

        auto operator co_await() {
            return awaiter{this->handle};
        }
    };
}