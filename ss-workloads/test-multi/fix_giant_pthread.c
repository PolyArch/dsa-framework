#include "testing.h"
#include <stdlib.h>
#include <pthread.h>

#define VTYPE uint64_t
// #define GSIZE 1024*128*1024
// #define GSIZE 1024*128*128
#define GSIZE 1024
// #define GSIZE 512
// #define NUM_THREADS	1
#define NUM_THREADS	5
// #define NUM_THREADS	3

// Barrier variable
pthread_barrier_t barr;
char* giant_array;
// uint64_t giant_array[GSIZE];
// uint32_t giant_array[GSIZE];
// char giant_array[GSIZE];

void compute(long tid) {
  int n = GSIZE/NUM_THREADS;
  // int n = GSIZE/4;
  // n=1024*16;
  // n=72;

#if ACCEL == 1
  // accelerated version
  SS_CONFIG(none_config,none_size);
  // SS_DMA_READ(&giant_array[3], 8, 8, n/8, P_none_in);
  // SS_DMA_WRITE(P_none_out, 8, 8, n/8, &giant_array[3]);
 
  SS_DMA_READ(&giant_array[(tid*n)], 8, 8, n/8, P_none_in);
  // SS_DMA_READ(&giant_array[3], 8, 8, n/8, P_none_in);
  // SS_SCR_WRITE(P_none_out, 8*(n/8), 0);
  SS_DMA_WRITE(P_none_out, 8, 8, n/8, &giant_array[(tid*n/8)*8]);
  // SS_DMA_WRITE(P_none_out, 8, 8, n/8, &giant_array[(tid*n)]);
  // SS_DMA_READ(&giant_array[(tid*n/8)*8], 8, 8, n/8, P_none_in);
  // SS_DMA_WRITE(P_none_out, 8, 8, n/8, &giant_array[(tid*n/8)*8]);
  SS_WAIT_ALL();
 
  /*
  if(tid<2) {
  // if(1) {
    // accelerated version
    SS_CONFIG(none_config,none_size);
    SS_DMA_READ(&giant_array[tid*n], 8, 8, n/8, P_none_in);
    // SS_SCRATCH_READ(0, 8*(n/8), P_none_in);
    SS_DMA_WRITE(P_none_out, 8, 8, n/8, &giant_array[tid*n]);
    // SS_SCR_WRITE(P_none_out, 8*(n/8), 0);
    SS_WAIT_ALL();
  } else {
    SS_CONFIG(none_config,none_size);
    SS_SCRATCH_READ(0, 8*(n/8), P_none_in);
    SS_SCR_WRITE(P_none_out, 8*(n/8), 0);
    SS_WAIT_ALL();
  }
  */
  
#else
  // non-accelerated version
  for(int i=0; i<n/8; ++i) {
    giant_array[tid*n+i] = giant_array[tid*n+i];
  }
#endif
}

void *entry_point(void *threadid) {
  
   long tid;
   tid = (long)threadid;
   // Synchronization point
   int rc = pthread_barrier_wait(&barr);
   if(rc != 0 && rc != PTHREAD_BARRIER_SERIAL_THREAD)
   {
     printf("Could not wait on barrier\n");
     // exit(-1);
   }

   compute(tid);
   return NULL;
}


int main() {
  giant_array = (char*) aligned_alloc(64, GSIZE);

#if SINGLECORE == 1
  compute(0);

#else

 // Barrier initialization
  if(pthread_barrier_init(&barr, NULL, NUM_THREADS))
  {
    printf("Could not create a barrier\n");
    return -1;
  }

  pthread_t threads[NUM_THREADS];
  int rc;
  long t;
  for(t=0;t<NUM_THREADS;t++){
    printf("In main: creating thread %ld\n", t);
	// it should diverge instead of being done serially with the host thread?: put a barrier
    rc = pthread_create(&threads[t], NULL, entry_point, (void *)t);     
	if (rc){
      printf("ERROR; return code from pthread_create() is %d\n", rc);
	  return 0;
    }
  }
  
  for(int i = 0; i < NUM_THREADS; ++i) {
    if(pthread_join(threads[i], NULL)) {
  	printf("Could not join thread %d\n", i);
      return -1;
    }
  }

#endif

  delete giant_array;
  return 0;
}
