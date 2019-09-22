#ifndef SS_INSTS_H
#define SS_INSTS_H

// Magic sentinal for matching
#define SENTINAL (((uint64_t)1)<<63)
#define SENTINAL16 (((uint16_t)1)<<15)
#define SENTINAL32 (((uint32_t)1)<<31)

#define REPEAT_FXPNT_BITS (3)
#define REPEAT_FXPNT_VAL (1<<REPEAT_FXPNT_BITS)

// For bottom two bits:
#define NO_FILL        0
#define POST_ZERO_FILL 1
#define PRE_ZERO_FILL  2
#define STRIDE_ZERO_FILL 3
#define STRIDE_DISCARD_FILL 4

#define SS_FILL_MODE(mode) \
  __asm__ __volatile__("ss_fill_mode t0, t0, %0" : : "i"(mode));

//Mask for accessing shared scratchpad
#define SHARED_SP 0x100
#define SHARED_SP_INDEX 8

// This sets the context -- ie. which cores the following commands apply to
#define SS_CONTEXT(bitmask) \
  __asm__ __volatile__("ss_ctx %0, x0, 0" : : "r"(bitmask));

//This is the same as SS_CONTEXT butwith
#define SS_SET_ACCEL(core_id) \
  SS_CONTEXT(1<<core_id)

#define SS_CONTEXT_I(bitmask) \
  __asm__ __volatile__("ss_ctx x0, x0, %0" : : "i"(bitmask));

#define SS_CONTEXT_OFFSET(bitmask,offset) \
  __asm__ __volatile__("ss_ctx x0, %0, %1" : : "r"(offset), "i"(bitmask));

//Stream in the Config
#define SS_CONFIG(mem_addr, size) \
  __asm__ __volatile__("ss_cfg  %0, %1" : : "r"(mem_addr), "i"(size));

//Reset all live data requests!  (config retained)
#define SS_RESET() \
  __asm__ __volatile__("ss_cfg x0, 0");

//Add duplicate port
#define SS_ADD_PORT(port) \
  __asm__ __volatile__("ss_add_port x0, x0, %0" : : "i"(port))

#define SS_GARBAGE_GENERAL(output_port, num_elem, elem_size) \
  do { \
    int imm = (output_port) << 1; \
    __asm__ __volatile__("ss_wr_dma %0, %1, %2" : : "r"(0), "r"((num_elem) * (elem_size)), "i"(imm)); \
  } while (false) 

#define SS_GARBAGE(output_port, num_elem) \
  SS_GARBAGE_GENERAL(output_port, num_elem, 8)

/* DMA */
#define SS_DMA_RD_INNER(addr, acc_size, port) \
  do { \
    __asm__ __volatile__("ss_dma_rd %0, %1, %2" : : "r"(addr), "r"(acc_size), "i"((port) << 1)); \
  } while (false)

#define SS_DMA_RD_OUTER(stride, n, stretch) \
  __asm__ __volatile__("ss_dma_rd %0, %1, %2" : : "r"(stride), "r"(n), "i"((stretch) << 1 | 1))

#define SS_DMA_WR_INNER(addr, acc_size, port) \
  do { \
    __asm__ __volatile__("ss_wr_dma %0, %1, %2" : : "r"(addr), "r"(acc_size), "i"((port) << 1)); \
  } while (false)

#define SS_DMA_WR_OUTER(stride, n, stretch) \
  __asm__ __volatile__("ss_wr_dma %0, %1, %2" : : "r"(stride), "r"(n), "i"((stretch) << 1 | 1))
/* DMA End */

/* Scratchpad */
#define SS_SCR_RD_INNER(addr, acc_size, port) \
  do { \
    __asm__ __volatile__("ss_scr_rd %0, %1, %2" : : "r"(addr), "r"(acc_size), "i"((port) << 1)); \
  } while (false)

#define SS_SCR_RD_OUTER(stride, n, stretch) \
  __asm__ __volatile__("ss_scr_rd %0, %1, %2" : : "r"(stride), "r"(n), "i"((stretch) << 1 | 1))

#define SS_SCR_WR_INNER(addr, acc_size, port) \
  do { \
    __asm__ __volatile__("ss_wr_scr %0, %1, %2" : : "r"(addr), "r"(acc_size), "i"((port) << 1)); \
  } while (false)

#define SS_SCR_WR_OUTER(stride, n, stretch) \
  __asm__ __volatile__("ss_wr_scr %0, %1, %2" : : "r"(stride), "r"(n), "i"((stretch) << 1 | 1))
