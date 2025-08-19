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

TEST_GROUP(GetHeaderValueTests)
{
    HTTP_HEADER header1, header2, header3, header4;
    const HTTP_HEADER* header_array[4];
    
    void setup()
    {
        // Setup test headers
        strcpy(header1.key, "Content-Type");
        strcpy(header1.value, "application/json");
        
        strcpy(header2.key, "Authorization");
        strcpy(header2.value, "Bearer token123");
        
        strcpy(header3.key, "User-Agent");
        strcpy(header3.value, "Mozilla/5.0");
        
        strcpy(header4.key, "Content-Length");
        strcpy(header4.value, "1024");
        
        header_array[0] = &header1;
        header_array[1] = &header2;
        header_array[2] = &header3;
        header_array[3] = &header4;
    }

    void teardown()
    {
        // Cleanup code for each test
    }
};

TEST_GROUP(ReadLineTests)
{
    char buffer[1024];
    
    void setup()
    {
        // Clear the buffer before each test
        memset(buffer, 0, sizeof(buffer));
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
    int result = parse_start_line(line, &start_line, REQUEST);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", start_line.request.method);
    STRCMP_EQUAL("/index.html", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.http_version);
}

TEST(ParseStartLineTests, ParseStartLineValidPostRequest)
{
    char line[] = "POST /api/users HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line, REQUEST);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("POST", start_line.request.method);
    STRCMP_EQUAL("/api/users", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.http_version);
}

TEST(ParseStartLineTests, ParseStartLineWithQueryString)
{
    char line[] = "GET /search?q=test&page=1 HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line, REQUEST);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", start_line.request.method);
    STRCMP_EQUAL("/search?q=test&page=1", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.http_version);
}

TEST(ParseStartLineTests, ParseStartLineNullLine)
{
    int result = parse_start_line(NULL, &start_line, REQUEST);
    CHECK_EQUAL(-1, result);
}

TEST(ParseStartLineTests, ParseStartLineEmptyLine)
{
    char line[] = "";
    int result = parse_start_line(line, &start_line, REQUEST);
    CHECK_EQUAL(-1, result);
}

TEST(ParseStartLineTests, ParseStartLineNullStartLine)
{
    char line[] = "GET / HTTP/1.1\r\n";
    int result = parse_start_line(line, NULL, REQUEST);
    CHECK_EQUAL(-2, result);
}

TEST(ParseStartLineTests, ParseStartLineMalformedTwoComponents)
{
    char line[] = "GET /index.html\r\n";
    int result = parse_start_line(line, &start_line, REQUEST);
    CHECK_EQUAL(-5, result);
}

TEST(ParseStartLineTests, ParseStartLineMalformedOneComponent)
{
    char line[] = "GET\r\n";
    int result = parse_start_line(line, &start_line, REQUEST);
    CHECK_EQUAL(-5, result);
}

TEST(ParseStartLineTests, ParseStartLineWithExtraSpaces)
{
    char line[] = "GET    /index.html    HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line, REQUEST);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", start_line.request.method);
    STRCMP_EQUAL("/index.html", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.http_version);
}

// Response start line tests
TEST(ParseStartLineTests, ParseStartLineValidResponse)
{
    char line[] = "HTTP/1.1 200 OK\r\n";
    int result = parse_start_line(line, &start_line, RESPONSE);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("HTTP/1.1", start_line.response.protocol);
    STRCMP_EQUAL("200", start_line.response.status_code);
    STRCMP_EQUAL("OK", start_line.response.status_message);
}

TEST(ParseStartLineTests, ParseStartLineValidResponseWithMultiWordStatus)
{
    char line[] = "HTTP/1.1 404 Not Found\r\n";
    int result = parse_start_line(line, &start_line, RESPONSE);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("HTTP/1.1", start_line.response.protocol);
    STRCMP_EQUAL("404", start_line.response.status_code);
    STRCMP_EQUAL("Not Found", start_line.response.status_message);
}

TEST(ParseStartLineTests, ParseStartLineResponseNullLine)
{
    int result = parse_start_line(NULL, &start_line, RESPONSE);
    CHECK_EQUAL(-1, result);
}

