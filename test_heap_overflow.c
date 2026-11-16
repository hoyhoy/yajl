#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <yajl/yajl_tree.h>

int main() {
    printf("Testing heap buffer over-read in yajl_tree_parse()...\n");
    
    // Allocate exactly 10 bytes for JSON without null terminator
    // This simulates receiving data from network/file without proper null termination
    char *json = malloc(10);
    if (!json) {
        fprintf(stderr, "malloc failed\n");
        return 1;
    }
    
    // Copy JSON string WITHOUT null terminator
    // Valid JSON: {"a":"b"} is exactly 9 bytes, we allocate 10
    memcpy(json, "{\"a\":\"b\"}", 9);
    json[9] = 'X';  // Fill last byte with non-null to ensure no accidental null terminator
    
    printf("Allocated 10 bytes at %p\n", (void*)json);
    printf("JSON content (first 10 bytes): ");
    for (int i = 0; i < 10; i++) {
        printf("%02x ", (unsigned char)json[i]);
    }
    printf("\n");
    
    char errbuf[1024];
    errbuf[0] = '\0';
    
    printf("Calling yajl_tree_parse() with non-null-terminated buffer...\n");
    printf("This will cause strlen() to read past the 10-byte allocation!\n");
    
    // This will cause strlen() to read beyond the 10-byte allocation
    // AddressSanitizer should detect: heap-buffer-overflow on READ
    yajl_val node = yajl_tree_parse(json, errbuf, sizeof(errbuf));
    
    if (node) {
        printf("Parse succeeded (unexpected)\n");
        yajl_tree_free(node);
    } else {
        printf("Parse failed: %s\n", errbuf);
    }
    
    free(json);
    
    printf("Test completed\n");
    return 0;
}