/* Scrathpad end */

#define SS_SET_ITER(n) \
  __asm__ __volatile__("ss_set_iter %0 " : : "r"(n))

//Fill the scratchpad from DMA (from memory or cache)
//Note that scratch_addr will be written linearly
//  __asm__ __volatile__("ss_stride    %0, %1, %2" : : "r"(stride), "r"(acc_size), "i"(stretch)); \
//  __asm__ __volatile__("ss_dma_addr  %0, %1" : : "r"(mem_addr), "r"(mem_addr)); \
//  __asm__ __volatile__("ss_dma_scr   %0, %1, %2" : : "r"(n_strides), "r"(scr_addr), "i"(shr)); 
#define SS_DMA_SCRATCH_LOAD_GENERAL(mem_addr, stride, acc_size, stretch, n_strides, scr_addr, shr) \
  SS_DMA_READ_STRETCH(mem_addr, stride, acc_size, stretch, n_strides, MEM_SCR_PORT); \
  SS_SCR_WRITE(MEM_SCR_PORT, acc_size * n_strides, scr_addr);
//TODO: FIXME: Make above work for other stretches

#define SS_DMA_SCRATCH_LOAD_STRETCH(mem_addr, stride, acc_size, stretch, n_strides, scr_addr) \
 SS_DMA_SCRATCH_LOAD_GENERAL(mem_addr, stride, acc_size, stretch, n_strides, scr_addr, 0);

#define SS_SCRATCH_LOAD_REMOTE(remote_scr_addr, stride, acc_size, stretch, n_strides, scr_addr) \
 SS_DMA_SCRATCH_LOAD_GENERAL(remote_scr_addr, stride, acc_size, stretch, n_strides, scr_addr, 1);

//Maintain compatibility with old thing (stretch=0)
#define SS_DMA_SCRATCH_LOAD(mem_addr, stride, acc_size, n_strides, scr_addr) \
  SS_DMA_SCRATCH_LOAD_STRETCH(mem_addr,stride, acc_size, 0, n_strides, scr_addr)


//old way:
//  __asm__ __volatile__("ss_stride    %0, %1, 0" : : "r"(stride), "r"(acc_size)); \
//  __asm__ __volatile__("ss_dma_addr  %0, %1" : : "r"(mem_addr), "r"(mem_addr)); \
//  __asm__ __volatile__("ss_scr_dma   %0, %1, %2" : : "r"(num_strides), "r"(scr_addr), "i"(shr));

#define SS_SCRATCH_DMA_STORE_GENERAL(scr_addr, stride, acc_size, num_strides, mem_addr, shr) \
  SS_SCR_PORT_STREAM_STRETCH(scr_addr,stride,acc_size,0,num_strides, SCR_MEM_PORT); \
  SS_DMA_WRITE(SCR_MEM_PORT, acc_size*num_strides, acc_size*num_strides, 1, mem_addr)

#define SS_SCRATCH_DMA_STORE(scr_addr, stride, access_size, num_strides, mem_addr) \
  SS_SCRATCH_DMA_STORE_GENERAL(scr_addr, stride, access_size, num_strides, mem_addr, 0)

#define SS_SCRATCH_STORE_REMOTE(scr_addr, stride, access_size, num_strides, mem_addr) \
  SS_SCRATCH_DMA_STORE_GENERAL(scr_addr, stride, access_size, num_strides, mem_addr, 1)

//Read from scratch into a cgra port
#define SS_SCR_PORT_STREAM_STRETCH(scr_addr,stride,acc_size,stretch,n_strides, port) \
  do { \
    if (acc_size > 0) { \
      SS_SCR_RD_OUTER(stride, n_strides, stretch); \
      SS_SCR_RD_INNER(scr_addr, acc_size, port);  \
    } else { \
      int _addr = scr_addr + acc_size; \
      int _outer_cnt = n_strides; \
      SS_SCR_RD_OUTER(stride, n_strides, stretch); \
      SS_SCR_RD_INNER(_addr, -acc_size, port);  \
    } \
  } while (false)

#define SS_SCR_PORT_STREAM(scr_addr,stride,acc_size,n_strides, port) \
   SS_SCR_PORT_STREAM_STRETCH(scr_addr,stride,acc_size,0,n_strides, port) 

//A convienience command for linear access
#define SS_SCRATCH_READ(scr_addr, n_bytes, port) \
  SS_SCR_PORT_STREAM_STRETCH(scr_addr,0,n_bytes,0,1, port) 

