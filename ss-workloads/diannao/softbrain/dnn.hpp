#ifndef DNN_H
#define DNN_H

#define M_REPEAT_4(X)  X X X X
#define M_REPEAT_8(X)  M_REPEAT_4(X)  M_REPEAT_4(X)
#define M_REPEAT_16(X) M_REPEAT_8(X)  M_REPEAT_8(X)
#define M_REPEAT_32(X) M_REPEAT_16(X) M_REPEAT_16(X)

#include <iostream>

//#define VTYPE uint32_t
#define VTYPE uint16_t

#ifdef __x86_64__
__attribute__ ((noinline))  void begin_roi() {
}
__attribute__ ((noinline))  void end_roi()   {
}

#else
__attribute__ ((noinline))  void begin_roi() {
  __asm__ __volatile__("add x0, x0, 1"); \
}
__attribute__ ((noinline))  void end_roi()   {
   __asm__ __volatile__("add x0, x0, 2"); \
}
#endif


static void sb_stats()   {
#ifndef __x86_64__
  __asm__ __volatile__("add x0, x0, 3");
#endif
}

//static __inline__ uint64_t rdtsc(void) {
//  unsigned a, d;
//  //asm("cpuid");
//  //asm volatile("rdtsc" : "=a" (a), "=d" (d));
//
//  return (((uint64_t)a) | (((uint64_t)d) << 32));
//}
//
//uint64_t ticks;
//__attribute__ ((noinline))  void begin_roi() {
//  ticks=rdtsc();
//}
//__attribute__ ((noinline))  void end_roi()   {
//  ticks=(rdtsc()-ticks);
//  std::cout << "ticks: " << ticks << "\n";
//}

//VTYPE a[16];
//VTYPE b[16];
//
VTYPE sigmoid(VTYPE i) {
//  return a[i&0xF]*i+b[i&0xF];
    return i*1024/(1024+i);
  return i;
}



void compare_short(VTYPE* neuron1, VTYPE* neuron2, int size) {
  bool error = false;
  for(int i = 0; i < size; ++i) {
    if(neuron1[i] != neuron2[i]) {
      printf("%d: %d %d\n",i,neuron1[i],neuron2[i]);
      error=true;
      //std::cout << i << " " << neuron1[i] << ":" << neuron2[i] << "\n";
    }
  }
  if(error) {
    std::cout << "ERROR: Results DO NOT Match\n";
  } else {
    std::cout << "Results Match\n";
  }
}

void compare(VTYPE* neuron1, VTYPE* neuron2, int size) {
  bool error = false;
  for(int i = 0; i < size; ++i) {
    if(neuron1[i] != neuron2[i]) {
      error = true; 
      break;
    }
  }
  if(error) {
    for(int i = 0; i < size; ++i) {
      std::cout << i << " " << neuron1[i] << ":" << neuron2[i];;
      if(neuron1[i] != neuron2[i]) {
        std::cout << " \t\tERROR";
      }
      std::cout << "\n";
    }
  } else {
    std::cout << "results match\n";
  }
}

void* aligned_malloc(uint64_t align, uint64_t bytes)  {
  size_t mask = (align-1)^((size_t)-1);
  char* ptr = (((char*)malloc(bytes+align)) + align);
  ptr = (char*) (((size_t)ptr) & mask);

  //touch each page to bring into OS! -- yes this takes a long time
  //never mind that, touch each cache line to bring into l2
  for(int i = 0; i < bytes; i+=32) {
    ptr[i]=0;
  }
  return (void*) ptr;
}

#endif
