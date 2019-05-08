#ifndef DNN_H
#define DNN_H

#include <iostream>

//#define VTYPE uint32_t
#define VTYPE uint16_t

//int a;
//
//__attribute__ ((noinline))  void begin_roi() {
//  a=1;
//}
//__attribute__ ((noinline))  void end_roi()   {
//  a=0;
//}

static __inline__ uint64_t rdtsc(void) {
  unsigned a, d;
  //asm("cpuid");
  asm volatile("rdtsc" : "=a" (a), "=d" (d));

  return (((uint64_t)a) | (((uint64_t)d) << 32));
}

uint64_t ticks;
__attribute__ ((noinline))  void begin_roi() {
  ticks=rdtsc();
}
__attribute__ ((noinline))  void end_roi()   {
  ticks=(rdtsc()-ticks);
  std::cout << "ticks: " << ticks << "\n";
}

VTYPE a[16];
VTYPE b[16];

VTYPE sigmoid(VTYPE i) {
  //return i*i; //return i*i*i;
  //return a[i&0xF]*i+b[i&0xF];
  return i*1024/(1024+i);
}

void compare(VTYPE* neuron1, VTYPE* neuron2, int size) {
  for(int i = 0; i < size; ++i) {
    if(neuron1[i] != neuron2[i]) {
      std::cout << i << " " << neuron1[i] << ":" << neuron2[i] << "\n";
    }
  }
}

#endif
