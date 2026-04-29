#include <stdio.h>
#include <string.h>
#include "config.h"
#include "auth_session.h"
#include "net_handler.h"

int main() {
    printf("=== Admin Login Debug ===\n");
    printf("Expected username: '%s'\n", ADMIN_USER);
    printf("Expected password: '%s'\n", ADMIN_PASS);
    
    // Test session_auth_admin directly
    struct sockaddr_in test_addr;
    test_addr.sin_family = AF_INET;
    test_addr.sin_port = htons(12345);
    test_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    
    printf("\n=== Testing Direct Auth ===\n");
    int result = session_auth_admin(&test_addr, ADMIN_USER, ADMIN_PASS);
    printf("Direct auth result: %d (SUCCESS=%d)\n", result, SUCCESS);
    
    if (result == SUCCESS) {
        printf("Admin session created successfully!\n");
        printf("Is admin: %d\n", session_is_admin(&test_addr));
    } else {
        printf("Auth failed with code: %d\n", result);
    }
    
    // Test wrong credentials
    printf("\n=== Testing Wrong Credentials ===\n");
    result = session_auth_admin(&test_addr, "wrong", "wrong");
    printf("Wrong auth result: %d (should be %d)\n", result, ERR_AUTH_FAIL);
    
    return 0;
}
