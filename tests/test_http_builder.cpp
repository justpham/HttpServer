#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "CppUTestExt/MockSupport.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

extern "C" {
    #include "http_lib.h"
    #include "http_builder.h"
    #include "mocks/send_mock.h"
}

TEST_GROUP(BuildHeaderTests)
{
    HTTP_MESSAGE msg;
    char buffer[2048];
    
    void setup()
    {
        // Initialize HTTP_MESSAGE for each test
        msg = init_http_message();
        memset(buffer, 0, sizeof(buffer));
    }

    void teardown()
    {
        // Cleanup code for each test
        free_http_message(&msg);
    }
    
    void setup_request_message()
    {
        set_http_method_from_string("GET", &msg.start_line.request.method);
        strcpy(msg.start_line.request.request_target, "/index.html");
        set_http_protocol_from_string("HTTP/1.1", &msg.start_line.request.protocol);
        
        // Add some headers
        strcpy(msg.headers[0].key, "Host");
        strcpy(msg.headers[0].value, "example.com");
        strcpy(msg.headers[1].key, "User-Agent");
        strcpy(msg.headers[1].value, "TestAgent/1.0");
        msg.header_count = 2;
    }
    
    void setup_response_message()
    {
        set_http_protocol_from_string("HTTP/1.1", &msg.start_line.response.protocol);
        set_http_status_code_from_string("200", &msg.start_line.response.status_code);
        strcpy(msg.start_line.response.status_message, "OK");
        
        // Add some headers
        strcpy(msg.headers[0].key, "Content-Type");
        strcpy(msg.headers[0].value, "text/html");
        strcpy(msg.headers[1].key, "Content-Length");
        strcpy(msg.headers[1].value, "1024");
        msg.header_count = 2;
    }
};

TEST(BuildHeaderTests, ValidRequestMessage)
{
    setup_request_message();
    
    int result = build_header(&msg, REQUEST, buffer, sizeof(buffer));
    
    CHECK_EQUAL(0, result);
    
    // Check that the start line is correct
    CHECK(strstr(buffer, "GET /index.html HTTP/1.1\r\n") != NULL);
    
    // Check that headers are present
    CHECK(strstr(buffer, "Host: example.com\r\n") != NULL);
    CHECK(strstr(buffer, "User-Agent: TestAgent/1.0\r\n") != NULL);
    
    // Check that it ends with empty line (header terminator)
    CHECK(strstr(buffer, "\r\n\r\n") != NULL);
}

TEST(BuildHeaderTests, ValidResponseMessage)
{
    setup_response_message();
    
    int result = build_header(&msg, RESPONSE, buffer, sizeof(buffer));
    
    CHECK_EQUAL(0, result);
    
    // Check that the start line is correct
    CHECK(strstr(buffer, "HTTP/1.1 200 OK\r\n") != NULL);
    
    // Check that headers are present
    CHECK(strstr(buffer, "Content-Type: text/html\r\n") != NULL);
    CHECK(strstr(buffer, "Content-Length: 1024\r\n") != NULL);
    
    // Check that it ends with empty line (header terminator)
    CHECK(strstr(buffer, "\r\n\r\n") != NULL);
}

TEST(BuildHeaderTests, RequestWithNoHeaders)
{
    setup_request_message();
    msg.header_count = 0;
    
    int result = build_header(&msg, REQUEST, buffer, sizeof(buffer));
    
    CHECK_EQUAL(0, result);
    
    // Should have start line and terminating CRLF
    CHECK(strstr(buffer, "GET /index.html HTTP/1.1\r\n\r\n") != NULL);
}

TEST(BuildHeaderTests, ResponseWithNoHeaders)
{
    setup_response_message();
    msg.header_count = 0;
    
    int result = build_header(&msg, RESPONSE, buffer, sizeof(buffer));
    
    CHECK_EQUAL(0, result);
    
    // Should have start line and terminating CRLF
    CHECK(strstr(buffer, "HTTP/1.1 200 OK\r\n\r\n") != NULL);
}

TEST(BuildHeaderTests, NullMessage)
{
    int result = build_header(NULL, REQUEST, buffer, sizeof(buffer));
    CHECK_EQUAL(-1, result);
}

