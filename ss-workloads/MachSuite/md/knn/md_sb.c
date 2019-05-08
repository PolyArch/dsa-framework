/*
Implemenataion based on:
A. Danalis, G. Marin, C. McCurdy, J. S. Meredith, P. C. Roth, K. Spafford, V. Tipparaju, and J. S. Vetter.
The scalable heterogeneous computing (shoc) benchmark suite.
In Proceedings of the 3rd Workshop on General-Purpose Computation on Graphics Processing Units, 2010.
*/

#include "md.h"
#include "md_sb.dfg.h"
#include "../../../common/include/ss_insts.h"

void md_kernel(TYPE force_x[nAtoms],
               TYPE force_y[nAtoms],
               TYPE force_z[nAtoms],
               TYPE position_x[nAtoms],
               TYPE position_y[nAtoms],
               TYPE position_z[nAtoms],
               int64_t NL[nAtoms*maxNeighbors])
{
    //TYPE delx, dely, delz, r2inv;
    //TYPE r6inv, potential, force, j_x, j_y, j_z;
    TYPE i_x, i_y, i_z;//, fx, fy, fz;

    int32_t i;//, j, jidx;

    SS_CONFIG(md_sb_config,md_sb_size);

    //SS_DMA_READ(&NL[0],8,8,nAtoms*maxNeighbors,P_IND_TRIP0);
    //SS_INDIRECT64(P_IND_TRIP0,&position_x[0],nAtoms*maxNeighbors,P_md_sb_jx);
    //SS_INDIRECT64(P_IND_TRIP1,&position_y[0],nAtoms*maxNeighbors,P_md_sb_jy);
    //SS_INDIRECT64(P_IND_TRIP2,&position_z[0],nAtoms*maxNeighbors,P_md_sb_jz);

    loop_i : for (i = 0; i < nAtoms; i++){
         i_x = position_x[i];
         i_y = position_y[i];
         i_z = position_z[i];

         SS_DMA_READ(&NL[i*maxNeighbors],8,8,maxNeighbors,P_IND_TRIP0);
         SS_INDIRECT(P_IND_TRIP0,&position_x[0],maxNeighbors,P_md_sb_jx);
         SS_INDIRECT(P_IND_TRIP1,&position_y[0],maxNeighbors,P_md_sb_jy);
         SS_INDIRECT(P_IND_TRIP2,&position_z[0],maxNeighbors,P_md_sb_jz);

         //SS_DMA_READ(&NL[i*maxNeighbors],8,8,maxNeighbors,P_IND_1);
         //SS_INDIRECT64(P_IND_1,&position_x[0],maxNeighbors,P_md_sb_jx);
         //SS_DMA_READ(&NL[i*maxNeighbors],8,8,maxNeighbors,P_IND_2);
         //SS_INDIRECT64(P_IND_2,&position_y[0],maxNeighbors,P_md_sb_jy);
         //SS_DMA_READ(&NL[i*maxNeighbors],8,8,maxNeighbors,P_IND_3);
         //SS_INDIRECT64(P_IND_3,&position_z[0],maxNeighbors,P_md_sb_jz);

         //SS_DMA_READ(&NL[i*maxNeighbors],8,8,maxNeighbors,P_md_sb_jx);
         //SS_DMA_READ(&NL[i*maxNeighbors],8,8,maxNeighbors,P_md_sb_jy);
         //SS_DMA_READ(&NL[i*maxNeighbors],8,8,maxNeighbors,P_md_sb_jz);

         SS_CONST(P_md_sb_ix,i_x,maxNeighbors);
         SS_CONST(P_md_sb_iy,i_y,maxNeighbors);
         SS_CONST(P_md_sb_iz,i_z,maxNeighbors);

         SS_CONST(P_md_sb_reset,0,maxNeighbors-1);
         SS_CONST(P_md_sb_reset,1,1);

         SS_GARBAGE(P_md_sb_A1,maxNeighbors-1);
         SS_GARBAGE(P_md_sb_A2,maxNeighbors-1);
         SS_GARBAGE(P_md_sb_A3,maxNeighbors-1);

         SS_DMA_WRITE(P_md_sb_A1,8,8,1,&force_x[i]);
         SS_DMA_WRITE(P_md_sb_A2,8,8,1,&force_y[i]);
         SS_DMA_WRITE(P_md_sb_A3,8,8,1,&force_z[i]);

         //loop_j : for( j = 0; j < maxNeighbors; j++){
         //    // Get neighbor
         //    jidx = NL[i*maxNeighbors + j];

         //    // Look up x,y,z positions
         //    j_x = position_x[jidx];
         //    j_y = position_y[jidx];
         //    j_z = position_z[jidx];
         //    // Calc distance
         //    delx = i_x - j_x;
         //    dely = i_y - j_y;
         //    delz = i_z - j_z;
         //    r2inv = 1.0/(delx*delx + dely*dely + delz*delz );
         //    // Assume no cutoff and aways account for all nodes in area
         //    r6inv = r2inv * r2inv * r2inv;
         //    potential = r6inv*(lj1*r6inv - lj2);
         //    // Sum changes in force
         //    force = r2inv*potential;
         //    fx += delx * force;
         //    fy += dely * force;
         //    fz += delz * force;
         //}

         //Update forces after all neighbors accounted for.
         //force_x[i] = fx;
         //force_y[i] = fy;
         //force_z[i] = fz;
         //printf("dF=%lf,%lf,%lf\n", fx, fy, fz);
    }
    SS_WAIT_ALL();
}
