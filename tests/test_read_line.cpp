#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <string.h>

extern "C" {
    #include "http_parser.h"
}

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

// Test reading a simple line with just LF (should fail - HTTP requires CRLF)
TEST(ReadLineTests, ReadSimpleLineWithNewlineShouldFail)
{
    const char* input = "Hello World\nNext line";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since only LF is not valid in HTTP
    POINTERS_EQUAL(NULL, next);
}

// Test reading a line with carriage return and newline (should succeed)
TEST(ReadLineTests, ReadLineWithCRLF)
{
    const char* input = "HTTP/1.1 200 OK\r\nContent-Type: text/html";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("HTTP/1.1 200 OK", buffer);
    STRCMP_EQUAL("Content-Type: text/html", next);
}

// Test reading a line without CRLF (end of string) - should fail
TEST(ReadLineTests, ReadLineWithoutCRLF)
{
    const char* input = "Last line without CRLF";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since no CRLF found
    POINTERS_EQUAL(NULL, next);
}

// Test reading an empty line with just LF (should fail)
TEST(ReadLineTests, ReadEmptyLineWithLFShouldFail)
{
    const char* input = "\nNext line";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since only LF is not valid
    POINTERS_EQUAL(NULL, next);
}

// Test reading an empty line with CRLF (should succeed)
TEST(ReadLineTests, ReadEmptyLineWithCRLF)
{
    const char* input = "\r\nNext line";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("", buffer);
    STRCMP_EQUAL("Next line", next);
}

// Test with NULL input
TEST(ReadLineTests, HandleNullInput)
{
    const char* next = read_line(NULL, buffer, sizeof(buffer));
    
    POINTERS_EQUAL(NULL, next);
}

// Test with empty string input
TEST(ReadLineTests, HandleEmptyStringInput)
{
    const char* input = "";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    POINTERS_EQUAL(NULL, next);
}

// Test buffer size limit with CRLF
TEST(ReadLineTests, HandleBufferSizeLimitWithCRLF)
{
    const char* input = "This is a very long line\r\nNext line";
    char small_buffer[10];
    const char* next = read_line(input, small_buffer, sizeof(small_buffer));
    
    // Should only copy 9 characters (buffer size - 1) and null terminate
    STRCMP_EQUAL("This is a", small_buffer);
    // Since we didn't find CRLF within buffer limit, should return NULL
    POINTERS_EQUAL(NULL, next);
}

// Test multiple consecutive CRLF sequences
TEST(ReadLineTests, HandleConsecutiveCRLF)
{
    const char* input = "First line\r\n\r\n\r\nFourth line\r\n";
    const char* next1 = read_line(input, buffer, sizeof(buffer));
    STRCMP_EQUAL("First line", buffer);
    
    const char* next2 = read_line(next1, buffer, sizeof(buffer));
    STRCMP_EQUAL("", buffer);
    
    const char* next3 = read_line(next2, buffer, sizeof(buffer));
    STRCMP_EQUAL("", buffer);
    
    read_line(next3, buffer, sizeof(buffer));
    STRCMP_EQUAL("Fourth line", buffer);
}

// Test line with only carriage return (no newline) - should fail
TEST(ReadLineTests, HandleCarriageReturnOnlyShouldFail)
{
    const char* input = "Line with CR only\rMore text";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since no CRLF found
    POINTERS_EQUAL(NULL, next);
}

// Test reading HTTP-style headers (should succeed with CRLF)
TEST(ReadLineTests, ReadHTTPHeaders)
{
    const char* input = "GET /path HTTP/1.1\r\nHost: example.com\r\nUser-Agent: TestAgent\r\n\r\nBody content";
    
    // Read request line
    const char* next1 = read_line(input, buffer, sizeof(buffer));
    STRCMP_EQUAL("GET /path HTTP/1.1", buffer);
    
    // Read Host header
    const char* next2 = read_line(next1, buffer, sizeof(buffer));
    STRCMP_EQUAL("Host: example.com", buffer);
    
    // Read User-Agent header
    const char* next3 = read_line(next2, buffer, sizeof(buffer));
    STRCMP_EQUAL("User-Agent: TestAgent", buffer);
    
    // Read empty line (header separator)
    const char* next4 = read_line(next3, buffer, sizeof(buffer));
    STRCMP_EQUAL("", buffer);
    
    // Body content has no CRLF, so should fail
    const char* next5 = read_line(next4, buffer, sizeof(buffer));
    POINTERS_EQUAL(NULL, next5);
}

// Test edge case: single character line with CRLF
TEST(ReadLineTests, SingleCharacterLineWithCRLF)
{
    const char* input = "A\r\nB";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("A", buffer);
    STRCMP_EQUAL("B", next);
}

// Test edge case: single character line with only LF (should fail)
TEST(ReadLineTests, SingleCharacterLineWithLFShouldFail)
{
    const char* input = "A\nB";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    // Should return NULL since only LF is not valid
    POINTERS_EQUAL(NULL, next);
}

// Test edge case: buffer size of 1 (only room for null terminator)
TEST(ReadLineTests, MinimalBufferSize)
{
    const char* input = "Hello\r\nWorld";
    char tiny_buffer[1];
    const char* next = read_line(input, tiny_buffer, sizeof(tiny_buffer));
    
    STRCMP_EQUAL("", tiny_buffer); // Should be empty string
    // Should return NULL since no room to find CRLF within buffer
    POINTERS_EQUAL(NULL, next);
}

// Test buffer too small to find CRLF
TEST(ReadLineTests, PartialCRLFAtBufferBoundary)
{
    const char* input = "123456789\r\nNext";
    char small_buffer[9]; // Can only fit "12345678" (8 chars + null), CRLF is beyond buffer limit
    const char* next = read_line(input, small_buffer, sizeof(small_buffer));
    
    // Should copy "12345678" and return NULL since CRLF is beyond what we can check
    STRCMP_EQUAL("12345678", small_buffer);
    POINTERS_EQUAL(NULL, next);
}

// Test CRLF at exact buffer boundary
TEST(ReadLineTests, CRLFAtExactBufferBoundary)
{
    const char* input = "12345\r\nNext";
    char buffer_exact[8]; // Exactly fits "12345\r\n" + null terminator
    const char* next = read_line(input, buffer_exact, sizeof(buffer_exact));
    
    // Should successfully read the line
    STRCMP_EQUAL("12345", buffer_exact);
    STRCMP_EQUAL("Next", next);
}