//Read from DMA into a port
#define SS_DMA_READ_STRETCH(mem_addr, stride, acc_size, stretch, n_strides, port ) \
  do { \
    SS_DMA_RD_OUTER(stride, n_strides, stretch); \
    SS_DMA_RD_INNER(mem_addr, acc_size, port); \
  } while (false)

#define SS_DMA_READ(mem_addr, stride, acc_size, n_strides, port ) \
  SS_DMA_READ_STRETCH(mem_addr, stride, acc_size, 0, n_strides, port )

#define SS_DMA_READ_SIMP(mem_addr, num_strides, port ) \
  __asm__ __volatile__("ss_dma_rd    %0, %1, %2" : : "r"(mem_addr), "r"(num_strides), "i"(port)); 


//Throw away some outputs.  We will add a proper instruction for this at some point, rather than writing to memory
#define SS_GARBAGE_SIMP(output_port, num_elem) \
  __asm__ __volatile__("ss_wr_dma   %0, %1, %2"   : : "r"(0), "r"(num_elem), "i"(output_port|0x100)); 


// Memory Oriented Instructions

//Set this back to zero if you need different kinds of writes later in the same code!!!
#define SS_GARBAGE_BEFORE_STRIDE(num_garb) \
  __asm__ __volatile__("ss_garb   %0, %1, 0" : : "r"(num_garb), "r"(num_garb)); \

// Plain Write to Memory
#define SS_DMA_WRITE(output_port, stride, acc_size, n_strides, mem_addr) \
  do { \
    SS_DMA_WR_OUTER(stride, n_strides, 0); \
    SS_DMA_WR_INNER(mem_addr, acc_size, output_port); \
  } while (false)


#define SS_DMA_WRITE_SIMP(output_port, num_strides, mem_addr) \
  __asm__ __volatile__("ss_wr_dma   %0, %1, %2"   : : "r"(mem_addr), "r"(num_strides), "i"(output_port)); 


//Write to DMA, but throw away all but the last 16-bits from each word
//TODO: make these work with types defined for indirection
#define SS_DMA_WRITE_SHF16(output_port, stride, access_size, num_strides, mem_addr) \
  __asm__ __volatile__("ss_stride   %0, %1, 0" : : "r"(stride), "r"(access_size)); \
  __asm__ __volatile__("ss_wr_dma   %0, %1, %2"   : : "r"(mem_addr),  "r"(num_strides), "i"(output_port|0x40)); 

//Write to DMA, but throw away all but the last 32-bits from each word  (implemented, not tested yet)
#define SS_DMA_WRITE_SHF32(output_port, stride, access_size, num_strides, mem_addr) \
  __asm__ __volatile__("ss_stride   %0, %1, 0" : : "r"(stride), "r"(access_size)); \
  __asm__ __volatile__("ss_wr_dma   %0, %1, %2"   : : "r"(mem_addr),  "r"(num_strides), "i"(output_port|0x80)); 


// Scratch Oriented Instructions
// Plain Write to Scratch  
#define SS_SCR_WRITE(output_port, num_bytes, scr_addr) \
  do { \
    SS_SCR_WR_INNER(scr_addr, num_bytes, output_port); \
  } while (false)

// Do atomic stream update in scratchpad
#define SS_ATOMIC_SCR_OP(addr_port, val_port, offset, iters, opcode) \
  __asm__ __volatile__("ss_atom_op %0, %1, %2" : : "r"(offset), "r"(iters), "i"((addr_port<<7) | (val_port<<2) | opcode));


// Send a constant value, repeated num_elements times to scratchpad
#define SS_CONST_SCR(scr_addr, val, num_elements) \
  __asm__ __volatile__("ss_set_iter %0 " : : "r"(num_elements)); \
  __asm__ __volatile__("ss_const_scr %0, %1, zero" : : "r"(scr_addr), "r"(val));

//Send a constant value, repeated num_elements times to a port
#define SS_CONST(port, val, num_elements) \
  __asm__ __volatile__("ss_const %0, %1, %2 " : : "r"(val), "r"(num_elements), "i"(port|(0<<8))); 

//Put a softbrain generated output value to a riscv core variable
#define SS_RECV(out_port, val) \
  __asm__ __volatile__("ss_recv %0, a0, %1 " : "=r"(val) : "i"(out_port)); 

