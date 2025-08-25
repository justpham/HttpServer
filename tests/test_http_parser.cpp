#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

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
    HTTP_HEADER header_array[4];
    
    void setup()
    {
        // Setup test headers
        strcpy(header_array[0].key, "Content-Type");
        strcpy(header_array[0].value, "application/json");

        strcpy(header_array[1].key, "Authorization");
        strcpy(header_array[1].value, "Bearer token123");

        strcpy(header_array[2].key, "User-Agent");
        strcpy(header_array[2].value, "Mozilla/5.0");

        strcpy(header_array[3].key, "Content-Length");
        strcpy(header_array[3].value, "1024");
        
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
    CHECK_EQUAL(299, strlen(header.key));
}

TEST(ParseHeaderTests, ParseHeaderLongValue)
{
    // Create a header with a very long value to test buffer limits
    char long_value[300];
    memset(long_value, 'B', 299);
    long_value[299] = '\0';
    
    char line[400];
    snprintf(line, sizeof(line), "Custom-Header: %s\r\n", long_value);
    
    fprintf(stderr, "Testing long header value: %s\n", line);

    int result = parse_header(line, &header);

    fprintf(stderr, "Parsed header: %s: %s\n", header.key, header.value);

    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("Custom-Header", header.key);
    CHECK_EQUAL(299, strlen(header.value));
}

// Tests for parse_start_line function
TEST(ParseStartLineTests, ParseStartLineValidGetRequest)
{
    char line[] = "GET /index.html HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line, REQUEST);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", start_line.request.method);
    STRCMP_EQUAL("/index.html", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.protocol);
}

TEST(ParseStartLineTests, ParseStartLineValidPostRequest)
{
    char line[] = "POST /api/users HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line, REQUEST);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("POST", start_line.request.method);
    STRCMP_EQUAL("/api/users", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.protocol);
}

TEST(ParseStartLineTests, ParseStartLineWithQueryString)
{
    char line[] = "GET /search?q=test&page=1 HTTP/1.1\r\n";
    int result = parse_start_line(line, &start_line, REQUEST);
    
    CHECK_EQUAL(0, result);
    STRCMP_EQUAL("GET", start_line.request.method);
    STRCMP_EQUAL("/search?q=test&page=1", start_line.request.request_target);
    STRCMP_EQUAL("HTTP/1.1", start_line.request.protocol);
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
    STRCMP_EQUAL("HTTP/1.1", start_line.request.protocol);
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
    HTTP_HEADER single_header_array[1];
    strcpy(single_header_array[0].key, "Content-Type");
    strcpy(single_header_array[0].value, "application/json");

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

TEST_GROUP(ParseBodyTests)
{
    FILE* temp_file;
    char temp_filename[256];
    
    void setup()
    {
        // Create a temporary file for testing
        strcpy(temp_filename, "/tmp/test_body_XXXXXX");
        int fd = mkstemp(temp_filename);
        CHECK(fd != -1);
        temp_file = fdopen(fd, "w+");
        CHECK(temp_file != NULL);
    }

    void teardown()
    {
        if (temp_file) {
            fclose(temp_file);
        }
        unlink(temp_filename);
    }
    
    void verify_file_content(const char* expected_content, int expected_length)
    {
        // Rewind and read file content
        rewind(temp_file);
        char* buffer = (char*)malloc(expected_length + 1);
        size_t bytes_read = fread(buffer, 1, expected_length, temp_file);
        buffer[bytes_read] = '\0';
        
        LONGS_EQUAL(expected_length, bytes_read);
        STRNCMP_EQUAL(expected_content, buffer, expected_length);
        
        free(buffer);
    }
};

TEST(ParseBodyTests, ValidSmallBody)
{
    const char* body_content = "Hello, World!";
    int content_length = strlen(body_content);

    int result = parse_body(body_content, content_length, fileno(temp_file));

    LONGS_EQUAL(0, result);
    verify_file_content(body_content, content_length);
}

TEST(ParseBodyTests, EmptyBody)
{
    const char* body_content = "";
    int content_length = 0;

    int result = parse_body(body_content, content_length, fileno(temp_file));

    LONGS_EQUAL(0, result);
    verify_file_content("", 0);
}

TEST(ParseBodyTests, LargeBody)
{
    // Create a body larger than the chunk size (8192)
    int content_length = 10000;
    char* large_body = (char*)malloc(content_length + 1);
    
    // Fill with a pattern
    for (int i = 0; i < content_length; i++) {
        large_body[i] = 'A' + (i % 26);
    }
    large_body[content_length] = '\0';

    int result = parse_body(large_body, content_length, fileno(temp_file));

    LONGS_EQUAL(0, result);
    verify_file_content(large_body, content_length);
    
    free(large_body);
}

TEST(ParseBodyTests, NullBodyBuffer)
{
    int result = parse_body(NULL, 10, fileno(temp_file));
    LONGS_EQUAL(-1, result);
}

TEST(ParseBodyTests, NullFilePointer)
{
    const char* body_content = "test";
    int result = parse_body(body_content, 4, -1);
    LONGS_EQUAL(-1, result);
}

TEST(ParseBodyTests, NegativeContentLength)
{
    const char* body_content = "test";
    int result = parse_body(body_content, -1, fileno(temp_file));
    LONGS_EQUAL(-1, result);
}

TEST(ParseBodyTests, BinaryData)
{
    // Test with binary data including null bytes
    unsigned char binary_data[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0x00, 0x05};
    int content_length = sizeof(binary_data);

    int result = parse_body((const char*)binary_data, content_length, fileno(temp_file));

    LONGS_EQUAL(0, result);
    
    // Verify binary content
    rewind(temp_file);
    char read_buffer[sizeof(binary_data)];
    size_t bytes_read = fread(read_buffer, 1, sizeof(binary_data), temp_file);
    
    LONGS_EQUAL(content_length, bytes_read);
    MEMCMP_EQUAL(binary_data, read_buffer, content_length);
}

TEST_GROUP(ParseBodyStreamTests)
{
    int socket_pair[2];  // [0] for reading (socket side), [1] for writing (test side)
    FILE* output_file;
    char output_filename[256];
    bool write_socket_closed;
    char buffer[8192] = {0};

    void setup()
    {
        // Create socket pair for input (simulating socket communication)
        int result = socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair);
        CHECK(result == 0);
        write_socket_closed = false;
        
        // Create temporary output file
        strcpy(output_filename, "/tmp/test_output_XXXXXX");
        int out_fd = mkstemp(output_filename);
        CHECK(out_fd != -1);
        output_file = fdopen(out_fd, "w+");
        CHECK(output_file != NULL);
    }

    void teardown()
    {
        close(socket_pair[0]);
        if (!write_socket_closed) {
            close(socket_pair[1]);
        }
        
        if (output_file) {
            fclose(output_file);
        }
        unlink(output_filename);
    }
    
    void write_to_socket(const char* content, int length)
    {
        ssize_t total_written = 0;
        while (total_written < length) {
            ssize_t written = write(socket_pair[1], content + total_written, 
                                   length - total_written);
            CHECK(written > 0);
            total_written += written;
        }
    }
    
    void close_write_socket()
    {
        if (!write_socket_closed) {
            close(socket_pair[1]);
            write_socket_closed = true;
        }
    }
    
    void verify_output_content(const char* expected_content, int expected_length)
    {
        rewind(output_file);
        char* buffer = (char*)malloc(expected_length + 1);
        size_t bytes_read = fread(buffer, 1, expected_length, output_file);
        buffer[bytes_read] = '\0';
        
        LONGS_EQUAL(expected_length, bytes_read);
        STRNCMP_EQUAL(expected_content, buffer, expected_length);
        
        free(buffer);
    }
};

