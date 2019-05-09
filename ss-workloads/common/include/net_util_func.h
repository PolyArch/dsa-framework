#include <stdint.h>
#include <stdio.h>
#define SCRATCH_SIZE 16384
#define LSCRATCH_SIZE 16384
#define NUM_CORES 64

uint64_t getMask(int list_dest[], int n) {
  uint64_t mask = 0;
  for(int i=0; i<n; ++i) {
    mask = mask | (1 << list_dest[i]);
  }
  return mask;
}

void addDest(uint64_t &mask, int d) {
  mask = mask | (1 << d);
}

int getLinearAddr(int offset) {
  int linear_scr_offset = 1 << 14; // log of scratch size
  return linear_scr_offset|offset;
}

// TODO: give the aligned answer
int getLinearOffset(int part_id, int n_part) {
  int x = LSCRATCH_SIZE*part_id/n_part;
  return ((x/8)*8);
}

int getBankedOffset(int part_id, int n_part) {
  int x = SCRATCH_SIZE*part_id/n_part;
  return ((x/8)*8);
}

int getRemoteAddr(int core_id, int addr) {
  int t = core_id << 15 | addr;
  return ((t/8)*8);
}

int getRemoteBankedOffset(int core_id, int part_id, int n_part) {
  int x = SCRATCH_SIZE*part_id/n_part;
  int t = core_id << 15 | x;
  return ((t/8)*8);
}

int getRemoteLinearOffset(int core_id, int part_id, int n_part) {
  int x = SCRATCH_SIZE*part_id/n_part;
  int t = core_id << 15 | 1 << 14 | x;
  return ((t/8)*8);
}

uint64_t merge_bits(uint16_t a, uint16_t b, uint16_t c, uint16_t d){
  uint64_t ret = (a | (b  & 0xFFFFFFFFFFFFFFFF) << 16 | (c & 0xFFFFFFFFFFFFFFFF) << 32 | (d & 0xFFFFFFFFFFFFFFFF) << 48);
  return ret;
}
