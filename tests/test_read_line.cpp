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

// Test reading a simple line with newline
TEST(ReadLineTests, ReadSimpleLineWithNewline)
{
    const char* input = "Hello World\nNext line";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("Hello World", buffer);
    STRCMP_EQUAL("Next line", next);
}

// Test reading a line with carriage return and newline
TEST(ReadLineTests, ReadLineWithCRLF)
{
    const char* input = "HTTP/1.1 200 OK\r\nContent-Type: text/html";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("HTTP/1.1 200 OK", buffer);
    STRCMP_EQUAL("Content-Type: text/html", next);
}

// Test reading a line without newline (end of string)
TEST(ReadLineTests, ReadLineWithoutNewline)
{
    const char* input = "Last line without newline";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("Last line without newline", buffer);
    STRCMP_EQUAL("", next); // Should point to null terminator
}

// Test reading an empty line
TEST(ReadLineTests, ReadEmptyLine)
{
    const char* input = "\nNext line";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("", buffer);
    STRCMP_EQUAL("Next line", next);
}

// Test reading an empty line with CRLF
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

// Test buffer size limit
TEST(ReadLineTests, HandleBufferSizeLimit)
{
    const char* input = "This is a very long line that should be truncated\nNext line";
    char small_buffer[10];
    const char* next = read_line(input, small_buffer, sizeof(small_buffer));
    
    // Should only copy 9 characters (buffer size - 1) and null terminate
    STRCMP_EQUAL("This is a", small_buffer);
    // When buffer is too small, function stops at buffer limit, not at newline
    // So it returns pointer to the 10th character (space after "a")
    STRCMP_EQUAL(" very long line that should be truncated\nNext line", next);
}

// Test multiple consecutive newlines
TEST(ReadLineTests, HandleConsecutiveNewlines)
{
    const char* input = "First line\n\n\nFourth line";
    const char* next1 = read_line(input, buffer, sizeof(buffer));
    STRCMP_EQUAL("First line", buffer);
    
    const char* next2 = read_line(next1, buffer, sizeof(buffer));
    STRCMP_EQUAL("", buffer);
    
    const char* next3 = read_line(next2, buffer, sizeof(buffer));
    STRCMP_EQUAL("", buffer);
    
    read_line(next3, buffer, sizeof(buffer));
    STRCMP_EQUAL("Fourth line", buffer);
}

// Test line with only carriage return (no newline)
TEST(ReadLineTests, HandleCarriageReturnOnly)
{
    const char* input = "Line with CR only\rMore text";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    // Should read the entire string since there's no newline
    STRCMP_EQUAL("Line with CR only\rMore text", buffer);
    STRCMP_EQUAL("", next); // Should point to null terminator
}

// Test reading HTTP-style headers
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
    
    // Read body
    read_line(next4, buffer, sizeof(buffer));
    STRCMP_EQUAL("Body content", buffer);
}

// Test edge case: single character line
TEST(ReadLineTests, SingleCharacterLine)
{
    const char* input = "A\nB";
    const char* next = read_line(input, buffer, sizeof(buffer));
    
    STRCMP_EQUAL("A", buffer);
    STRCMP_EQUAL("B", next);
}

// Test edge case: buffer size of 1 (only room for null terminator)
TEST(ReadLineTests, MinimalBufferSize)
{
    const char* input = "Hello\nWorld";
    char tiny_buffer[1];
    const char* next = read_line(input, tiny_buffer, sizeof(tiny_buffer));
    
    STRCMP_EQUAL("", tiny_buffer); // Should be empty string
    // Should return pointer to the first character since no room to copy anything
    STRCMP_EQUAL("Hello\nWorld", next);
}