//Send a constant value, repetated num_elements times to a port
// Plain Write to Scratch
#define SS_2D_CONST(port, val1, v1_repeat, val2, v2_repeat, iters) \
  __asm__ __volatile__("ss_set_iter %0 " : : "r"(iters)); \
  __asm__ __volatile__("ss_const %0, %1, %2 " : : "r"(val1), "r"(v1_repeat), "i"(port|(1<<7))); \
  __asm__ __volatile__("ss_const %0, %1, %2 " : : "r"(val2), "r"(v2_repeat), "i"(port|(1<<6))); 

//Send a constant value, repeated num_elements times to a port, with encoded
//const_width
#define SS_DCONST(port, val, num_elements, const_width) \
  __asm__ __volatile__("ss_const %0, %1, %2 " : : "r"(val), "r"(num_elements), "i"(port|const_width<<8));


// This tells the port to repeat a certain number of times before consuming
// This is only really associated with the next command, as this information is forgotten as soon as
// a command is issued.
// Assuming stretch of size 10-bits (MSB represents if repeat_times is a port
// ot number
#define SS_CONFIG_PORT_EXPLICIT(repeat_times, stretch) \
  __asm__ __volatile__("ss_cfg_port %0, t0, %1" : : "r"(repeat_times), "i"((stretch) << 1));

#define SS_CONFIG_PORT(repeat_times, stretch) \
  do { \
    SS_CONFIG_PORT_EXPLICIT((repeat_times)*REPEAT_FXPNT_VAL, (stretch)*REPEAT_FXPNT_VAL) \
  } while(false)

#define SS_REPEAT_PORT(times) \
  SS_CONFIG_PORT_EXPLICIT((times)*REPEAT_FXPNT_VAL, 0);

// data-dependent repeat based on the data in a port
// only affine read dma->port, scr->port stream
// Assume the configuration same as the config of times port
#define SS_VREPEAT_PORT(times_port) \
  SS_CONFIG_PORT_EXPLICIT(times_port, 1);

//Write from output to input port
#define SS_RECURRENCE(output_port, input_port, num_strides) \
  __asm__ __volatile__("ss_wr_rd %0, zero, %1" : : "r"(num_strides), "i"((input_port<<6) | (output_port)));

//Write from output to input port
#define SS_RECURRENCE_PAD(output_port, input_port, num_strides) \
  __asm__ __volatile__("ss_wr_rd %0, %1, %2" : : "r"(num_strides), "r"(4), "i"((input_port<<6) | (output_port)));

//Write from output to remote input port through the network (num_elem
//according to the port width)
#define SS_REM_PORT(output_port, num_elem, mask, remote_port) \
  __asm__ __volatile("ss_rem_port %0, %1, %2" : : "r"(num_elem), "r"(mask), "i"(((output_port<15?output_port:output_port-32)<<7) | (0<<6) | (remote_port<<1) | (0)));
  // __asm__ __volatile("ss_rem_port %0, %1, %2" : : "r"(num_elem), "r"(mask), "i"((output_port<<7) | (0<<6) | (remote_port<<1) | (0)));
  // __asm__ __volatile("ss_rem_port %0, %1, %2" : : "r"(num_elem), "r"(mask), "i"((output_port<<6) | (remote_port)));

//Write from output to remote scratchpad through the network (1 flag stands for spad)
#define SS_IND_REM_SCRATCH(val_port, addr_port, num_elem, scr_base_addr, scratch_type) \
  __asm__ __volatile("ss_rem_port %0, %1, %2" : : "r"(num_elem), "r"(scr_base_addr), "i"((val_port<<7) | (scratch_type<<6) | (addr_port<<1) | (1)));

#define SS_REM_SCRATCH(scr_base_addr, stride, access_size, num_strides, val_port, scratch_type) \
  __asm__ __volatile__("ss_stride   %0, %1, 0" : : "r"(stride), "r"(access_size)); \
  __asm__ __volatile("ss_rem_port %0, %1, %2" : : "r"(num_strides), "r"(scr_base_addr), "i"((val_port<<7) | (scratch_type<<6) | (0<<1) | (1)));
  // __asm__ __volatile("ss_rem_port %0, %1, %2" : : "r"(num_strides), "r"(scr_base_addr), "i"(((val_port<15?val_port:val_port-32)<<7) | (scratch_type<<6) | (0<<1) | (1)));

// banked scratchpad: scr->port, port->remote scr
// TODO: remove scratch_type later (for now, I make the immediate negative)
#define SS_SCR_REM_SCR(src_scr_base_addr, stride, access_size, num_strides, dest_scr_base_addr, scratch_type) \
  SS_SCR_PORT_STREAM(src_scr_base_addr, stride, access_size, num_strides, SCR_SCR_PORT) \
  SS_REM_SCRATCH(dest_scr_base_addr, stride, access_size, num_strides, (SCR_SCR_PORT-32), scratch_type);

