#include "memory.h"
#include "monitor.h"

//Конец определен в скрипте компоновщика
extern u32int end;
u32int placement_address = (u32int)&end;

u32int vmalloc_int(u32int sz, int align, u32int *phys)
{
    //Это в конечном итоге вызовет malloc () в куче ядра. Сейчас мы назначаем память по адресу 
    //placement_address  и увеличиваем ее на sz. Даже когда мы закодировали нашу кучу ядра, это 
    //будет полезно для использования до инициализации кучи.

    if (align == 1 && (placement_address & 0xFFFFF000) )
    {
        //Сдвиг андресного пр-ва

        placement_address &= 0xFFFFF000;
        placement_address += 0x1000;
    }
    if (phys)
    {
        *phys = placement_address;
    }
    u32int tmp = placement_address;
    placement_address += sz;

    monitor_write("Memory was allocated at: ");
    monitor_write_hex(placement_address);
    monitor_write("\n");

    return tmp;
}

u32int vmalloc_a(u32int sz)
{
    return vmalloc_int(sz, 1, 0);
}

u32int vmalloc_p(u32int sz, u32int *phys)
{
    return vmalloc_int(sz, 0, phys);
}

u32int vmalloc_ap(u32int sz, u32int *phys)
{
    return vmalloc_int(sz, 1, phys);
}

u32int vmalloc(u32int sz)
{
    return vmalloc_int(sz, 0, 0);
}