#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <string.h>

extern "C" {
    #include "http_parser.h"
}

TEST_GROUP(ParseHeaderTests)
{
    HTTP_HEADER header;
    
    void setup()
    {
        // Reset structures before each test
        memset(&header, 0, sizeof(HTTP_HEADER));
    }

    void teardown()
    {
        // Cleanup code for each test
    }
};

TEST_GROUP(ParseStartLineTests)
{
    HTTP_START_LINE start_line;
    
    void setup()
    {
        // Reset structures before each test
        memset(&start_line, 0, sizeof(HTTP_START_LINE));
    }

    void teardown()
    {
        // Cleanup code for each test
    }
};

// Tests for parse_header function
TEST(ParseHeaderTests, ParseHeaderValidInput)
{
    char line[] = "Content-Type: application/json\r\n";
    int result = parse_header(line, &header);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("Content-Type", header.key);
    STRCMP_EQUAL("application/json", header.value);
}

TEST(ParseHeaderTests, ParseHeaderWithSpaces)
{
    char line[] = "Authorization: Bearer token123\r\n";
    int result = parse_header(line, &header);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("Authorization", header.key);
    STRCMP_EQUAL("Bearer token123", header.value);
}

TEST(ParseHeaderTests, ParseHeaderWithMultipleSpaces)
{
    char line[] = "User-Agent:   Mozilla/5.0 (Windows NT 10.0)\r\n";
    int result = parse_header(line, &header);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("User-Agent", header.key);
    STRCMP_EQUAL("Mozilla/5.0 (Windows NT 10.0)", header.value);
}

TEST(ParseHeaderTests, ParseHeaderNullLine)
{
    int result = parse_header(NULL, &header);
    CHECK_EQUAL(-1, result);
}

TEST(ParseHeaderTests, ParseHeaderEmptyLine)
{
    char line[] = "";
    int result = parse_header(line, &header);
    CHECK_EQUAL(-1, result);
}

TEST(ParseHeaderTests, ParseHeaderNullHeader)
{
    char line[] = "Content-Length: 123\r\n";
    int result = parse_header(line, NULL);
    CHECK_EQUAL(-2, result);
}

TEST(ParseHeaderTests, ParseHeaderMalformedNoColon)
{
    char line[] = "InvalidHeaderLine\r\n";
    int result = parse_header(line, &header);
    CHECK_EQUAL(-4, result);
}

TEST(ParseHeaderTests, ParseHeaderMalformedOnlyKey)
{
    char line[] = "Content-Type:\r\n";
    int result = parse_header(line, &header);
    CHECK_EQUAL(-4, result);
}

TEST(ParseHeaderTests, ParseHeaderLongKey)
{
    // Create a header with a very long key to test buffer limits
    char long_key[300];
    memset(long_key, 'A', 299);
    long_key[299] = '\0';
    
    char line[400];
    snprintf(line, sizeof(line), "%s: value\r\n", long_key);
    
    int result = parse_header(line, &header);
    CHECK_EQUAL(0, result);
    // Key should be truncated to MAX_HEADER_LENGTH - 1
    CHECK_EQUAL(MAX_HEADER_LENGTH - 1, strlen(header.key));
}

TEST(ParseHeaderTests, ParseHeaderLongValue)
{
    // Create a header with a very long value to test buffer limits
    char long_value[300];
    memset(long_value, 'B', 299);
    long_value[299] = '\0';
    
    char line[400];
    snprintf(line, sizeof(line), "Custom-Header: %s\r\n", long_value);
    
    int result = parse_header(line, &header);
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("Custom-Header", header.key);
    // Value should be truncated to MAX_HEADER_LENGTH - 1
    CHECK_EQUAL(MAX_HEADER_LENGTH - 1, strlen(header.value));
}

// Tests for parse_start_line function
TEST(ParseStartLineTests, ParseStartLineValidGetRequest)
{
    char line[] = "GET /index.html HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", start_line.request.method);
    STRCMP_EQUAL("/index.html", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.http_version);
}

TEST(ParseStartLineTests, ParseStartLineValidPostRequest)
{
    char line[] = "POST /api/users HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("POST", start_line.request.method);
    STRCMP_EQUAL("/api/users", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.http_version);
}

TEST(ParseStartLineTests, ParseStartLineWithQueryString)
{
    char line[] = "GET /search?q=test&page=1 HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", start_line.request.method);
    STRCMP_EQUAL("/search?q=test&page=1", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.http_version);
}

TEST(ParseStartLineTests, ParseStartLineNullLine)
{
    int result = parse_start_line(NULL, &start_line);
    CHECK_EQUAL(-1, result);
}

TEST(ParseStartLineTests, ParseStartLineEmptyLine)
{
    char line[] = "";
    int result = parse_start_line(line, &start_line);
    CHECK_EQUAL(-1, result);
}

TEST(ParseStartLineTests, ParseStartLineNullStartLine)
{
    char line[] = "GET / HTTP/1.1\r\n";
    int result = parse_start_line(line, NULL);
    CHECK_EQUAL(-2, result);
}

TEST(ParseStartLineTests, ParseStartLineMalformedTwoComponents)
{
    char line[] = "GET /index.html\r\n";
    int result = parse_start_line(line, &start_line);
    CHECK_EQUAL(-4, result);
}

TEST(ParseStartLineTests, ParseStartLineMalformedOneComponent)
{
    char line[] = "GET\r\n";
    int result = parse_start_line(line, &start_line);
    CHECK_EQUAL(-4, result);
}

TEST(ParseStartLineTests, ParseStartLineWithExtraSpaces)
{
    char line[] = "GET    /index.html    HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", start_line.request.method);
    STRCMP_EQUAL("/index.html", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.http_version);
}
