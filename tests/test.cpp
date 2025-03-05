#include <gtest/gtest.h>

#include <eipc.hpp>

TEST(ConnectionTests, ConstructionTest) {
    eipc::bus bus("test");

    ASSERT_EQ(bus.init(), true);
}

TEST(ConnectionTests, SharingTest) {
    eipc::bus bus_a("test");
    eipc::bus bus_b("test");
    eipc::bus bus_c("test");

    ASSERT_EQ(bus_a.init(), true);
    ASSERT_EQ(bus_b.init(), true);
    ASSERT_EQ(bus_c.init(), true);
}

TEST(ConnectionTests, HangingTest) {
    eipc::bus bus_a("test");
    eipc::bus bus_c("test");
    eipc::bus bus_d("test");

    {
        eipc::bus bus_b("test");

        ASSERT_EQ(bus_a.init(), true);
        ASSERT_EQ(bus_b.init(), true);
        ASSERT_EQ(bus_c.init(), true);
    }

    ASSERT_EQ(bus_d.init(), true);
}