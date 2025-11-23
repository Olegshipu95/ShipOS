#include "../include/panic.h"
#include "../../serial/serial.h"

void panic(char *message) {
    // Output to both VGA and serial for maximum visibility
    print(message);
    print("\tpanic!");
    
    serial_printf("[PANIC] %s\n", message);
    serial_printf("[PANIC] System halted!\n");
    
    while (1) {}
}