TEST(BuildHeaderTests, EmptyRequestMethod)
{
    setup_request_message();
    msg.start_line.request.method = HTTP_METHOD_UNKNOWN;
    
    int result = build_header(&msg, REQUEST, buffer, sizeof(buffer));
    CHECK_EQUAL(-3, result);
}

TEST(BuildHeaderTests, EmptyRequestTarget)
{
    setup_request_message();
    msg.start_line.request.request_target[0] = '\0';
    
    int result = build_header(&msg, REQUEST, buffer, sizeof(buffer));
    CHECK_EQUAL(-3, result);
}

TEST(BuildHeaderTests, EmptyRequestProtocol)
{
    setup_request_message();
    msg.start_line.request.protocol = HTTP_PROTOCOL_UNKNOWN;
    
    int result = build_header(&msg, REQUEST, buffer, sizeof(buffer));
    CHECK_EQUAL(-3, result);
}

TEST(BuildHeaderTests, EmptyResponseProtocol)
{
    setup_response_message();
    msg.start_line.response.protocol = HTTP_PROTOCOL_UNKNOWN;
    
    int result = build_header(&msg, RESPONSE, buffer, sizeof(buffer));
    CHECK_EQUAL(-3, result);
}

TEST(BuildHeaderTests, EmptyResponseStatusCode)
{
    setup_response_message();
    msg.start_line.response.status_code = HTTP_STATUS_CODE_UNKNOWN; // Invalid status code
    
    int result = build_header(&msg, RESPONSE, buffer, sizeof(buffer));
    CHECK_EQUAL(-3, result);
}

TEST(BuildHeaderTests, EmptyResponseStatusMessage)
{
    setup_response_message();
    msg.start_line.response.status_message[0] = '\0';
    
    int result = build_header(&msg, RESPONSE, buffer, sizeof(buffer));
    CHECK_EQUAL(-3, result);
}

TEST(BuildHeaderTests, UnknownMessageType)
{
    setup_request_message();
    
    int result = build_header(&msg, 999, buffer, sizeof(buffer));
    CHECK_EQUAL(-4, result);
}

TEST(BuildHeaderTests, BufferTooSmall)
{
    setup_request_message();
    char tiny_buffer[10];
    
    int result = build_header(&msg, REQUEST, tiny_buffer, sizeof(tiny_buffer));
    CHECK_EQUAL(-6, result); // Not enough space for final CRLF terminator
}

TEST(BuildHeaderTests, BufferJustTooSmallForTerminator)
{
    setup_request_message();
    msg.header_count = 0; // No headers to minimize space needed
    
    // Calculate exact space needed for start line + CRLF (without final CRLF)
    const char* expected_start = "GET /index.html HTTP/1.1\r\n";
    int start_len = strlen(expected_start);
    
    // Buffer with exactly space for start line but not final CRLF (need 2 more bytes)
    char limited_buffer[64];
    memset(limited_buffer, 0, sizeof(limited_buffer));
    
    // This should work since we have enough space
    int result = build_header(&msg, REQUEST, limited_buffer, start_len + 3);
    CHECK_EQUAL(0, result);
    
    // This should fail - not enough space for final CRLF
    result = build_header(&msg, REQUEST, limited_buffer, start_len + 1);
    CHECK_EQUAL(-6, result);
}

TEST(BuildHeaderTests, MaxHeaders)
{
    setup_request_message();
    
    // Fill up to maximum headers
    for (int i = 2; i < MAX_HEADERS; i++) {
        snprintf(msg.headers[i].key, MAX_HEADER_LENGTH, "Header%d", i);
        snprintf(msg.headers[i].value, MAX_HEADER_LENGTH, "Value%d", i);
    }
    msg.header_count = MAX_HEADERS;
    
    int result = build_header(&msg, REQUEST, buffer, sizeof(buffer));
    CHECK_EQUAL(0, result);
    
    // Verify some of the headers made it in
    CHECK(strstr(buffer, "Header10: Value10\r\n") != NULL);
    CHECK(strstr(buffer, "Header20: Value20\r\n") != NULL);
}

