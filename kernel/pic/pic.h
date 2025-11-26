//
// Created by ShipOS developers on 28.10.23.
// Copyright (c) 2023 SHIPOS. All rights reserved.
//

// #include "../lib/include/stdint.h"
#include <inttypes.h>

#ifndef PIC_H
#define PIC_H

#define PIC1 0x20 /* IO base address for master PIC */
#define PIC2 0xA0 /* IO base address for slave PIC */
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)
#define PIC_EOI 0x20 /* End-of-interrupt command code */

/*
    https://pdos.csail.mit.edu/6.828/2005/readings/hardware/8259A.pdf
    Page 11, Figure 7
 */
/* ICW */
#define ICW1 0x11 /* D4 set to high, D0 set to high to use ICW4 */
#define PIC_MASTER_OFFSET 32
#define PIC_SLAVE_OFFSET 40
#define ICW3_MASTER 0x04 /* IR2 is a slave */
#define ICW3_SLAVE 0x02  /* Slave identity (2) */
#define ICW4_8086 0x01

void pic_send_eoi(uint8_t irq);
void pic_init();

#endif // PIC_H