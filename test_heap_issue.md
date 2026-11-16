# Testing for Heap Buffer Issues in yajl_tree_parse()

## Analysis Summary

Based on code review of `yajl_tree_parse()` in `src/yajl_tree.c`, I identified a **heap buffer over-read vulnerability**:

### Critical Issue: Unbounded strlen() on Input Buffer

**Location**: Lines 409-410 and 418-419 in `yajl_tree_parse()`

```c
// Line 409-410
status = yajl_parse(handle,
                    (unsigned char *) input,
                    strlen (input));  // ← VULNERABILITY: reads until null terminator

// Line 418-419 (error path)
internal_err_str = (char *) yajl_get_error(handle, 1,
      (const unsigned char *) input,
      strlen(input));  // ← Same vulnerability
```

**Root Cause**: 
- Function signature: `yajl_val yajl_tree_parse(const char *input, char *error_buffer, size_t error_buffer_size)`
- The function accepts `const char *input` without a length parameter
- Uses `strlen(input)` which assumes null-termination
- If caller passes a non-null-terminated heap buffer, `strlen()` reads beyond allocation

**Attack Scenario**:
1. Application receives JSON data from network/file into fixed-size buffer
2. Buffer is not null-terminated (common with binary protocols, fixed-length fields)
3. Application passes buffer to `yajl_tree_parse()`
4. `strlen()` reads past buffer boundary → heap buffer over-read
5. Information disclosure or crash

## Test Program to Reproduce

```c
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <yajl/yajl_tree.h>

int main() {
    printf("Testing heap buffer over-read in yajl_tree_parse()...\n");
    
    // Allocate exactly 10 bytes for JSON without null terminator
    char *json = malloc(10);
    if (!json) return 1;
    
    // Copy JSON string WITHOUT null terminator
    // Valid JSON: {"a":"b"} is 9 bytes
    memcpy(json, "{\"a\":\"b\"}", 9);
    json[9] = 'X';  // Ensure no accidental null terminator
    
    char errbuf[1024];
    
    // This will cause strlen() to read past the 10-byte allocation
    // AddressSanitizer should detect: heap-buffer-overflow on READ
    yajl_val node = yajl_tree_parse(json, errbuf, sizeof(errbuf));
    
    if (node) yajl_tree_free(node);
    free(json);
    
    return 0;
}
```

## Expected AddressSanitizer Output

When compiled with `-fsanitize=address` and run, ASAN should report:

```
==PID==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x...
READ of size 1 at 0x... thread T0
    #0 in strlen
    #1 in yajl_tree_parse src/yajl_tree.c:410
    #2 in main test_heap_overflow.c:XX
```

## Compilation and Testing Steps

```bash
# Compile test with AddressSanitizer
cd /Users/jperrie/yajl-hoyhoy
clang -fsanitize=address -g -I./src/include -L./build.asan/lib \
      test_heap_overflow.c -lyajl -o test_heap_overflow

# Run test (should trigger ASAN)
./test_heap_overflow

# Alternative: Use existing ASAN build
clang -fsanitize=address -g -I./build.asan/include \
      -L./build.asan/lib test_heap_overflow.c -lyajl -o test_heap_overflow
```

## Recommended Fix

Change the API to accept explicit length:

```c
// Option 1: Add new API with length parameter
yajl_val yajl_tree_parse_len(const char *input, size_t input_len,
                              char *error_buffer, size_t error_buffer_size);

// Option 2: Document requirement and add assertion
yajl_val yajl_tree_parse(const char *input,
                         char *error_buffer, size_t error_buffer_size) {
    // Document: input MUST be null-terminated
    assert(input != NULL);
    // ... rest of implementation
}
```

## Impact Assessment

- **Severity**: Medium to High
- **Type**: Heap buffer over-read (CWE-126)
- **Impact**: Information disclosure, potential crash
- **Exploitability**: Depends on how applications use the API
- **Affected Code**: `yajl_tree_parse()` function

## Additional Notes

The vulnerability exists because:
1. The API design assumes null-terminated strings
2. No length parameter is provided
3. No validation that input is properly terminated
4. Common in C APIs but dangerous with untrusted input

This is a design-level issue that requires API change or clear documentation of requirements.