#include <gtest/gtest.h>

#include <eipc.hpp>
#include <thread>

TEST(ConnectionTests, ConstructionTest) {
    eipc::endpoint endpoint("/test");

    ASSERT_EQ(endpoint.init(), true);
}

TEST(ConnectionTests, SendTest) {
    eipc::endpoint endpoint_a("/test");
    eipc::endpoint endpoint_b("/test");

    ASSERT_EQ(endpoint_a.init(), true);
    ASSERT_EQ(endpoint_b.init(), true);

    endpoint_b.send(0, 7765).valid();

    std::this_thread::yield();

    eipc::request req;

    ASSERT_TRUE(endpoint_a.poll(req));
    ASSERT_EQ(req.get<int>(), 7765);
}