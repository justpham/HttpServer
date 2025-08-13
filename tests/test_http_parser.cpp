#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <string.h>

extern "C" {
    #include "http_parser.h"
}

TEST_GROUP(HttpParserTests)
{
    HTTP_REQUEST request;
    
    void setup()
    {
        // Clear the request structure before each test
        memset(&request, 0, sizeof(HTTP_REQUEST));
    }

    void teardown()
    {
        // Cleanup code for each test
    }
};

TEST(HttpParserTests, ParseSimpleGetRequest)
{
    const char* raw_request = 
        "GET /index.html HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: TestAgent/1.0\r\n"
        "\r\n";
    
    int result = parse_request(raw_request, &request);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", request.method);
    STRCMP_EQUAL("/index.html", request.request_target);
    STRCMP_EQUAL("HTTP/1.1", request.http_version);
    CHECK_EQUAL(2, request.header_count);
    CHECK_EQUAL(0, request.body_length);
}

TEST(HttpParserTests, ParsePostRequestWithBody)
{
    const char* raw_request = 
        "POST /api/data HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 24\r\n"
        "\r\n"
        "{\"name\":\"test\",\"id\":123}";
    
    int result = parse_request(raw_request, &request);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("POST", request.method);
    STRCMP_EQUAL("/api/data", request.request_target);
    STRCMP_EQUAL("HTTP/1.1", request.http_version);
    CHECK_EQUAL(3, request.header_count);
    CHECK_EQUAL(24, request.content_length);
    CHECK_EQUAL(24, request.body_length);
    STRCMP_EQUAL("{\"name\":\"test\",\"id\":123}", request.body);
}

TEST(HttpParserTests, ParseRequestWithMultipleHeaders)
{
    const char* raw_request = 
        "GET /test HTTP/1.1\r\n"
        "Host: localhost:8080\r\n"
        "User-Agent: Mozilla/5.0\r\n"
        "Accept: text/html,application/xhtml+xml\r\n"
        "Accept-Language: en-US,en;q=0.9\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    
    int result = parse_request(raw_request, &request);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", request.method);
    STRCMP_EQUAL("/test", request.request_target);
    CHECK_EQUAL(5, request.header_count);
}

TEST(HttpParserTests, GetHeaderValue)
{
    const char* raw_request = 
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Type: text/html\r\n"
        "Authorization: Bearer token123\r\n"
        "\r\n";
    
    int result = parse_request(raw_request, &request);
    CHECK_EQUAL(0, result);
    
    const char* host = get_header_value(&request, "Host");
    const char* content_type = get_header_value(&request, "Content-Type");
    const char* auth = get_header_value(&request, "Authorization");
    const char* missing = get_header_value(&request, "X-Missing-Header");
    
    STRCMP_EQUAL("example.com", host);
    STRCMP_EQUAL("text/html", content_type);
    STRCMP_EQUAL("Bearer token123", auth);
    CHECK(missing == NULL);
}

TEST(HttpParserTests, GetHeaderValueCaseInsensitive)
{
    const char* raw_request = 
        "GET / HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "content-type: application/json\r\n"
        "\r\n";
    
    int result = parse_request(raw_request, &request);
    CHECK_EQUAL(0, result);
    
    // Test case insensitive header lookup
    const char* value1 = get_header_value(&request, "Content-Type");
    const char* value2 = get_header_value(&request, "content-type");
    const char* value3 = get_header_value(&request, "CONTENT-TYPE");
    
    STRCMP_EQUAL("application/json", value1);
    STRCMP_EQUAL("application/json", value2);
    STRCMP_EQUAL("application/json", value3);
}

TEST(HttpParserTests, ParseInvalidMethod)
{
    const char* raw_request = 
        "INVALID_METHOD_THAT_IS_TOO_LONG /path HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";
    
    int result = parse_request(raw_request, &request);
    
    // Should return error code for invalid/too long method
    CHECK_EQUAL(-1, result);
}

TEST(HttpParserTests, ParseMalformedRequestLine)
{
    const char* raw_request = 
        "GET\r\n"  // Missing target and version
        "Host: example.com\r\n"
        "\r\n";
    
    int result = parse_request(raw_request, &request);
    
    // Should return error code for malformed request line
    CHECK_EQUAL(-6, result);
}

TEST(HttpParserTests, ParseRequestWithoutHeaders)
{
    const char* raw_request = 
        "GET / HTTP/1.1\r\n"
        "\r\n";  // No headers, just empty line
    
    int result = parse_request(raw_request, &request);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", request.method);
    STRCMP_EQUAL("/", request.request_target);
    STRCMP_EQUAL("HTTP/1.1", request.http_version);
    CHECK_EQUAL(0, request.header_count);
    CHECK_EQUAL(0, request.body_length);
}

TEST(HttpParserTests, ParseRequestWithContentLengthMismatch)
{
    const char* raw_request = 
        "POST /data HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Content-Length: 10\r\n"  // Claims 10 bytes
        "\r\n"
        "short";  // But only 5 bytes actual body
    
    int result = parse_request(raw_request, &request);
    
    // Should handle content length mismatch appropriately
    // (behavior depends on implementation - might be error or partial parse)
    CHECK_EQUAL(0, result);
    CHECK_EQUAL(5, request.body_length);  // Actual body length
    CHECK_EQUAL(10, request.content_length);  // Parsed Content-Length header
}

TEST(HttpParserTests, ParseEmptyRequest)
{
    const char* raw_request = "";
    
    int result = parse_request(raw_request, &request);
    
    // Should return error for empty request
    CHECK(result != 0);
}

TEST(HttpParserTests, ParseNullRequest)
{
    int result = parse_request(NULL, &request);
    
    // Should return error for null input
    CHECK(result != 0);
}

TEST(HttpParserTests, ParseInvalidHttpVersion)
{
    const char* raw_request = 
        "GET / HTTP/2.0\r\n"
        "Host: example.com\r\n"
        "\r\n";
    
    int result = parse_request(raw_request, &request);
    
    // Should return error for unsupported HTTP version
    CHECK_EQUAL(-5, result);
}

TEST(HttpParserTests, ParseInvalidRequestTarget)
{
    const char* raw_request = 
        "GET invalid-target HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";
    
    int result = parse_request(raw_request, &request);
    
    // Should return error for invalid request target format
    CHECK_EQUAL(-3, result);
}

TEST(HttpParserTests, ParseValidAbsoluteForm)
{
    const char* raw_request = 
        "GET https://example.com/path HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "\r\n";
    
    int result = parse_request(raw_request, &request);
    
    // Should successfully parse absolute form URL
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", request.method);
    STRCMP_EQUAL("https://example.com/path", request.request_target);
    STRCMP_EQUAL("HTTP/1.1", request.http_version);
}
