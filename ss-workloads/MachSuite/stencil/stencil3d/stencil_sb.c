/*
Implementation based on algorithm described in:
"Stencil computation optimization and auto-tuning on state-of-the-art multicore architectures"
K. Datta, M. Murphy, V. Volkov, S. Williams, J. Carter, L. Oliker, D. Patterson, J. Shalf, K. Yelick
SC 2008
*/

#include "stencil.h"
#include "stencil_sb.dfg.h"
#include "../../../common/include/ss_insts.h"

void stencil3d(TYPE C[2], TYPE orig[SIZE], TYPE sol[SIZE]) {
    int i, j, k;
    //TYPE sum0, sum1, mul0, mul1;

    // Handle boundary conditions by filling with original values
    //height_bound_col : for(j=0; j<col_size; j++) {
    //    height_bound_row : for(k=0; k<row_size; k++) {
    //        sol[INDX(row_size, col_size, k, j, 0)] = orig[INDX(row_size, col_size, k, j, 0)];
    //        sol[INDX(row_size, col_size, k, j, height_size-1)] = orig[INDX(row_size, col_size, k, j, height_size-1)];
    //    }
    //}
    //col_bound_height : for(i=1; i<height_size-1; i++) {
    //    col_bound_row : for(k=0; k<row_size; k++) {
    //        sol[INDX(row_size, col_size, k, 0, i)] = orig[INDX(row_size, col_size, k, 0, i)];
    //        sol[INDX(row_size, col_size, k, col_size-1, i)] = orig[INDX(row_size, col_size, k, col_size-1, i)];
    //    }
    //}
    //row_bound_height : for(i=1; i<height_size-1; i++) {
    //    row_bound_col : for(j=1; j<col_size-1; j++) {
    //        sol[INDX(row_size, col_size, 0, j, i)] = orig[INDX(row_size, col_size, 0, j, i)];
    //        sol[INDX(row_size, col_size, row_size-1, j, i)] = orig[INDX(row_size, col_size, row_size-1, j, i)];
    //    }
    //}

    SS_CONFIG(stencil_sb_config,stencil_sb_size);
    int total_elem = (height_size-2) * (col_size-2) * (row_size-2);
    SS_CONST(P_stencil_sb_C0, C[0], total_elem);
    SS_CONST(P_stencil_sb_C1, C[1], total_elem);


    // Stencil computation
    loop_height : for(i = 1; i < height_size - 1; i++){
//        loop_col : for(j = 1; j < col_size - 1; j++){
//            loop_row : for(k = 1; k < row_size - 1; k++) {
          k = 1;
          SS_DMA_READ(&orig[INDX(row_size, col_size, k, j, i)], 
                      row_size * sizeof(TYPE), (row_size-2) * sizeof(TYPE),(col_size-2),P_stencil_sb_M);

          SS_DMA_READ(&orig[INDX(row_size, col_size, k, j, i+1)], 
                      row_size * sizeof(TYPE), (row_size-2) * sizeof(TYPE),(col_size-2),P_stencil_sb_MIP);
          SS_DMA_READ(&orig[INDX(row_size, col_size, k, j, i-1)], 
                      row_size * sizeof(TYPE), (row_size-2) * sizeof(TYPE),(col_size-2),P_stencil_sb_MIM);


          SS_DMA_READ(&orig[INDX(row_size, col_size, k, j+1, i)], 
                      row_size * sizeof(TYPE), (row_size-2) * sizeof(TYPE),(col_size-2),P_stencil_sb_MJP);
          SS_DMA_READ(&orig[INDX(row_size, col_size, k, j-1, i)], 
                      row_size * sizeof(TYPE), (row_size-2) * sizeof(TYPE),(col_size-2),P_stencil_sb_MJM);


          SS_DMA_READ(&orig[INDX(row_size, col_size, k+1, j, i)], 
                      row_size * sizeof(TYPE), (row_size-2) * sizeof(TYPE),(col_size-2),P_stencil_sb_MKP);
          SS_DMA_READ(&orig[INDX(row_size, col_size, k-1, j, i)], 
                      row_size * sizeof(TYPE), (row_size-2) * sizeof(TYPE),(col_size-2),P_stencil_sb_MKM);

         //printf("i,j,k: %d,%d,%d, %d\n",i,j,k,INDX(row_size, col_size, k, j, i));

         SS_DMA_WRITE(P_stencil_sb_R, row_size*sizeof(TYPE), (row_size-2)*sizeof(TYPE), (col_size-2), &(sol[INDX(row_size, col_size, k, j, i)]) );
//      }
//      }
   }
   SS_WAIT_ALL();
}

//                sum0 = orig[INDX(row_size, col_size, k, j, i)];
//                sum1 = orig[INDX(row_size, col_size, k, j, i + 1)] +
//                       orig[INDX(row_size, col_size, k, j, i - 1)] +
//                       orig[INDX(row_size, col_size, k, j + 1, i)] +
//                       orig[INDX(row_size, col_size, k, j - 1, i)] +
//                       orig[INDX(row_size, col_size, k + 1, j, i)] +
//                       orig[INDX(row_size, col_size, k - 1, j, i)];
//                mul0 = sum0 * C[0];
//                mul1 = sum1 * C[1];
//                sol[INDX(row_size, col_size, k, j, i)] = mul0 + mul1;
//
