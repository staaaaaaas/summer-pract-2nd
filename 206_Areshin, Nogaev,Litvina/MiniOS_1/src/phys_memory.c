// phys_memory.c -- Определяет интерфейс и структуры, относящиеся к подкачке.

#include "phys_memory.h"
#include "memory.h"

// Каталог страниц ядра.
page_directory_t *kernel_directory=0;

// Текущая страница каталога.
page_directory_t *current_directory=0;

// Набор bitset для фреймов.
u32int *frames;
u32int nframes;

// Определено в memory.c
extern u32int placement_address;

// В алгоритмах для bitset используются макросы.
#define INDEX_FROM_BIT(a) (a/(8*4))
#define OFFSET_FROM_BIT(a) (a%(8*4))

// Статическая функция для установки бита в наборе bitset для фреймов.
static void set_frame(u32int frame_addr)
{
    u32int frame = frame_addr/0x1000;
    u32int idx = INDEX_FROM_BIT(frame);
    u32int off = OFFSET_FROM_BIT(frame);
    frames[idx] |= (0x1 << off);
}

// Статическая функция для сброса бита в наборе bitset для фреймов.
static void clear_frame(u32int frame_addr)
{
    u32int frame = frame_addr/0x1000;
    u32int idx = INDEX_FROM_BIT(frame);
    u32int off = OFFSET_FROM_BIT(frame);
    frames[idx] &= ~(0x1 << off);
}

// Статическая функция для проверки, установлен ли бит.
static u32int test_frame(u32int frame_addr)
{
    u32int frame = frame_addr/0x1000;
    u32int idx = INDEX_FROM_BIT(frame);
    u32int off = OFFSET_FROM_BIT(frame);
    return (frames[idx] & (0x1 << off));
}

// Статическая функция для поиска первого свободного фрейма.
static u32int first_frame()
{
    u32int i, j;
    for (i = 0; i < INDEX_FROM_BIT(nframes); i++)
    {
        if (frames[i] != 0xFFFFFFFF) // нечего не освобождаем, сразу выходим.
        {
            // По меньшей мере, здесь один свободный бит.
            for (j = 0; j < 32; j++)
            {
                u32int toTest = 0x1 << j;
                if ( !(frames[i]&toTest) )
                {
                    return i*4*8+j;
                }
            }
        }
    }
}

// Функция выделения фрейма.
void alloc_frame(page_t *page, int is_kernel, int is_writeable)
{
    if (page->frame != 0)
    {
        return;
    }
    else
    {
        u32int idx = first_frame();
        if (idx == (u32int)-1)
        {
            // Нет свободных фреймов.
        }
        set_frame(idx*0x1000);
        page->present = 1;
        page->rw = (is_writeable)?1:0;
        page->user = (is_kernel)?0:1;
        page->frame = idx;
    }
}

// Функция освобождения фрейма.
void free_frame(page_t *page)
{
    u32int frame;   
    if (!(frame=page->frame))
    {
        return;
    }
    else
    {
        clear_frame(frame);
        page->frame = 0x0;
    }
    monitor_write("\nDISK WAS FORMATED!\n");
}

void mem_init()
{
    // Размер физической памяти. Сейчас мы предполагаем,
    // что размер равен 16 MB.
    u32int mem_end_page = 0x1000000;

    nframes = mem_end_page / 0x1000;
    frames = (u32int*)vmalloc(INDEX_FROM_BIT(nframes));
    memset(frames, 0, INDEX_FROM_BIT(nframes));
    
    // Создадим директорий страниц.
    kernel_directory = (page_directory_t*)vmalloc_a(sizeof(page_directory_t));
    current_directory = kernel_directory;

    // Нам нужна карта идентичности (физический адрес = виртуальный адрес) с адреса
    // 0x0 до конца используемой памяти, чтобы у нас к ним был прозрачный 
    // доступ, как если бы страничная организация памяти не использовалась.
    // Мы преднамеренно используем цикл while.
    // Внутри тела цикла мы фактически изменяем адрес placement_address
    // с помощью вызова функции vmalloc(). Цикл while используется здесь, т.к. выход
    // из цикла динамически, а не один раз после запуска цикла.
    int i = 0;
    while (i < placement_address)
    {
        // Код ядра можно читать из пользовательского режима, но нельзя в него записывать.
        alloc_frame( get_page(i, 1, kernel_directory), 0, 0);
        i += 0x1000;
    }
    // Прежде, чем включить страничное управлениеpaging памятью, нужно зарегистрировать
    // обработчик некорректного обращения к памяти - page_fault.
    register_interrupt_handler(14, &die);

    // Теперь включаем страничную организацию памяти.
    zone_list(kernel_directory);
    monitor_write("\nDISK WAS CREATED!\n");
    switch_user_mode();
}

void switch_user_mode(){
    monitor_write("Switched to user mode\n");
}

void zone_list(page_directory_t *dir)
{
    current_directory = dir;
    asm volatile("mov %0, %%cr3":: "r"(&dir->tablesPhysical));
    u32int cr0;
    asm volatile("mov %%cr0, %0": "=r"(cr0));
    cr0 |= 0x80000000; // Enable paging!
    asm volatile("mov %0, %%cr0":: "r"(cr0));
}

page_t *get_page(u32int address, int make, page_directory_t *dir)
{
    // Помещаем адрес в индекс.
    address /= 0x1000;
    // Находим таблицу страниц, в которой есть этот адрес.
    u32int table_idx = address / 1024;
    if (dir->tables[table_idx]) // Если эта таблица уже назначена
    {
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else if(make)
    {
        u32int tmp;
        dir->tables[table_idx] = (page_table_t*)vmalloc_ap(sizeof(page_table_t), &tmp);
        dir->tablesPhysical[table_idx] = tmp | 0x7; // PRESENT, RW, US.
        return &dir->tables[table_idx]->pages[address%1024];
    }
    else
    {
        return 0;
    }
}

void do_page_fault(registers_t regs)
{
    // A page fault has occurred.
    // The faulting address is stored in the CR2 register.
    u32int faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    
    // The error code gives us details of what happened.
    int present   = !(regs.err_code & 0x1); // Page not present
    int rw = regs.err_code & 0x2;           // Write operation?
    int us = regs.err_code & 0x4;           // Processor was in user-mode?
    int reserved = regs.err_code & 0x8;     // Overwritten CPU-reserved bits of page entry?
    int id = regs.err_code & 0x10;          // Caused by an instruction fetch?

    // Output an error message.
    monitor_write("Page fault! ( ");
    if (present) {monitor_write("present ");}
    if (rw) {monitor_write("read-only ");}
    if (us) {monitor_write("user-mode ");}
    if (reserved) {monitor_write("reserved ");}
    monitor_write(") at 0x");
    monitor_write_hex(faulting_address);
    monitor_write("\n");
    PANIC("Page fault");
}

void die(registers_t regs){
    u32int faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));
    monitor_write("\nGOODBYE!\n\n");
    register_interrupt_handler(13, &do_page_fault);
    asm volatile("int $0xD");


}