TEST(BuildHeaderTests, PostRequestWithContentLength)
{
    set_http_method_from_string("POST", &msg.start_line.request.method);
    strcpy(msg.start_line.request.request_target, "/api/users");
    set_http_protocol_from_string("HTTP/1.1", &msg.start_line.request.protocol);
    
    strcpy(msg.headers[0].key, "Content-Type");
    strcpy(msg.headers[0].value, "application/json");
    strcpy(msg.headers[1].key, "Content-Length");
    strcpy(msg.headers[1].value, "123");
    msg.header_count = 2;
    
    int result = build_header(&msg, REQUEST, buffer, sizeof(buffer));
    
    CHECK_EQUAL(0, result);
    CHECK(strstr(buffer, "POST /api/users HTTP/1.1\r\n") != NULL);
    CHECK(strstr(buffer, "Content-Type: application/json\r\n") != NULL);
    CHECK(strstr(buffer, "Content-Length: 123\r\n") != NULL);
}

TEST(BuildHeaderTests, ResponseWithMultiWordStatusMessage)
{
    set_http_protocol_from_string("HTTP/1.1", &msg.start_line.response.protocol);
    set_http_status_code_from_string("404", &msg.start_line.response.status_code);
    strcpy(msg.start_line.response.status_message, "Not Found");
    
    strcpy(msg.headers[0].key, "Content-Type");
    strcpy(msg.headers[0].value, "text/html");
    msg.header_count = 1;
    
    int result = build_header(&msg, RESPONSE, buffer, sizeof(buffer));
    
    CHECK_EQUAL(0, result);
    CHECK(strstr(buffer, "HTTP/1.1 404 Not Found\r\n") != NULL);
    CHECK(strstr(buffer, "Content-Type: text/html\r\n") != NULL);
}

TEST(BuildHeaderTests, HeadersWithSpecialCharacters)
{
    setup_request_message();
    
    strcpy(msg.headers[0].key, "X-Custom-Header");
    strcpy(msg.headers[0].value, "Value with spaces & symbols!");
    strcpy(msg.headers[1].key, "Authorization");
    strcpy(msg.headers[1].value, "Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9");
    msg.header_count = 2;
    
    int result = build_header(&msg, REQUEST, buffer, sizeof(buffer));
    
    CHECK_EQUAL(0, result);
    CHECK(strstr(buffer, "X-Custom-Header: Value with spaces & symbols!\r\n") != NULL);
    CHECK(strstr(buffer, "Authorization: Bearer eyJ0eXAiOiJKV1QiLCJhbGciOiJIUzI1NiJ9\r\n") != NULL);
}

TEST_GROUP(BuildAndSendHeadersTests)
{
    HTTP_MESSAGE msg;
    int mock_socket_fd;
    
    void setup()
    {
        // Initialize HTTP_MESSAGE for each test
        msg = init_http_message();
        // Use a mock socket fd (we can't easily test actual network sends in unit tests)
        // In real testing, you might use a pipe or mock framework
        mock_socket_fd = 42; // Arbitrary valid-looking fd
    }

    void teardown()
    {
        // Cleanup code for each test
        free_http_message(&msg);
        
        // Clear mock expectations
        mock().clear();
    }
    
    void setup_request_message()
    {
        set_http_method_from_string("GET", &msg.start_line.request.method);
        strcpy(msg.start_line.request.request_target, "/test");
        set_http_protocol_from_string("HTTP/1.1", &msg.start_line.request.protocol);
        
        strcpy(msg.headers[0].key, "Host");
        strcpy(msg.headers[0].value, "example.com");
        msg.header_count = 1;
    }
    
    void setup_response_message()
    {
        set_http_protocol_from_string("HTTP/1.1", &msg.start_line.response.protocol);
        set_http_status_code_from_string("200", &msg.start_line.response.status_code);
        strcpy(msg.start_line.response.status_message, "OK");
        
        strcpy(msg.headers[0].key, "Content-Type");
        strcpy(msg.headers[0].value, "text/plain");
        msg.header_count = 1;
    }
};

