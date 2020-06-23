// main.c -- Определяет точку входа Cи-кода ядра

#include "monitor.h"
#include "multiboot.h"
#include "descriptor_tables.h"
#include "timer.h"
#include "phys_memory.h"


int main(struct multiboot *mboot_ptr)
{
   // Initialise all the ISRs and segmentation
    init_descriptor_tables();

    // Initialise the screen (by clearing it)
    monitor_clear();

    // Enables interrupts
    free_frame();
    
    monitor_write("\n!!!!!!!!!!!!!\n\n");

    asm volatile("sti");
    // Initialising of disk
    mem_init();

    monitor_write("\n!!!!!!!!!!!!!\n");
    monitor_write("!!!HI USER!!!\n");
    monitor_write("!!!!!!!!!!!!!\n\n");

    
    // An example of page_fult
    u32int *ptr = (u32int*)0xA0000000;
    u32int page_fault = *ptr;

    return 0;
}