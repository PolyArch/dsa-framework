/*
Implementation based on algorithm described in:
The cache performance and optimizations of blocked algorithms
M. D. Lam, E. E. Rothberg, and M. E. Wolf
ASPLOS 1991
*/

#include "gemm.h"
#include "mm_sb.dfg.h"

#include "../../../common/include/ss_insts.h"


void bbgemm(TYPE m1[N], TYPE m2[N], TYPE prod[N]){
    int i, k, j, jj, kk;
    int i_row, k_row;
    TYPE temp_x, mul;

    SS_CONFIG(mm_sb_config, mm_sb_size);

    loopjj:for (jj = 0; jj < row_size; jj += block_size){
        loopkk:for (kk = 0; kk < row_size; kk += block_size*2){
            loopi:for ( i = 0; i < row_size; ++i){
                i_row = i * row_size;

                SS_CONST(P_mm_sb_A,1,1);
                SS_DMA_READ(&m1[i_row + 0 + kk], 0, block_size * sizeof(TYPE),2,P_mm_sb_A);

                SS_GARBAGE_SIMP(P_mm_sb_R,block_size*2);
                SS_DMA_WRITE_SIMP(P_mm_sb_R,1,&prod[i_row + 0 + jj]);

                SS_STRIDE(row_size, block_size * sizeof(TYPE));
                SS_DMA_READ_SIMP(&prod[i_row + 0 + jj], 1,P_mm_sb_B);
                SS_DMA_READ_SIMP(&m2[k_row + jj],block_size*2,P_mm_sb_B);

                SS_CONST(P_mm_sb_reset,0,block_size*2);
                SS_CONST(P_mm_sb_reset,0,1);

                //loopk:for (k = 0; k < block_size; ++k){
                //    k_row = (k  + kk) * row_size;
                //    temp_x = m1[i_row + k + kk];
                //    loopj:for (j = 0; j < block_size; ++j){
                //        mul = temp_x * m2[k_row + j + jj];
                //        prod[i_row + j + jj] += mul;
                //    }
                //}
            }
        }
    }
    SS_WAIT_ALL();
}
