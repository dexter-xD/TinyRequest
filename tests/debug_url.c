#include <stdio.h>
#include "include/http_client.h"

int main() {
    printf("Testing URL validation:\n");

    const char* invalid_urls[] = {
        NULL,
        "",
        "example.com",
        "www.example.com",
        "ftp://example.com",
        "file://example.com",
        "http://",
        "https://",
        "http:///path",
        "http://.example.com",
        "http://-example.com",
        "http:",
        "h"
    };

    for (int i = 0; i < 13; i++) {
        const char* url = invalid_urls[i];
        int result = ValidateUrl(url);
        printf("URL: '%s' -> %s (expected: invalid)\n",
               url ? url : "NULL",
               result ? "VALID" : "INVALID");
        if (result) {
            printf("  ^^^ This should be INVALID but returned VALID!\n");
        }
    }

    return 0;
}