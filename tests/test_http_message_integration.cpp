/*
 * HTTP Message Parsing Integration Tests
 * 
 * This file contains comprehensive integration tests for the parse_http_message function,
 * testing its ability to correctly handle HTTP messages received over real socket connections
 * in various scenarios that mirror real-world network conditions.
 * 
 * Test scenarios include:
 * - Complete HTTP messages sent at once
 * - Messages sent in fragments (simulating network packet fragmentation)
 * - Messages with bodies sent in parts
 * - Byte-by-byte transmission (worst-case scenario)
 * - Large messages with many headers
 * - Messages split at inconvenient boundaries (mid-header, mid-value)
 * 
 * These tests use socketpair() to create a controlled environment that simulates
 * client-server communication without requiring actual network setup.
 * Based on the client.c behavior as reference for realistic fragmentation patterns.
 */

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>

extern "C" {
    #include "http_parser.h"
    #include "http_lib.h"
}

TEST_GROUP(ParseHttpMessageIntegrationTests)
{
    int client_socket;
    int server_socket;
    HTTP_MESSAGE message;
    
    void setup()
    {
        // Create a socket pair for testing
        int sockets[2];
        CHECK_EQUAL(0, socketpair(AF_UNIX, SOCK_STREAM, 0, sockets));
        
        server_socket = sockets[0];  // Server side (receive)
        client_socket = sockets[1];  // Client side (send)
        
        // Reset message structure
        message = init_http_message();
        
        // Clear any previous mock expectations
        mock().clear();
    }

    void teardown()
    {
        // Cleanup
        free_http_message(&message);
        close(client_socket);
        close(server_socket);
        
        // Clear mock expectations
        mock().clear();
    }
    
    void send_http_data_real(const char* data, size_t len)
    {
        // Use the real system send function for our socket pair
        // We'll bypass the mock by using write() instead of send()
        ssize_t sent = write(client_socket, data, len);
        CHECK_TRUE(sent > 0);
    }
    
    void send_http_string_real(const char* data)
    {
        send_http_data_real(data, strlen(data));
    }
};