#define SS_SCR_REM_PORT(scr_base_addr, num_strides, mask, remote_port) \
  SS_SCRATCH_READ(scr_base_addr, num_strides, SCR_REM_PORT) \
  SS_REM_PORT((SCR_REM_PORT-32), num_strides, mask, remote_port);

// could be affine stream to banked scratchpad also
// #define SS_REM_SCRATCH(scr_base_addr, num_bytes, val_port, scratch_type) \
//   __asm__ __volatile("ss_rem_port %0, %1, %2" : : "r"(num_bytes), "r"(scr_base_addr), "i"((val_port<<7) | (scratch_type<<6) | (0<<1) | (1)));

//Write from output to remote input port
//pos: local=0, left=1, right=2, undef=3
//13th bit: disable-padding=0, enable-padding=1
//(might be replaced later by some other RISCV instructions)
#define SS_XFER_LEFT(output_port, input_port, num_strides) \
  __asm__ __volatile__("ss_wr_rd %0, %1, %2" : : "r"(num_strides), "r"(1), "i"((input_port<<6) | (output_port)));
#define SS_XFER_RIGHT(output_port, input_port, num_strides) \
  __asm__ __volatile__("ss_wr_rd %0, %1, %2" : : "r"(num_strides), "r"(2), "i"((input_port<<6) | (output_port)))

#define SS_XFER_LEFT_PAD(output_port, input_port, num_strides) \
  __asm__ __volatile__("ss_wr_rd %0, %1, %2" : : "r"(num_strides), "r"(1 | 4), "i"((input_port<<6) | (output_port)));
#define SS_XFER_RIGHT_PAD(output_port, input_port, num_strides) \
  __asm__ __volatile__("ss_wr_rd %0, %1, %2" : : "r"(num_strides), "r"(2 | 4), "i"((input_port<<6) | (output_port)))


// Datatype Encodings
#define T64 0
#define T32 1
#define T16 2
#define T08 3

// currently output and data should be of same type
#define SS_CONFIG_ATOMIC_SCR_OP(addr_type, val_type, output_type) \
  __asm__ __volatile__("ss_cfg_atom_op t0, t0, %0" : : "i"( ((val_type<<4)&0x1ADB0 | (output_type<<2)&0x44C | (addr_type)&0x3)))
  
//configure the type of indirection -- here multiplier has to be less than 2^7
//Currently DTYPE MUST be 64 bits
#define SS_CONFIG_INDIRECT_GENERAL(itype,dtype,mult,offset_list)  \
  __asm__ __volatile__("ss_cfg_ind %0, %1, %2" : : "r"(offset_list), "r"(mult), "i"( (itype<<2)  |  (dtype<<0) )  )

#define SS_CONFIG_INDIRECT( itype,dtype,mult)             SS_CONFIG_INDIRECT_GENERAL(itype,dtype,mult,0) 
#define SS_CONFIG_INDIRECT1(itype,dtype,mult,o1)          SS_CONFIG_INDIRECT_GENERAL(itype,dtype,mult,o1) 
#define SS_CONFIG_INDIRECT2(itype,dtype,mult,o1,o2)       SS_CONFIG_INDIRECT_GENERAL(itype,dtype,mult,o1 | o2 << 8) 
#define SS_CONFIG_INDIRECT3(itype,dtype,mult,o1,o2,o3)    SS_CONFIG_INDIRECT_GENERAL(itype,dtype,mult,o1 | o2 << 8 | o3 << 16) 
#define SS_CONFIG_INDIRECT4(itype,dtype,mult,o1,o2,o3,o4) SS_CONFIG_INDIRECT_GENERAL(itype,dtype,mult,o1 | o2 << 8 | o3 << 16 | o4 << 24) 

//Write from output to input port  (type -- 3:8-bit,2:16-bit,1:32-bit,0:64-bit)
#define SS_INDIRECT(ind_port, addr_offset, num_elem, input_port) \
  __asm__ __volatile__("ss_ind    %0, %1, %2" : : "r"(addr_offset), "r"(num_elem),\
                                                  "i"((input_port<<5) | (ind_port)))

