//Функции кучи ядра

#ifndef MEMORY_H
#define MEMORY_H
#include "common.h"

//Выделите кусок памяти, размер по sz. 
//Если выровнять allign== 1, кусок должен быть выровнен по странице. Если Phys! = 0, физическое местоположение выделенного фрагмента будет сохранено в Phys.

u32int vmalloc_int(u32int sz, int align, u32int *phys);

//Выделяет кусок памяти размера sz, который должен быть выровнен по странице

u32int vmalloc_a(u32int sz);

//Выделяет кусок памяти размера sz. Физический адрес -> phys. 
  
u32int vmalloc_p(u32int sz, u32int *phys);

//Выделяет кусок памяти размера sz. Физический адрес -> phys. Кусок должен быть выровнен по странице

u32int vmalloc_ap(u32int sz, u32int *phys);

//Основная функция выделения памяти

u32int vmalloc(u32int sz);

#endif // MEMORY_H
