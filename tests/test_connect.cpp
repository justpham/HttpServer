#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

extern "C" {
    #include "connect.h"
}

TEST_GROUP(ConnectTests)
{
    void setup()
    {
        // Setup code for each test
    }

    void teardown()
    {
        // Cleanup code for each test
    }
};

TEST(ConnectTests, GetInAddrIPv4)
{
    struct sockaddr_in addr4;
    addr4.sin_family = AF_INET;
    addr4.sin_addr.s_addr = htonl(INADDR_LOOPBACK); // 127.0.0.1
    
    void *result = get_in_addr((struct sockaddr*)&addr4);
    
    CHECK(result != NULL);
    CHECK_EQUAL(&addr4.sin_addr, result);
}

TEST(ConnectTests, GetInAddrIPv6)
{
    struct sockaddr_in6 addr6;
    addr6.sin6_family = AF_INET6;
    addr6.sin6_addr = in6addr_loopback; // ::1
    
    void *result = get_in_addr((struct sockaddr*)&addr6);
    
    CHECK(result != NULL);
    CHECK_EQUAL(&addr6.sin6_addr, result);
}

TEST(ConnectTests, ConnectToInvalidHost)
{
    // Test connecting to an invalid hostname
    int result = connect_to_host("invalid.hostname.that.does.not.exist");
    
    // Should return -1 (getaddrinfo error)
    CHECK_EQUAL(-1, result);
}