TEST(ParseStartLineTests, ParseStartLineResponseMalformedTwoComponents)
{
    char line[] = "HTTP/1.1 200\r\n";
    int result = parse_start_line(line, &start_line, RESPONSE);
    CHECK_EQUAL(-5, result);
}

// Tests for get_header_value function
TEST(GetHeaderValueTests, GetHeaderValueValidKey)
{
    const char* value = get_header_value(header_array, 4, "Content-Type");
    STRCMP_EQUAL("application/json", value);
}

TEST(GetHeaderValueTests, GetHeaderValueCaseInsensitive)
{
    const char* value = get_header_value(header_array, 4, "content-type");
    STRCMP_EQUAL("application/json", value);
    
    value = get_header_value(header_array, 4, "AUTHORIZATION");
    STRCMP_EQUAL("Bearer token123", value);
    
    value = get_header_value(header_array, 4, "user-agent");
    STRCMP_EQUAL("Mozilla/5.0", value);
}

TEST(GetHeaderValueTests, GetHeaderValueKeyNotFound)
{
    const char* value = get_header_value(header_array, 4, "Non-Existent-Header");
    POINTERS_EQUAL(NULL, value);
}

TEST(GetHeaderValueTests, GetHeaderValueNullHeaderArray)
{
    const char* value = get_header_value(NULL, 4, "Content-Type");
    POINTERS_EQUAL(NULL, value);
}

TEST(GetHeaderValueTests, GetHeaderValueNullKey)
{
    const char* value = get_header_value(header_array, 4, NULL);
    POINTERS_EQUAL(NULL, value);
}

TEST(GetHeaderValueTests, GetHeaderValueZeroLength)
{
    const char* value = get_header_value(header_array, 0, "Content-Type");
    POINTERS_EQUAL(NULL, value);
}

TEST(GetHeaderValueTests, GetHeaderValueEmptyKey)
{
    const char* value = get_header_value(header_array, 4, "");
    POINTERS_EQUAL(NULL, value);
}

TEST(GetHeaderValueTests, GetHeaderValueFirstHeader)
{
    const char* value = get_header_value(header_array, 4, "Content-Type");
    STRCMP_EQUAL("application/json", value);
}

TEST(GetHeaderValueTests, GetHeaderValueLastHeader)
{
    const char* value = get_header_value(header_array, 4, "Content-Length");
    STRCMP_EQUAL("1024", value);
}

TEST(GetHeaderValueTests, GetHeaderValueMiddleHeader)
{
    const char* value = get_header_value(header_array, 4, "Authorization");
    STRCMP_EQUAL("Bearer token123", value);
    
    value = get_header_value(header_array, 4, "User-Agent");
    STRCMP_EQUAL("Mozilla/5.0", value);
}

TEST(GetHeaderValueTests, GetHeaderValueSingleHeaderArray)
{
    const HTTP_HEADER* single_header_array[1] = { &header1 };
    const char* value = get_header_value(single_header_array, 1, "Content-Type");
    STRCMP_EQUAL("application/json", value);
    
    value = get_header_value(single_header_array, 1, "Authorization");
    POINTERS_EQUAL(NULL, value);
}

// Tests for read_crlf_line function
TEST(ReadLineTests, ReadSimpleLineWithNewlineShouldFail)
{
    const char* input = "Hello World\nNext line";
    const char* next = read_crlf_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since only LF is not valid in HTTP
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, ReadLineWithCRLF)
{
    const char* input = "HTTP/1.1 200 OK\r\nContent-Type: text/html";
    const char* next = read_crlf_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("HTTP/1.1 200 OK", buffer);
    STRCMP_EQUAL("Content-Type: text/html", next);
}

TEST(ReadLineTests, ReadLineWithoutCRLF)
{
    const char* input = "Last line without CRLF";
    const char* next = read_crlf_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since no CRLF found
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, ReadEmptyLineWithLFShouldFail)
{
    const char* input = "\nNext line";
    const char* next = read_crlf_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since only LF is not valid
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, ReadEmptyLineWithCRLF)
{
    const char* input = "\r\nNext line";
    const char* next = read_crlf_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("", buffer);
    STRCMP_EQUAL("Next line", next);
}

