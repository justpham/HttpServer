#ifndef SEND_MOCK_H
#define SEND_MOCK_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

// Mock interface for send() function
// This file provides the interface for mocking the send() system call
// using CppUTest's mocking framework with weak symbol override

// The actual mock implementation is in send_mock.c
// Tests should use the CppUTest mock() API to set expectations

#ifdef __cplusplus
}
#endif

#endif // SEND_MOCK_H
