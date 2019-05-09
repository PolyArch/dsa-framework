#define IN_ACT_SLICE 0  //also the group throughput for 6 groups are mapped here,
                        // with 5 bits each
#define OUT_ACT_SLICE 1 
#define VP_MAP_SLICE_1 2
#define VP_MAP_SLICE_2 3
#define VP_MAP_SLICE_OUT 4

#define NUM_DFG_GROUPS 6
#define IN_ACT_GROUP12 5
#define IN_ACT_GROUP34 6
#define IN_ACT_GROUP56 7

#define OUT_ACT_GROUP12 8
#define OUT_ACT_GROUP34 9
#define OUT_ACT_GROUP56 10

#define SWITCH_SLICE 11        //the starting slice position in bitslice

#define NUM_IN_DIRS 8
#define NUM_OUT_DIRS 8
#define NUM_IN_FU_DIRS 3
#define BITS_PER_DIR 3
#define BITS_PER_FU_DIR 3

#define ROW_LOC 0
#define ROW_BITS 4

#define SWITCH_LOC   ROW_LOC + ROW_BITS
#define SWITCH_BITS   BITS_PER_DIR * NUM_OUT_DIRS

#define FU_DIR_LOC   SWITCH_LOC + SWITCH_BITS
#define FU_DIR_BITS   BITS_PER_FU_DIR * NUM_IN_FU_DIRS

#define FU_PRED_INV_LOC   FU_DIR_LOC + FU_DIR_BITS
#define FU_PRED_INV_BITS   1

#define OPCODE_LOC   FU_PRED_INV_LOC + FU_PRED_INV_BITS
#define OPCODE_BITS   7

#define IN_DELAY_LOC   OPCODE_LOC + OPCODE_BITS
#define BITS_PER_DELAY   4
#define NUM_DELAY   3
#define IN_DELAY_BITS   BITS_PER_DELAY*NUM_DELAY

//Config Message uses same row loc:
#define COL_LOC ROW_LOC+ROW_BITS
#define COL_BITS 4

#define IS_IMM_LOC COL_LOC+COL_BITS
#define IS_IMM_BITS 1

#define CTRL_LOC IS_IMM_LOC+IS_IMM_BITS
#define CTRL_BITS 32
