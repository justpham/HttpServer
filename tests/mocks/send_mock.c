#include <sys/socket.h>
#include <errno.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#include "CppUTestExt/MockSupport_c.h"
}
#else
#include "CppUTestExt/MockSupport_c.h"
#endif

// Weak symbol override of send() function
// This allows us to intercept send() calls in tests while leaving
// production code unchanged
ssize_t send(int sockfd, const void *buf, size_t len, int flags) __attribute__((weak));

ssize_t send(int sockfd, const void *buf, size_t len, int flags) {
    // Create a copy of the buffer data for parameter checking
    // Note: We'll use a smaller buffer for practical testing
    char buffer_copy[4096];
    size_t copy_len = len < sizeof(buffer_copy) ? len : sizeof(buffer_copy);
    
    if (buf && copy_len > 0) {
        memcpy(buffer_copy, buf, copy_len);
        if (copy_len < sizeof(buffer_copy)) {
            buffer_copy[copy_len] = '\0'; // Ensure null termination for string operations
        } else {
            buffer_copy[sizeof(buffer_copy) - 1] = '\0';
        }
    } else {
        buffer_copy[0] = '\0';
    }
    
    // Record the actual call with CppUTest mock framework
    // Use the C interface properly
    mock_c()->actualCall("send")
        ->withIntParameters("sockfd", sockfd)
        ->withIntParameters("len", (int)len)
        ->withIntParameters("flags", flags)
        ->withStringParameters("buf", buffer_copy);
    
    // Get the return value from the mock framework
    int return_value = mock_c()->returnValue().value.intValue;
    
    // Handle errno setting if return value indicates error
    if (return_value < 0) {
        // Set default errno for errors
        errno = EBADF;
    }
    
    return return_value;
}
