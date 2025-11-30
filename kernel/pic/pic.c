//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

//#include "../lib/include/stdint.h"
#include <inttypes.h>
#include "pic.h"

void pic_send_eoi(uint8_t irq) {
    if (irq >= PIC_SLAVE_OFFSET) outb(PIC2_COMMAND, PIC_EOI);
    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_init() {
    outb(PIC1_COMMAND, ICW1);
    outb(PIC2_COMMAND, ICW1);

    outb(PIC1_DATA, PIC_MASTER_OFFSET);
    outb(PIC2_DATA, PIC_SLAVE_OFFSET);

    outb(PIC1_DATA, ICW3_MASTER);
    outb(PIC2_DATA, ICW3_SLAVE);

    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
}