// Note: These tests use CppUTest's mocking framework with weak symbol override

TEST(BuildAndSendHeadersTests, NullMessage)
{
    int result = build_and_send_headers(NULL, mock_socket_fd, REQUEST);
    CHECK_EQUAL(-1, result);
}

TEST(BuildAndSendHeadersTests, InvalidSocketFd)
{
    setup_request_message();
    
    int result = build_and_send_headers(&msg, -1, REQUEST);
    CHECK_EQUAL(-1, result);
}

TEST(BuildAndSendHeadersTests, EmptyRequestMethod)
{
    setup_request_message();
    msg.start_line.request.method = HTTP_METHOD_UNKNOWN; // Make method empty
    
    int result = build_and_send_headers(&msg, mock_socket_fd, REQUEST);
    CHECK_EQUAL(-2, result); // Should fail in build_header step
}

TEST(BuildAndSendHeadersTests, EmptyResponseStatusCode)
{
    setup_response_message();
    msg.start_line.response.status_code = HTTP_STATUS_CODE_UNKNOWN; // Make status code empty
    
    int result = build_and_send_headers(&msg, mock_socket_fd, RESPONSE);
    CHECK_EQUAL(-2, result); // Should fail in build_header step
}

TEST(BuildAndSendHeadersTests, UnknownMessageType)
{
    setup_request_message();
    
    int result = build_and_send_headers(&msg, mock_socket_fd, 999);
    CHECK_EQUAL(-2, result); // Should fail in build_header step
}

TEST(BuildAndSendHeadersTests, SuccessfulRequestSend)
{
    setup_request_message();
    
    // Set up mock expectation for successful send
    mock().expectOneCall("send")
          .withParameter("sockfd", mock_socket_fd)
          .withParameter("len", 41) // Actual length of "GET /test HTTP/1.1\r\nHost: example.com\r\n\r\n"
          .withParameter("flags", 0)
          .withParameter("buf", "GET /test HTTP/1.1\r\nHost: example.com\r\n\r\n")
          .andReturnValue(41); // Return the full length sent
    
    int result = build_and_send_headers(&msg, mock_socket_fd, REQUEST);
    
    CHECK_EQUAL(0, result); // Should succeed
    mock().checkExpectations();
}

TEST(BuildAndSendHeadersTests, SuccessfulResponseSend)
{
    setup_response_message();
    
    // Set up mock expectation for successful send
    mock().expectOneCall("send")
          .withParameter("sockfd", mock_socket_fd)
          .withParameter("len", 45) // Actual length of response
          .withParameter("flags", 0)
          .withParameter("buf", "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n")
          .andReturnValue(45); // Return the full length sent
    
    int result = build_and_send_headers(&msg, mock_socket_fd, RESPONSE);
    
    CHECK_EQUAL(0, result); // Should succeed
    mock().checkExpectations();
}

TEST(BuildAndSendHeadersTests, SendFailureWithErrno)
{
    setup_request_message();
    
    // Set up mock expectation for send failure
    mock().expectOneCall("send")
          .withParameter("sockfd", mock_socket_fd)
          .withParameter("len", 41)
          .withParameter("flags", 0)
          .withParameter("buf", "GET /test HTTP/1.1\r\nHost: example.com\r\n\r\n")
          .andReturnValue(-1); // Return error
    
    // Set errno for the mock
    mock().setData("send_errno", EPIPE);
    
    int result = build_and_send_headers(&msg, mock_socket_fd, REQUEST);
    
    CHECK_EQUAL(-3, result); // Should return send failure error
    mock().checkExpectations();
}

TEST(BuildAndSendHeadersTests, PartialSendStillSucceeds)
{
    setup_request_message();
    
    // Mock partial send (returns fewer bytes than expected)
    // Note: Current implementation doesn't handle partial sends as failures
    mock().expectOneCall("send")
          .withParameter("sockfd", mock_socket_fd)
          .withParameter("len", 41)
          .withParameter("flags", 0)
          .withParameter("buf", "GET /test HTTP/1.1\r\nHost: example.com\r\n\r\n")
          .andReturnValue(10); // Much less than the full message
    
    int result = build_and_send_headers(&msg, mock_socket_fd, REQUEST);
    
    // Current implementation only checks for -1, so partial send still succeeds
    CHECK_EQUAL(0, result); // Should succeed since send() didn't return -1
    mock().checkExpectations();
}

