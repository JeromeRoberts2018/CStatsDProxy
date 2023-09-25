#include <stdio.h>
#include "../lib/logger.h"

int main() {
    // Test writing a simple message to the log
    write_log("This is a test message");

    // Test writing a message with a format specifier
    int num = 42;
    write_log("The answer to life, the universe, and everything is %d", num);

    // Test writing a message with multiple format specifiers
    double pi = 3.14159;
    char* str = "apple";
    write_log("The value of pi is %f and my favorite fruit is %s", pi, str);

    return 0;
}