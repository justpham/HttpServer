# Testing Setup

This project uses CppUTest for unit testing C functions.

## Prerequisites

CppUTest is already installed and configured for this project.

If you need to install it on another system:

### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install cpputest
```

### Building from source:
```bash
git clone https://github.com/cpputest/cpputest.git
cd cpputest
./configure --prefix=/usr/local
make
sudo make install
```

## Running Tests

To build and run all tests:
```bash
make test
```

To clean test builds:
```bash
make clean_tests
```

To clean everything:
```bash
make clean
```

## Current Test Results

The test suite currently includes:
- **ConnectTests**: Tests for connection and IP address utility functions
- **ExampleTests**: Example tests showing different assertion types

Run `make test` to see current test results.

## Writing Tests

1. Create test files in the `tests/` directory with names starting with `test_` and `.cpp` extension
2. Include the CppUTest headers and your C header files
3. Use `extern "C"` to wrap C includes
4. Create TEST_GROUP and TEST macros as shown in the example files

Example test structure:
```cpp
#include "CppUTest/TestHarness.h"

extern "C" {
    #include "your_header.h"
}

TEST_GROUP(YourTestGroup)
{
    void setup() { /* setup code */ }
    void teardown() { /* cleanup code */ }
};

TEST(YourTestGroup, YourTestName)
{
    // Your test code here
    CHECK_EQUAL(expected, actual);
}
```

## Available Assertions

- `CHECK(condition)` - Check boolean condition
- `CHECK_EQUAL(expected, actual)` - Check equality
- `CHECK_TRUE(condition)` - Check if true
- `CHECK_FALSE(condition)` - Check if false
- `STRCMP_EQUAL(expected, actual)` - Compare strings
- `POINTERS_EQUAL(expected, actual)` - Compare pointers
- `FAIL(message)` - Force a test failure

## Test Files

The Makefile automatically discovers and compiles all `test_*.cpp` files in the tests directory.

Current test files:
- `test_main.cpp` - Test runner main function
- `test_connect.cpp` - Tests for connect.c functions
- `test_example.cpp` - Example tests showing different assertion types

## Notes

- Network-dependent tests should be used sparingly in unit tests
- Consider mocking external dependencies for better test isolation
- Each test should be independent and not rely on the state from other tests
- Tests run automatically when you use `make test`
