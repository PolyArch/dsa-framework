#include <stdint.h>
#include <stdio.h>


#include <sys/time.h>

static __inline__ uint64_t rdtsc(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (((uint64_t)tv.tv_sec) * 1000000 + ((uint64_t)tv.tv_usec));
}

static uint64_t ticks;

static void begin_roi() {

#ifndef __x86_64__
  __asm__ __volatile__("add x0, x0, 1");
#else
  ticks=rdtsc();
#endif

}


static void end_roi()   {

#ifndef __x86_64__
  __asm__ __volatile__("add x0, x0, 2");
#else
  ticks=(rdtsc()-ticks);
  printf("ticks: %lu\n", ticks);
#endif

}

static void sb_stats()   {
#ifndef __x86_64__
  __asm__ __volatile__("add x0, x0, 3");
#endif
}

static void sb_verify()   {
#ifndef __x86_64__
    __asm__ __volatile__("add x0, x0, 4");
#endif
}