TEST(BuildAndSendHeadersTests, RequestWithMultipleHeaders)
{
    set_http_method_from_string("POST", &msg.start_line.request.method);
    strcpy(msg.start_line.request.request_target, "/api/data");
    set_http_protocol_from_string("HTTP/1.1", &msg.start_line.request.protocol);
    
    strcpy(msg.headers[0].key, "Host");
    strcpy(msg.headers[0].value, "api.example.com");
    strcpy(msg.headers[1].key, "Content-Type");
    strcpy(msg.headers[1].value, "application/json");
    strcpy(msg.headers[2].key, "Content-Length");
    strcpy(msg.headers[2].value, "123");
    strcpy(msg.headers[3].key, "Authorization");
    strcpy(msg.headers[3].value, "Bearer token123");
    msg.header_count = 4;
    
    // Expected message content
    const char* expected_msg = "POST /api/data HTTP/1.1\r\n"
                              "Host: api.example.com\r\n"
                              "Content-Type: application/json\r\n"
                              "Content-Length: 123\r\n"
                              "Authorization: Bearer token123\r\n\r\n";
    
    // Set up mock expectation for successful send
    mock().expectOneCall("send")
          .withParameter("sockfd", mock_socket_fd)
          .withParameter("len", (int)strlen(expected_msg))
          .withParameter("flags", 0)
          .withParameter("buf", expected_msg)
          .andReturnValue((int)strlen(expected_msg));
    
    int result = build_and_send_headers(&msg, mock_socket_fd, REQUEST);
    
    CHECK_EQUAL(0, result); // Should succeed
    mock().checkExpectations();
}

TEST(BuildAndSendHeadersTests, ResponseWithCustomHeaders)
{
    set_http_protocol_from_string("HTTP/1.1", &msg.start_line.response.protocol);
    set_http_status_code_from_string("404", &msg.start_line.response.status_code);
    strcpy(msg.start_line.response.status_message, "Not Found");
    
    strcpy(msg.headers[0].key, "Content-Type");
    strcpy(msg.headers[0].value, "text/html");
    strcpy(msg.headers[1].key, "Cache-Control");
    strcpy(msg.headers[1].value, "no-cache");
    strcpy(msg.headers[2].key, "Server");
    strcpy(msg.headers[2].value, "TestServer/1.0");
    msg.header_count = 3;
    
    // Expected message content
    const char* expected_msg = "HTTP/1.1 404 Not Found\r\n"
                              "Content-Type: text/html\r\n"
                              "Cache-Control: no-cache\r\n"
                              "Server: TestServer/1.0\r\n\r\n";
    
    // Set up mock expectation for successful send
    mock().expectOneCall("send")
          .withParameter("sockfd", mock_socket_fd)
          .withParameter("len", (int)strlen(expected_msg))
          .withParameter("flags", 0)
          .withParameter("buf", expected_msg)
          .andReturnValue((int)strlen(expected_msg));
    
    int result = build_and_send_headers(&msg, mock_socket_fd, RESPONSE);
    
    CHECK_EQUAL(0, result); // Should succeed
    mock().checkExpectations();
}

TEST(BuildAndSendHeadersTests, VerifyExactSentContent)
{
    setup_request_message();
    
    // Expected exact content
    const char* expected_msg = "GET /test HTTP/1.1\r\nHost: example.com\r\n\r\n";
    
    // Set up mock expectation for successful send
    mock().expectOneCall("send")
          .withParameter("sockfd", mock_socket_fd)
          .withParameter("len", (int)strlen(expected_msg))
          .withParameter("flags", 0)
          .withParameter("buf", expected_msg)
          .andReturnValue((int)strlen(expected_msg));
    
    int result = build_and_send_headers(&msg, mock_socket_fd, REQUEST);
    
    CHECK_EQUAL(0, result);
    mock().checkExpectations();
}
