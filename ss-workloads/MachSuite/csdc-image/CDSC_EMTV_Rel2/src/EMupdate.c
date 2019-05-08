/******************************************************************************
 * EMupdate.cpp: EMTV for 3D image reconstruction
 *
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version :
 ******************************************************************************/

#include "EMupdate.h"
#include "Forward_Tracer_3D.h"
#include "Backward_Tracer_3D.h"
#include <inttypes.h>
#include <stdio.h>

void divfunc(DataType *gAfDataPtr, DataType *sinoDataPtr, DataType *AfDataPtr,  int M_x, int M_y, int M_z)
{
    int j, k, l;
    for (j = 0; j < M_x; j ++)
        for (k = 0; k < M_y; k ++)
            for (l = 0; l < M_z; l ++)
            {
                if ( Data(AfDataPtr,M_x,M_y,M_z,j,k,l) > 1.e-4)
                    Data(gAfDataPtr, M_x, M_y, M_z, j, k, l) = Data(sinoDataPtr, M_x, M_y, M_z, j, k, l) / Data(AfDataPtr, M_x, M_y, M_z, j, k, l);
            }
}

void multidiv(DataType *imageDataPtr, DataType *AtAfDataPtr, DataType *sumADataPtr, int N_x, int N_y, int N_z)
{
    int j, k, l;
    for (j = 0; j < N_x; j ++)
        for (k = 0; k < N_y; k ++)
            for (l = 0; l < N_z; l ++)
                if ( Data(sumADataPtr, N_x, N_y, N_z, j, k, l) > 1.e-1)
                {
                    Data(imageDataPtr, N_x, N_y, N_z, j, k, l) = Data(imageDataPtr, N_x, N_y, N_z, j, k, l) * Data(AtAfDataPtr, N_x, N_y, N_z, j, k, l) / Data(sumADataPtr, N_x, N_y, N_z, j, k, l);
                }
}

void EMupdate_CPU (int sinoindex1Size,//sinagram data information inculding size and data
                   int sinoindex2Size,
                   int sinoindex3Size,
                   DataType *sinoDataPtr,
                   int imageindex1Size,//image data information including size and data, output
                   int imageindex2Size,
                   int imageindex3Size,
                   DataType *imageDataPtr,
                   DataType *image_denoteDataPtr, //image mask
                   DataType *sumADataPtr, //sum for each column, refer to EM equation
                   int max_iter,  //the number of iterations
                   int Afindex1Size,//the intermedia calculation results
                   int Afindex2Size,
                   int Afindex3Size,
                   DataType *AfDataPtr,
                   int gAfindex1Size,
                   int gAfindex2Size,
                   int gAfindex3Size,
                   DataType *gAfDataPtr,
                   int AtAfindex1Size,
                   int AtAfindex2Size,
                   int AtAfindex3Size,
                   DataType *AtAfDataPtr,
                   DataType *sourcecosf,//position information related to source and detector
                   DataType *sourcesinf,
                   DataType *detectorcosf,
                   DataType *detectorsinf,
				   DataType *TablePosition)
{
    //EM regulation process refer to paper: Expectation Maximization and Total Variation Based Model for Computed Tomography Reconstruction from
    //Undersampled Data; equation (2)
    int i;
#ifdef EMupdate_DEBUG
    char filename[100];
#endif
    int64_t i_start, i_end;

    for (i = 0; i < max_iter; i ++)
    {
        //         printf("Number of Iterations in EM %d\n",i);
        //printf("i=%d, imageindex1Size %d,%d,%d,%d,%d,%d\n ", i, imageindex1Size, imageindex2Size, imageindex3Size, Afindex1Size, Afindex2Size, Afindex3Size);
        //get the denominator in the em update equation as (Ax)i
        i_start = emtv3d_timer_us();
        Forward_Projection_CPU(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size,
                               AfDataPtr, Afindex1Size, Afindex2Size, Afindex3Size,
                               sourcecosf,
                               sourcesinf,
                               detectorcosf,
                               detectorsinf, TablePosition);
        i_end = emtv3d_timer_us();
        //printf("  Forward %lld us\n", (i_end - i_start));

#ifdef EMupdate_DEBUG
        sprintf(filename, "../data/gpu_emupdate_%d_Af.dat", i);
        writeImgArray3DDataPtr(filename, AfDataPtr, Afindex1Size, Afindex2Size, Afindex3Size);
#endif
        //M_x=301, M_y=257, M_z=36
        //get the refined pixel as (b_i/Ax_i)
		//printf("divfunc\n");
        divfunc(gAfDataPtr, sinoDataPtr, AfDataPtr, sinoindex1Size, sinoindex2Size, sinoindex3Size);
		//printf("divfunc done\n");
#ifdef EMupdate_DEBUG
        sprintf(filename, "../data/gpu_emupdate_%d_gAf.dat", i);
        writeImgArray3DDataPtr(filename, gAfDataPtr, gAfindex1Size, gAfindex2Size, gAfindex3Size);
#endif

        //get the numerator of the of the EM update equation sum(a_i_j*(b_i/Ax_i)), i range from i to M
        i_start = emtv3d_timer_us();
        //printf("backward\n");
		Backward_Projection_CPU(AtAfDataPtr, image_denoteDataPtr,
                                AtAfindex1Size,
                                AtAfindex2Size,
                                AtAfindex3Size,
                                gAfDataPtr,
                                gAfindex1Size,
                                gAfindex2Size,
                                gAfindex3Size,
                                sourcecosf,
                                sourcesinf,
                                detectorcosf,
                                detectorsinf, TablePosition);
        i_end = emtv3d_timer_us();
        //printf("  Backward %lld us\n", (i_end - i_start));

#ifdef EMupdate_DEBUG
        sprintf(filename, "../data/gpu_emupdate_%d_AtAf.dat", i);
        writeImgArray3DDataPtr(filename, AtAfDataPtr, AtAfindex1Size, AtAfindex2Size, AtAfindex3Size);
#endif

        //N_x=128, N_y=128, N_z=128
        //get the final EM upadate result according to the whole EM iterative equation
        //printf("multidiv");
		multidiv(imageDataPtr, AtAfDataPtr, sumADataPtr, imageindex1Size, imageindex2Size, imageindex3Size);
		//printf("multidiv...done");
#ifdef EMupdate_DEBUG
        sprintf(filename, "../data/gpu_emupdate_%d_image.dat", i);
        writeImgArray3DDataPtr(filename, imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size);
#endif
    }
}