// Test 1: Complete HTTP GET request sent at once
TEST(ParseHttpMessageIntegrationTests, ParseCompleteGetRequest)
{
    const char* http_request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestClient/1.0\r\n"
        "Accept: */*\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    // Send complete message at once
    send_http_string_real(http_request);
    
    // Close write end to signal end of transmission
    shutdown(client_socket, SHUT_WR);
    
    // Parse the message
    parse_http_message(&message, server_socket, REQUEST);
    
    // Verify start line
    char method_str[64], protocol_str[64];
    get_value_from_http_method(message.start_line.request.method, method_str, sizeof(method_str));
    get_value_from_http_protocol(message.start_line.request.protocol, protocol_str, sizeof(protocol_str));
    
    STRCMP_EQUAL("GET", method_str);
    STRCMP_EQUAL("/index.html", message.start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", protocol_str);
    
    // Verify headers
    CHECK_EQUAL(4, message.header_count);
    
    const char* host = get_header_value(message.headers, message.header_count, "Host");
    STRCMP_EQUAL("example.com", host);
    
    const char* user_agent = get_header_value(message.headers, message.header_count, "User-Agent");
    STRCMP_EQUAL("TestClient/1.0", user_agent);
    
    const char* accept = get_header_value(message.headers, message.header_count, "Accept");
    STRCMP_EQUAL("*/*", accept);
    
    const char* connection = get_header_value(message.headers, message.header_count, "Connection");
    STRCMP_EQUAL("close", connection);
    
    // No body expected
    CHECK_EQUAL(0, message.body_length);
}

// Test 2: Complete HTTP POST request with body sent at once
TEST(ParseHttpMessageIntegrationTests, ParseCompletePostRequestWithBody)
{
    const char* http_request = 
        "POST /api/users HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 24\r\n"
        "\r\n"
        "{\"name\":\"John\",\"age\":30}";
    
    // Send complete message at once
    send_http_string_real(http_request);
    
    // Close write end to signal end of transmission
    shutdown(client_socket, SHUT_WR);
    
    // Parse the message
    parse_http_message(&message, server_socket, REQUEST);
    
    // Verify start line
    char method_str[64], protocol_str[64];
    get_value_from_http_method(message.start_line.request.method, method_str, sizeof(method_str));
    get_value_from_http_protocol(message.start_line.request.protocol, protocol_str, sizeof(protocol_str));
    
    STRCMP_EQUAL("POST", method_str);
    STRCMP_EQUAL("/api/users", message.start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", protocol_str);
    
    // Verify headers
    CHECK_EQUAL(3, message.header_count);
    
    const char* host = get_header_value(message.headers, message.header_count, "Host");
    STRCMP_EQUAL("api.example.com", host);
    
    const char* content_type = get_header_value(message.headers, message.header_count, "Content-Type");
    STRCMP_EQUAL("application/json", content_type);
    
    const char* content_length = get_header_value(message.headers, message.header_count, "Content-Length");
    STRCMP_EQUAL("24", content_length);
    
    // Verify body
    CHECK_EQUAL(24, message.body_length);
    CHECK_TRUE(message.body_fd > 0);
    
    // Read body from file descriptor
    char body_buffer[100] = {0};
    lseek(message.body_fd, 0, SEEK_SET);  // Reset to beginning
    ssize_t body_read = read(message.body_fd, body_buffer, sizeof(body_buffer) - 1);
    CHECK_EQUAL(24, body_read);
    STRCMP_EQUAL("{\"name\":\"John\",\"age\":30}", body_buffer);
}

// Test 3: HTTP request sent in small fragments
TEST(ParseHttpMessageIntegrationTests, ParseFragmentedRequest)
{
    const char* fragments[] = {
        "GET /f",
        "ragmented HTTP/1.1\r\n",
        "Host: ",
        "example.com\r\n",
        "User-Agent: ",
        "FragmentedClient/1.0\r\n",
        "Accept: text/html",
        ",application/xml\r\n",
        "\r\n"
    };
    
    const int num_fragments = sizeof(fragments) / sizeof(fragments[0]);
    
    // Send fragments with small delays
    for (int i = 0; i < num_fragments; i++) {
        send_http_string_real(fragments[i]);
        usleep(1000);  // 1ms delay between fragments
    }
    
    // Close write end to signal end of transmission
    shutdown(client_socket, SHUT_WR);
    
    // Parse the message
    parse_http_message(&message, server_socket, REQUEST);
    
    // Verify start line
    char method_str[64], protocol_str[64];
    get_value_from_http_method(message.start_line.request.method, method_str, sizeof(method_str));
    get_value_from_http_protocol(message.start_line.request.protocol, protocol_str, sizeof(protocol_str));
    
    STRCMP_EQUAL("GET", method_str);
    STRCMP_EQUAL("/fragmented", message.start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", protocol_str);
    
    // Verify headers
    CHECK_EQUAL(3, message.header_count);
    
    const char* host = get_header_value(message.headers, message.header_count, "Host");
    STRCMP_EQUAL("example.com", host);
    
    const char* user_agent = get_header_value(message.headers, message.header_count, "User-Agent");
    STRCMP_EQUAL("FragmentedClient/1.0", user_agent);
    
    const char* accept = get_header_value(message.headers, message.header_count, "Accept");
    STRCMP_EQUAL("text/html,application/xml", accept);
}

// Test 4: HTTP request with body sent in fragments
TEST(ParseHttpMessageIntegrationTests, ParseFragmentedRequestWithBody)
{
    const char* header_part = 
        "POST /api/data HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 33\r\n"
        "\r\n";
    
    const char* body_fragments[] = {
        "This is ",
        "a fragmented ",
        "body message"
    };
    
    // Send header first
    send_http_string_real(header_part);
    usleep(10000);  // 10ms delay
    
    // Send body in fragments
    for (size_t i = 0; i < sizeof(body_fragments) / sizeof(body_fragments[0]); i++) {
        send_http_string_real(body_fragments[i]);
        usleep(5000);  // 5ms delay between body fragments
    }
    
    // Close write end to signal end of transmission
    shutdown(client_socket, SHUT_WR);
    
    // Parse the message
    parse_http_message(&message, server_socket, REQUEST);
    
    // Verify start line
    char method_str[64], protocol_str[64];
    get_value_from_http_method(message.start_line.request.method, method_str, sizeof(method_str));
    get_value_from_http_protocol(message.start_line.request.protocol, protocol_str, sizeof(protocol_str));
    
    STRCMP_EQUAL("POST", method_str);
    STRCMP_EQUAL("/api/data", message.start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", protocol_str);
    
    // Verify content length
    CHECK_EQUAL(33, message.body_length);
    CHECK_TRUE(message.body_fd > 0);
    
    // Read body from file descriptor
    char body_buffer[100] = {0};
    lseek(message.body_fd, 0, SEEK_SET);  // Reset to beginning
    ssize_t body_read = read(message.body_fd, body_buffer, sizeof(body_buffer) - 1);
    CHECK_EQUAL(33, body_read);
    STRCMP_EQUAL("This is a fragmented body message", body_buffer);
}

// Test 5: HTTP request similar to client.c example - split across multiple sends
TEST(ParseHttpMessageIntegrationTests, ParseClientStyleFragmentedRequest)
{
    // This mimics the client.c behavior with split sends
    const char* first_part = 
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestClient/1.0\r\nAccept:";
    
    const char* second_part = 
        " */*\r\n"
        "X-Dummy-Header: hello-world\r\n"
        "Content-Length: 15\r\n"
        "\r\n"
        "Sample Body!!!\n";
    
    // Send first part
    send_http_string_real(first_part);
    usleep(50000);  // 50ms delay (simulating network delay)
    
    // Send second part
    send_http_string_real(second_part);
    
    // Close write end to signal end of transmission
    shutdown(client_socket, SHUT_WR);
    
    // Parse the message
    parse_http_message(&message, server_socket, REQUEST);
    
    // Verify start line
    char method_str[64], protocol_str[64];
    get_value_from_http_method(message.start_line.request.method, method_str, sizeof(method_str));
    get_value_from_http_protocol(message.start_line.request.protocol, protocol_str, sizeof(protocol_str));
    
    STRCMP_EQUAL("GET", method_str);
    STRCMP_EQUAL("/", message.start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", protocol_str);
    
    // Verify headers
    CHECK_EQUAL(5, message.header_count);
    
    const char* accept = get_header_value(message.headers, message.header_count, "Accept");
    STRCMP_EQUAL("*/*", accept);
    
    const char* dummy_header = get_header_value(message.headers, message.header_count, "X-Dummy-Header");
    STRCMP_EQUAL("hello-world", dummy_header);
    
    // Verify body
    CHECK_EQUAL(15, message.body_length);
    CHECK_TRUE(message.body_fd > 0);
    
    // Read body from file descriptor
    char body_buffer[100] = {0};
    lseek(message.body_fd, 0, SEEK_SET);
    ssize_t body_read = read(message.body_fd, body_buffer, sizeof(body_buffer) - 1);
    CHECK_EQUAL(15, body_read);
    STRCMP_EQUAL("Sample Body!!!\n", body_buffer);
}

// Test 6: HTTP request with very small byte-by-byte fragments
TEST(ParseHttpMessageIntegrationTests, ParseByteByByteFragments)
{
    const char* http_request = 
        "GET /slow HTTP/1.1\r\n"
        "Host: slow.example.com\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    // Send byte by byte
    size_t len = strlen(http_request);
    for (size_t i = 0; i < len; i++) {
        send_http_data_real(&http_request[i], 1);
        usleep(500);  // 0.5ms between each byte
    }
    
    // Close write end to signal end of transmission
    shutdown(client_socket, SHUT_WR);
    
    // Parse the message
    parse_http_message(&message, server_socket, REQUEST);
    
    // Verify parsing worked correctly despite fragmentation
    char method_str[64], protocol_str[64];
    get_value_from_http_method(message.start_line.request.method, method_str, sizeof(method_str));
    get_value_from_http_protocol(message.start_line.request.protocol, protocol_str, sizeof(protocol_str));
    
    STRCMP_EQUAL("GET", method_str);
    STRCMP_EQUAL("/slow", message.start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", protocol_str);
    
    CHECK_EQUAL(2, message.header_count);
    
    const char* host = get_header_value(message.headers, message.header_count, "Host");
    STRCMP_EQUAL("slow.example.com", host);
    
    const char* connection = get_header_value(message.headers, message.header_count, "Connection");
    STRCMP_EQUAL("close", connection);
    
    CHECK_EQUAL(0, message.body_length);
}

// Test 7: HTTP request with large headers and body fragments
TEST(ParseHttpMessageIntegrationTests, ParseLargeFragmentedRequest)
{
    const char* body = "name=John+Doe&email=john%40example.com&age=30";
    
    // Send headers in chunks
    const char* header_chunks[] = {
        "POST /api/upload HTTP/1.1\r\n",
        "Host: upload.example.com\r\n",
        "User-Agent: IntegrationTestClient/2.0 (Linux; x64) TestFramework/1.0\r\n",
        "Accept: application/json, text/plain, */*\r\n",
        "Accept-Language: en-US,en;q=0.9,es;q=0.8\r\n",
        "Accept-Encoding: gzip, deflate, br\r\n",
        "Content-Type: application/x-www-form-urlencoded\r\n",
        "Content-Length: 45\r\n",
        "\r\n"
    };
    
    // Send header chunks
    for (size_t i = 0; i < sizeof(header_chunks) / sizeof(header_chunks[0]); i++) {
        send_http_string_real(header_chunks[i]);
        usleep(2000);  // 2ms delay
    }
    
    // Send body in small chunks
    size_t body_len = strlen(body);
    const size_t chunk_size = 8;
    for (size_t i = 0; i < body_len; i += chunk_size) {
        size_t current_chunk_size = (i + chunk_size > body_len) ? body_len - i : chunk_size;
        send_http_data_real(&body[i], current_chunk_size);
        usleep(3000);  // 3ms delay
    }
    
    // Close write end
    shutdown(client_socket, SHUT_WR);
    
    // Parse the message
    parse_http_message(&message, server_socket, REQUEST);
    
    // Verify start line
    char method_str[64], protocol_str[64];
    get_value_from_http_method(message.start_line.request.method, method_str, sizeof(method_str));
    get_value_from_http_protocol(message.start_line.request.protocol, protocol_str, sizeof(protocol_str));
    
    STRCMP_EQUAL("POST", method_str);
    STRCMP_EQUAL("/api/upload", message.start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", protocol_str);
    
    // Verify we got all headers
    CHECK_EQUAL(7, message.header_count);
    
    // Verify some key headers
    const char* content_type = get_header_value(message.headers, message.header_count, "Content-Type");
    STRCMP_EQUAL("application/x-www-form-urlencoded", content_type);
    
    const char* user_agent = get_header_value(message.headers, message.header_count, "User-Agent");
    STRCMP_EQUAL("IntegrationTestClient/2.0 (Linux; x64) TestFramework/1.0", user_agent);
    
    // Verify body
    CHECK_EQUAL(45, message.body_length);
    CHECK_TRUE(message.body_fd > 0);
    
    // Read and verify body content
    char body_buffer[100] = {0};
    lseek(message.body_fd, 0, SEEK_SET);
    ssize_t body_read = read(message.body_fd, body_buffer, sizeof(body_buffer) - 1);
    CHECK_EQUAL(45, body_read);
    STRCMP_EQUAL("name=John+Doe&email=john%40example.com&age=30", body_buffer);
}

// Test 8: HTTP request with headers split at inconvenient boundaries
TEST(ParseHttpMessageIntegrationTests, ParseInconvenientFragmentBoundaries)
{
    // Split in the middle of header names and values
    const char* fragments[] = {
        "GET /boundary-test HTTP/1.1\r",
        "\nHost: boundary-",
        "test.example.com\r\nContent-",
        "Type: text/pla",
        "in\r\nX-Custom-Header: split-",
        "value-test\r\n\r\n"
    };
    
    // Send fragments
    for (size_t i = 0; i < sizeof(fragments) / sizeof(fragments[0]); i++) {
        send_http_string_real(fragments[i]);
        usleep(5000);  // 5ms delay
    }
    
    // Close write end
    shutdown(client_socket, SHUT_WR);
    
    // Parse the message
    parse_http_message(&message, server_socket, REQUEST);
    
    // Verify parsing worked despite awkward boundaries
    char method_str[64], protocol_str[64];
    get_value_from_http_method(message.start_line.request.method, method_str, sizeof(method_str));
    get_value_from_http_protocol(message.start_line.request.protocol, protocol_str, sizeof(protocol_str));
    
    STRCMP_EQUAL("GET", method_str);
    STRCMP_EQUAL("/boundary-test", message.start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", protocol_str);
    
    CHECK_EQUAL(3, message.header_count);
    
    const char* host = get_header_value(message.headers, message.header_count, "Host");
    STRCMP_EQUAL("boundary-test.example.com", host);
    
    const char* content_type = get_header_value(message.headers, message.header_count, "Content-Type");
    STRCMP_EQUAL("text/plain", content_type);
    
    const char* custom_header = get_header_value(message.headers, message.header_count, "X-Custom-Header");
    STRCMP_EQUAL("split-value-test", custom_header);
}