TEST(ReadLineTests, HandleNullInput)
{
    const char* next = read_crlf_line(NULL, buffer, sizeof(buffer));
    
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, HandleEmptyStringInput)
{
    const char* input = "";
    const char* next = read_crlf_line(input, buffer, sizeof(buffer));
    
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, HandleBufferSizeLimitWithCRLF)
{
    const char* input = "This is a very long line\r\nNext line";
    char small_buffer[10];
    const char* next = read_crlf_line(input, small_buffer, sizeof(small_buffer));
    
    // Should only copy 9 characters (buffer size - 1) and null terminate
    STRCMP_EQUAL("This is a", small_buffer);
    // Since we didn't find CRLF within buffer limit, should return NULL
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, HandleConsecutiveCRLF)
{
    const char* input = "First line\r\n\r\n\r\nFourth line\r\n";
    const char* next1 = read_crlf_line(input, buffer, sizeof(buffer));
    STRCMP_EQUAL("First line", buffer);
    
    const char* next2 = read_crlf_line(next1, buffer, sizeof(buffer));
    STRCMP_EQUAL("", buffer);
    
    const char* next3 = read_crlf_line(next2, buffer, sizeof(buffer));
    STRCMP_EQUAL("", buffer);
    
    read_crlf_line(next3, buffer, sizeof(buffer));
    STRCMP_EQUAL("Fourth line", buffer);
}

TEST(ReadLineTests, HandleCarriageReturnOnlyShouldFail)
{
    const char* input = "Line with CR only\rMore text";
    const char* next = read_crlf_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since no CRLF found
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, ReadHTTPHeaders)
{
    const char* input = "GET /path HTTP/1.1\r\nHost: example.com\r\nUser-Agent: TestAgent\r\n\r\nBody content";
    
    // Read request line
    const char* next1 = read_crlf_line(input, buffer, sizeof(buffer));
    STRCMP_EQUAL("GET /path HTTP/1.1", buffer);
    
    // Read Host header
    const char* next2 = read_crlf_line(next1, buffer, sizeof(buffer));
    STRCMP_EQUAL("Host: example.com", buffer);
    
    // Read User-Agent header
    const char* next3 = read_crlf_line(next2, buffer, sizeof(buffer));
    STRCMP_EQUAL("User-Agent: TestAgent", buffer);
    
    // Read empty line (header separator)
    const char* next4 = read_crlf_line(next3, buffer, sizeof(buffer));
    STRCMP_EQUAL("", buffer);
    
    // Body content has no CRLF, so should fail
    const char* next5 = read_crlf_line(next4, buffer, sizeof(buffer));
    POINTERS_EQUAL(NULL, next5);
}

TEST(ReadLineTests, SingleCharacterLineWithCRLF)
{
    const char* input = "A\r\nB";
    const char* next = read_crlf_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("A", buffer);
    STRCMP_EQUAL("B", next);
}

TEST(ReadLineTests, SingleCharacterLineWithLFShouldFail)
{
    const char* input = "A\nB";
    const char* next = read_crlf_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since only LF is not valid
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, MinimalBufferSize)
{
    const char* input = "Hello\r\nWorld";
    char tiny_buffer[1];
    const char* next = read_crlf_line(input, tiny_buffer, sizeof(tiny_buffer));
    
    STRCMP_EQUAL("", tiny_buffer); // Should be empty string
    // Should return NULL since no room to find CRLF within buffer
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, PartialCRLFAtBufferBoundary)
{
    const char* input = "123456789\r\nNext";
    char small_buffer[9]; // Can only fit "12345678" (8 chars + null), CRLF is beyond buffer limit
    const char* next = read_crlf_line(input, small_buffer, sizeof(small_buffer));
    
    // Should copy "12345678" and return NULL since CRLF is beyond what we can check
    STRCMP_EQUAL("12345678", small_buffer);
    POINTERS_EQUAL(NULL, next);
}

TEST(ReadLineTests, CRLFAtExactBufferBoundary)
{
    const char* input = "12345\r\nNext";
    char buffer_exact[8]; // Exactly fits "12345\r\n" + null terminator
    const char* next = read_crlf_line(input, buffer_exact, sizeof(buffer_exact));
    
    // Should successfully read the line
    STRCMP_EQUAL("12345", buffer_exact);
    STRCMP_EQUAL("Next", next);
}
