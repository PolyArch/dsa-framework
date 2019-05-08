/*
Based on algorithm described here:
http://www.cs.berkeley.edu/~mhoemmen/matrix-seminar/slides/UCB_sparse_tutorial_1.pdf
*/

#include "spmv.h"
#include "mm_lanes.dfg.h"
#include "../../../common/include/ss_insts.h"

void spmv(TYPE val[NNZ], int64_t cols[NNZ], int64_t rowDelimiters[N+1], TYPE vec[N], TYPE out[N]){
    int i;

    SS_CONFIG(mm_lanes_config,mm_lanes_size);

    SS_DMA_READ(&val[0],  0, NNZ * sizeof(TYPE),1,P_mm_lanes_Val);
    SS_DMA_READ(&cols[0], 0, NNZ * sizeof(TYPE),1,P_IND_1);
    SS_INDIRECT(P_IND_1,&vec[0],NNZ,P_mm_lanes_Vec);

    SS_STRIDE(8,8); //for making later commands simpler

    uint64_t prev_ind = rowDelimiters[0];
    spmv_1_unr : for(i = 0; i < N-3; i+=4){ //last two have zeros anyways
        uint64_t ind0 = rowDelimiters[i+1];
        uint64_t sizem0 = ind0 - prev_ind -1;
        SS_CONST(P_mm_lanes_reset,0,sizem0);
        SS_CONST(P_mm_lanes_reset,1,1);
        SS_GARBAGE_SIMP(P_mm_lanes_O,sizem0);
        SS_DMA_WRITE_SIMP(P_mm_lanes_O,1,&out[i]);

        uint64_t ind1 = rowDelimiters[i+2];
        uint64_t sizem1 = ind1 - ind0 -1;
        SS_CONST(P_mm_lanes_reset,0,sizem1);
        SS_CONST(P_mm_lanes_reset,1,1);
        SS_GARBAGE_SIMP(P_mm_lanes_O,sizem1);
        SS_DMA_WRITE_SIMP(P_mm_lanes_O,1,&out[i+1]);

        uint64_t ind2 = rowDelimiters[i+3];
        uint64_t sizem2 = ind2 - ind1 -1;
        SS_CONST(P_mm_lanes_reset,0,sizem2);
        SS_CONST(P_mm_lanes_reset,1,1);
        SS_GARBAGE_SIMP(P_mm_lanes_O,sizem2);
        SS_DMA_WRITE_SIMP(P_mm_lanes_O,1,&out[i+2]);

        uint64_t ind3 = rowDelimiters[i+4];
        uint64_t sizem3 = ind3 - ind2 -1;
        SS_CONST(P_mm_lanes_reset,0,sizem3);
        SS_CONST(P_mm_lanes_reset,1,1);
        SS_GARBAGE_SIMP(P_mm_lanes_O,sizem3);
        SS_DMA_WRITE_SIMP(P_mm_lanes_O,1,&out[i+3]);

        //spmv_2 : for (j = tmp_begin; j < tmp_end; j++){
        //    Si = val[j] * vec[cols[j]];
        //    sum = sum + Si;
        //}
        //out[i] = sum;
        prev_ind=ind3;
    }

    spmv_1 : for(; i < N; i++){ //last two have zeros anyways
        uint64_t ind = rowDelimiters[i+1];
        uint64_t size = ind - prev_ind;

        SS_CONST(P_mm_lanes_reset,0,size-1);
        SS_CONST(P_mm_lanes_reset,1,1);
        SS_GARBAGE_SIMP(P_mm_lanes_O,size-1);
        SS_DMA_WRITE_SIMP(P_mm_lanes_O,1,&out[i]);


        //spmv_2 : for (j = tmp_begin; j < tmp_end; j++){
        //    Si = val[j] * vec[cols[j]];
        //    sum = sum + Si;
        //}
        //out[i] = sum;
        prev_ind=ind;
    }
    SS_WAIT_ALL();

    //for(int i = 0; i < N; ++i) {
    //  printf("%d, ",a[i]);
    //  if(i%16==0) {
    //    printf("\n");
    //  }
    //}
    //printf("\n");

}

//TODO: Optimization Idea: Break into two halves, send each one down separate lane
