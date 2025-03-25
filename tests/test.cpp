#include <gtest/gtest.h>
#include <sys/epoll.h>

#include <eipc.hpp>
#include <thread>

struct epoll_context {
    static const int MAX_EVENTS = 10;

    int                          fd;
    epoll_event                  events[MAX_EVENTS];
    std::vector<eipc::endpoint*> eps;
};

epoll_context epoll_setup() {
    return {.fd = epoll_create1(0), .events = {}};
}

void epoll_add(epoll_context& ctx, eipc::endpoint& ep) {
    epoll_event ev;

    ev.events  = EPOLLIN;
    ev.data.fd = ep.native_handle;

    epoll_ctl(ctx.fd, EPOLL_CTL_ADD, ep.native_handle, &ev);
    ctx.eps.push_back(&ep);
}

void epoll_poll_once(epoll_context& ctx) {
    int nfds = epoll_wait(ctx.fd, ctx.events, ctx.MAX_EVENTS, 100);

    for (int i = 0; i < nfds; i++) {
        epoll_event& event = ctx.events[i];

        for (int j = 0; j < ctx.eps.size(); j++) {
            if (event.data.fd != ctx.eps[j]->native_handle)
                continue;

            ctx.eps[j]->try_receive();
            return;
        }
    }
}

TEST(ConnectionTests, ConstructionTest) {
    eipc::set_root("./.eipc/");
    eipc::endpoint endpoint("test2.a");

    ASSERT_EQ(endpoint.init("test2.b"), true);
}

TEST(ConnectionTests, HandleTest) {
    const uint8_t TEST_FUNCTION = 0;

    eipc::set_root("./.eipc/");

    eipc::endpoint endpoint_a("test2.a");
    eipc::endpoint endpoint_b("test2.b");

    ASSERT_EQ(endpoint_a.init("test2.b"), true);
    ASSERT_EQ(endpoint_b.init("test2.a"), true);

    endpoint_a.ready();
    endpoint_b.ready();

    epoll_context ctx = epoll_setup();

    epoll_add(ctx, endpoint_a);
    epoll_add(ctx, endpoint_b);

    endpoint_b.on(TEST_FUNCTION, [](eipc::request& req) -> eipc::response {
        //
        return req.get<int>() % 32;
    });

    ASSERT_TRUE(endpoint_a.try_receive());
    ASSERT_TRUE(endpoint_b.try_receive());

    auto request_coro = endpoint_a.request_async(TEST_FUNCTION, 52);
    request_coro.handle.resume();

    ASSERT_TRUE(endpoint_b.try_receive());
    ASSERT_TRUE(endpoint_a.try_receive());

    auto response = request_coro.get_value();

    ASSERT_EQ(response.get<int>(), 20);
}