// generated streams are with base_addr = ind_port[i] (offset[col_ind],
// num_elem=num_elem_port[i] (offset[col_ind+1]-offset[col_ind],
// stride=sequential?)
#define SS_INDIRECT_2D(ind_port, addr_offset, num_elem, stride, access_size, num_elem_port, input_port) \
  __asm__ __volatile__("ss_stride   %0, %1, %2" : : "r"(stride), "r"(access_size), "i"(num_elem_port | (1<<10))); \
  __asm__ __volatile__("ss_ind    %0, %1, %2" : : "r"(addr_offset), "r"(num_elem),\
                                                  "i"((input_port<<5) | (ind_port)));
                                                  // "i"((input_port<<5) | (ind_port) | (1<<9))); \

// This works for only linear scratchpad right now
#define SS_INDIRECT_SCR_2D(ind_port, addr_offset, num_elem, stride, access_size, num_elem_port, input_port) \
  __asm__ __volatile__("ss_stride   %0, %1, %2" : : "r"(stride), "r"(access_size), "i"(num_elem_port | (1<<10))); \
  __asm__ __volatile__("ss_ind    %0, %1, %2" : : "r"(addr_offset), "r"(num_elem),\
                                                  "i"((1<<10) | (input_port<<5) | (ind_port)));


#define SS_INDIRECT_WR(ind_port, addr_offset, num_elem, output_port) \
  __asm__ __volatile__("ss_ind_wr %0, %1, %2" : : "r"(addr_offset), "r"(num_elem),\
                                                  "i"((output_port<<5) | (ind_port)));

//Write from output to input port  (type -- 3:8-bit,2:16-bit,1:32-bit,0:64-bit)
#define SS_INDIRECT_SCR(ind_port, addr_offset, num_elem, input_port) \
  __asm__ __volatile__("ss_ind    %0, %1, %2" : : "r"(addr_offset), "r"(num_elem),\
                                                  "i"((1<<10) | (input_port<<5) | (ind_port)));

#define SS_INDIRECT_WR_SCR(ind_port, addr_offset, num_elem, output_port) \
  __asm__ __volatile__("ss_ind_wr %0, %1, %2" : : "r"(addr_offset), "r"(num_elem),\
                                                  "i"((1<<10) | (output_port<<5) | (ind_port)));

//Wait on N number of remote scratchpad writes (num_bytes)
#define SS_WAIT_DF(num_rem_writes, scratch_type) \
  __asm __volatile__("ss_wait_df %0, %1" : : "r"(num_rem_writes), "i"(scratch_type));

//Wait with custom bit vector -- probably don't need to use
#define SS_WAIT(bit_vec) \
  __asm__ __volatile__("ss_wait t0, t0, " #bit_vec); \

//Wait for all softbrain commands and computations to be visible to memory from control core 
#define SS_WAIT_ALL() \
  __asm__ __volatile__("ss_wait t0, t0, 0" : : : "memory"); \

//Wait for all prior scratch writes to be complete.
#define SS_WAIT_SCR_WR() \
  __asm__ __volatile__("ss_wait t0, t0, 1"); \

//wait for everything except outputs to be complete. (useful for debugging)
#define SS_WAIT_COMPUTE() \
  __asm__ __volatile__("ss_wait t0, t0, 2" : : : "memory"); \

//wait for all prior scratch reads to be complete
#define SS_WAIT_SCR_RD() \
  __asm__ __volatile__("ss_wait t0, t0, 4"); \

//wait for all prior scratch reads to be complete (NOT IMPLEMENTED IN SIMULTOR YET)
#define SS_WAIT_SCR_RD_QUEUED() \
  __asm__ __volatile__("ss_wait t0, t0, 8"); \

//wait for all prior scratch reads to be complete (NOT IMPLEMENTED IN SIMULTOR YET)
#define SS_WAIT_MEM_WR() \
  __asm__ __volatile__("ss_wait t0, t0, 16"); \

#define SS_WAIT_SCR_ATOMIC() \
  __asm__ __volatile__("ss_wait t0, t0, 32"); \



//Indirect Ports
#define P_IND_1 (31)
#define P_IND_2 (30)
#define P_IND_3 (29)
#define P_IND_4 (28)
//TODO: make indirect ports also 1-byte
#define P_IND_5 (27)
// #define P_IND_6 (26)

//Convenience ports for these functions
#define MEM_SCR_PORT (23)
#define SCR_MEM_PORT (24)
#define SCR_SCR_PORT (25)
#define SCR_REM_PORT (26)

// #define NET_ADDR_PORT (25)
// #define NET_VAL_PORT (32)

#endif
