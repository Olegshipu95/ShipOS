#include "../include/panic.h"
#include "../../serial/serial.h"

void panic(char *message) {
    // Output to both VGA and serial for maximum visibility
    print(message);
    print("\tpanic!");
    
    PANIC_SERIAL("%s", message);
    PANIC_SERIAL("System halted!");
    
    while (1) {}
}