TEST(ParseBodyStreamTests, ValidSmallBodyStream)
{
    const char* body_content = "Hello from stream!";
    int content_length = strlen(body_content);
    
    write_to_socket(body_content, content_length);
    
    int result = parse_body_stream(socket_pair[0], content_length, buffer, fileno(output_file));
    
    LONGS_EQUAL(0, result);
    verify_output_content(body_content, content_length);
}

TEST(ParseBodyStreamTests, EmptyBodyStream)
{
    int content_length = 0;

    int result = parse_body_stream(socket_pair[0], content_length, buffer, fileno(output_file));

    LONGS_EQUAL(0, result);
    verify_output_content("", 0);
}

TEST(ParseBodyStreamTests, LargeBodyStream)
{
    // Create a body larger than the internal buffer (8192)
    int content_length = 15000;
    char* large_body = (char*)malloc(content_length + 1);
    
    // Fill with a pattern
    for (int i = 0; i < content_length; i++) {
        large_body[i] = 'X' + (i % 3); // X, Y, Z pattern
    }
    large_body[content_length] = '\0';
    
    write_to_socket(large_body, content_length);

    int result = parse_body_stream(socket_pair[0], content_length, buffer, fileno(output_file));

    LONGS_EQUAL(0, result);
    verify_output_content(large_body, content_length);
    
    free(large_body);
}

TEST(ParseBodyStreamTests, NullInputFile)
{
    int result = parse_body_stream(-1, 10, buffer, fileno(output_file));
    LONGS_EQUAL(-1, result);
}

TEST(ParseBodyStreamTests, NullOutputFile)
{
    int result = parse_body_stream(socket_pair[0], 10, buffer, -1);
    LONGS_EQUAL(-1, result);
}

TEST(ParseBodyStreamTests, NegativeContentLength)
{
    int result = parse_body_stream(socket_pair[0], -1, buffer, fileno(output_file));
    LONGS_EQUAL(-1, result);
}

TEST(ParseBodyStreamTests, UnexpectedEOF)
{
    const char* partial_content = "Only half";
    int partial_length = strlen(partial_content);
    int expected_length = 20; // Expect more than what's available
    
    write_to_socket(partial_content, partial_length);
    // Close the writing end to simulate EOF
    close_write_socket();

    int result = parse_body_stream(socket_pair[0], expected_length, buffer, fileno(output_file));

    LONGS_EQUAL(-2, result); // Should return -2 for unexpected EOF
}

TEST(ParseBodyStreamTests, BinaryDataStream)
{
    // Test with binary data including null bytes
    unsigned char binary_data[] = {0x10, 0x20, 0x00, 0x30, 0xFF, 0x00, 0x40, 0x50};
    int content_length = sizeof(binary_data);
    
    write_to_socket((const char*)binary_data, content_length);

    int result = parse_body_stream(socket_pair[0], content_length, buffer, fileno(output_file));

    LONGS_EQUAL(0, result);
    
    // Verify binary content
    rewind(output_file);
    char read_buffer[sizeof(binary_data)];
    size_t bytes_read = fread(read_buffer, 1, sizeof(binary_data), output_file);
    
    LONGS_EQUAL(content_length, bytes_read);
    MEMCMP_EQUAL(binary_data, read_buffer, content_length);
}
