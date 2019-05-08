/******************************************************************************
 * Ray_Tracer_3D.cpp: EMTV for 3D image reconstruction
 *
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version :
 ******************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "define.h"
#include "Forward_Tracer_3D.h"
#include "utilities.h"


extern int  threadnum;  //For CPU Routine, single thread is default

void writeline(int x, int y, int z, DataType length)
{
    FILE *outputfile;
#ifdef SINGLEPOINT
    outputfile = fopen("sinaupdate_float.dat", "a");
#else
    outputfile = fopen("sinaupdate_double.dat", "a");
#endif
    if ( outputfile == NULL)
    {
        printf("Cannot open the output file!\n");
        exit(1);
    }
#ifdef SINGLEPOINT
    fprintf(outputfile, "%3d %3d %3d %f \n", x, y, z, length);
#else
    fprintf(outputfile, "%3d %3d %3d %lf \n", x, y, z, length);
#endif

    fclose(outputfile);
}

/*
 * ************************************************
 * Function: Tracer Forward
 * Input:  source
 *         detector
 *         image
 * Output: Return Value
 * Return:
 *************************************************
 */
void Ray_Tracer_Forward_CPU(DataType sx,//the postion for suorce and detecto x, y,z
                            DataType sy,
                            DataType sz,
                            DataType dx,
                            DataType dy,
                            DataType dz,
                            int N_x,
                            int N_y,
                            int N_z,
                            DataType *imageDataPtr,//image data
                            DataType *sino)// sinogram data
{
    //the function is used to perform forward projection: A*x=b
    //this process is the invert of backward projection, please refer to backward projection function for more documents details
    //please refer to paper: Fast Ray-Tracing Technique to Calculate Line Integral Paths in Voxel Arrays
    Vector3D Ray_Dir;
    DataType L;
    int signx, signy, signz;
    Vector3D Len;
    DataType absvalue_x;
    DataType absvalue_y;
    DataType absvalue_z;
    DataType lambda_min = 0.0;
    DataType lambda_max;
    DataType lambda0, lambdaN;
    DataType temp;
    Vector3D lambda;
    Vector3DInt v;
    int index = 0;

    //get the direction of ray from source to detector
    Ray_Dir.x = dx - sx;
    Ray_Dir.y = dy - sy;
    Ray_Dir.z = dz - sz;

    //get the length of the ray
    L = Ray_Dir.x * Ray_Dir.x + Ray_Dir.y * Ray_Dir.y + Ray_Dir.z * Ray_Dir.z;
    L = SQRT(L);
    lambda_max = L;

    //the the direction of increment in x, y and z
    signx = (Ray_Dir.x > 0) ? 1 : -1;
    signy = (Ray_Dir.y > 0) ? 1 : -1;
    signz = (Ray_Dir.z > 0) ? 1 : -1;
    //    printf("sign %d %d %d \n",signx,signy,signz);
    //increment in x, y and z direction
    absvalue_x = ABS_VALUE(Ray_Dir.x);
    absvalue_y = ABS_VALUE(Ray_Dir.y);
    absvalue_z = ABS_VALUE(Ray_Dir.z);
    //get x=1 Lx Ly Lz
    Len.x = (absvalue_x > 1.e-4) ? (L / absvalue_x) * vx : 1.e6;
    Len.y = (absvalue_y > 1.e-4) ? (L / absvalue_y) * vy : 1.e6;
    Len.z = (absvalue_z > 1.e-4) ? (L / absvalue_z) * vz : 1.e6;

    *sino = 0.0;

    //get the entry and exit point between Ray & Image
    //distance between source and entry point
    DataType tempx;
    DataType tempy;
    DataType tempz;

    tempx = N_x * vx;
    tempy = N_y * vy;
    tempz = N_z * vz;

    lambda0 = LAMBDA_X(0, sx, dx, L);
    lambdaN = LAMBDA_X(tempx, sx, dx, L);
    temp    = MIN(lambda0, lambdaN);
    lambda_min = MAX(lambda_min, temp);
    if (lambda_min == temp)
        index = 1;
    temp    = MAX(lambda0, lambdaN);
    lambda_max = MIN(lambda_max, temp);  // start x plane

    lambda0 = LAMBDA_Y(0, sy, dy, L);
    lambdaN = LAMBDA_Y(tempy, sy, dy, L);
    temp    = MIN(lambda0, lambdaN);
    lambda_min = MAX(lambda_min, temp);
    if (lambda_min == temp)
        index = 2;
    temp    = MAX(lambda0, lambdaN);
    lambda_max = MIN(lambda_max, temp);   // start y plane

    lambda0 = LAMBDA_Z(0, sz, dz, L);
    lambdaN = LAMBDA_Z(tempz, sz, dz, L);
    temp    = MIN(lambda0, lambdaN);
    lambda_min = MAX(lambda_min, temp);
    if (lambda_min == temp)
        index = 3;
    temp    = MAX(lambda0, lambdaN);
    lambda_max = MIN(lambda_max, temp);  //  start z plane

    if (lambda_min >= lambda_max)
        return;

    lambda0 = lambda_min;   // lambda = lambda_min
    if (index == 1)
    {
        if (signx == 1)
            v.x = 0;
        else
            v.x = N_x - 1;
        lambda.x = lambda0 + Len.x;

        v.y = (sy + lambda0 * Ray_Dir.y / L) / vy;
        tempy = v.y * vy;
        lambda.y = (absvalue_y < 1.e-4) ? 1.e6 : LAMBDA_Y(tempy + (signy > 0) * vy, sy, dy, L);

        v.z = (sz + lambda0 * Ray_Dir.z / L) / vz;
        tempz = v.z * vz;
        lambda.z = (absvalue_z < 1.e-4) ? 1.e6 : LAMBDA_Z(tempz + (signz > 0) * vz, sz, dz, L);
    }
    else if (index == 2)
    {
        if (signy == 1)
            v.y = 0;
        else
            v.y = N_y - 1;
        lambda.y = lambda0 + Len.y;

        v.x = (sx + lambda0 * Ray_Dir.x / L) / vx;
        tempx = v.x * vx;
        lambda.x = (absvalue_x < 1.e-4) ? 1.e6 : LAMBDA_X(tempx + (signx > 0) * vx, sx, dx, L);

        v.z = (sz + lambda0 * Ray_Dir.z / L) / vz;
        tempz = v.z * vz;
        lambda.z = (absvalue_z < 1.e-4) ? 1.e6 : LAMBDA_Z(tempz + (signz > 0) * vz, sz, dz, L);
    }
    else  // if (index == 3)
    {
        if (signz == 1)
            v.z = 0;
        else
            v.z = N_z - 1;
        lambda.z = lambda0 + Len.z;

        v.x = (sx + lambda0 * Ray_Dir.x / L) / vx;
        tempx = v.x * vx;
        lambda.x = (absvalue_x < 1.e-4) ? 1.e6 : LAMBDA_X(tempx + (signx > 0) * vx, sx, dx, L);

        v.y = (sy + lambda0 * Ray_Dir.y / L) / vy;
        tempy = v.y * vy;
        lambda.y = (absvalue_y < 1.e-4) ? 1.e6 : LAMBDA_Y(tempy + (signy > 0) * vy, sy, dy, L);
    }
    //  printf("V %d %d %d \n",v.x,v.y,v.z);

    while (lambda0 < lambda_max - 5.e-2)
    {
        if (lambda.x <= lambda.y && lambda.x <= lambda.z)
        {
            (*sino)  += (lambda.x - lambda0) * Data(imageDataPtr, N_x, N_y, N_z, v.x, v.y, v.z);
            lambda0   = lambda.x;
            lambda.x += Len.x;
            v.x      += signx;
        }
        else if (lambda.y <= lambda.z)
        {
            (*sino)  += (lambda.y - lambda0) * Data(imageDataPtr, N_x, N_y, N_z, v.x, v.y, v.z);
            lambda0   = lambda.y;
            lambda.y += Len.y;
            v.y      += signy;
        }
        else
        {
            (*sino)  += (lambda.z - lambda0) * Data(imageDataPtr, N_x, N_y, N_z, v.x, v.y, v.z);
            lambda0   = lambda.z;
            lambda.z += Len.z;
            v.z      += signz;
        }
        //  printf("V %d %d %d\n",v.x,v.y,v.z);
        //  printf("Lambda0 %f Lambda_Max %f Diff %f\n",lambda0,lambda_max,lambda_max-lambda0);
    }
    return;
}


/*
 * ************************************************
 * Function: Forward Prejection
 * Input:  image
 * Output: sino
 * Return:
 *************************************************
 */
#ifdef MULTITHREAD
void forwardproj(emtv3d_fcom *com)
{
    int i, j, k;
    int NS = Num_of_Source;
    Vector3D source, detector;
    DataType *imageDataPtr = com->imageDataPtr;
    int imageindex1Size = com->imageindex1Size;
    int imageindex2Size = com->imageindex2Size;
    int imageindex3Size = com->imageindex3Size;
    DataType *sinoDataPtr = com->sinoDataPtr;
    int sinoindex1Size = com->sinoindex1Size;
    int sinoindex2Size = com->sinoindex2Size;
    int sinoindex3Size = com->sinoindex3Size;
    DataType *sourcecosf = com->sourcecosf;
    DataType *sourcesinf = com->sourcesinf;
    DataType *detectorcosf = com->detectorcosf;
    DataType *detectorsinf = com->detectorsinf;
	DataType *TablePosition = com->TablePosition;
    int tid = com->threadid;

#ifdef SET_CPU_AFFINITY  //set CPU binding on CPU 
    setcpu(tid);
#endif

    int start =  tid * (NS + 1) / threadnum;
    int end   =  (tid + 1) * (NS + 1) / threadnum;

    end   = (end > (NS + 1)) ? (NS + 1) : end;
    //printf("NS %d q %d qz %d \n", NS, q ,qz);
    //NS 35 q 150 qz 128
    for (i = start; i < end; i++)
    {

		source.z = initialz + TablePosition[i];
        source.x = DistSource * sourcecosf[i] + LenofInt;
        source.y = DistSource * sourcesinf[i] + LenofInt;
        //printf("i,z,x,y: %d %f  %f   %f\n",i,source.z,source.x,source.y);
        for (j = -Nofdetector; j < Nofdetector; j++)
        {
            //printf("JJJ %d\n",j);
            detector.x = DistDetector * (detectorcosf[j + Nofdetector] * sourcecosf[i] - detectorsinf[j + Nofdetector] * sourcesinf[i]) + source.x;
            detector.y = DistDetector * (detectorsinf[j + Nofdetector] * sourcecosf[i] + detectorcosf[j + Nofdetector] * sourcesinf[i]) + source.y;

            k = 0;
            for (k = -Nofdetectorz; k < Nofdetectorz; k++) // -128  128+1
            {
                //printf("KKK %d\n",k);
                DataType aa;
                detector.z = k * Lenofdetectorz + source.z;
                //printf("III %d\n",i);
				//if(detector.z >= 0){
					Ray_Tracer_Forward_CPU(source.x,
										   source.y,
										   source.z,
										   detector.x,
										   detector.y,
										   detector.z,
										   imageindex1Size,
										   imageindex2Size,
										   imageindex3Size,
										   imageDataPtr,
										   &aa);
					//printf("III %d %d %d %d\n",i,sinoindex1Size, sinoindex2Size, sinoindex3Size);
					Data(sinoDataPtr, sinoindex1Size, sinoindex2Size, sinoindex3Size, (j + Nofdetector), (k + Nofdetectorz), i) = aa;
				//}
            }
        }
    }
    //fprintf(stdout, "Forward_Projection Thread %d Processing %d ~ %d \n", tid, start, end);
}

void Forward_Projection_CPU(DataType *imageDataPtr,
                            int imageindex1Size,
                            int imageindex2Size,
                            int imageindex3Size,
                            DataType *sinoDataPtr,
                            int sinoindex1Size,
                            int sinoindex2Size,
                            int sinoindex3Size,
                            DataType *sourcecosf,
                            DataType *sourcesinf,
                            DataType *detectorcosf,
                            DataType *detectorsinf,
							DataType *TablePosition)
{
    int ret;
    int tid;

    for (tid = 0; tid < threadnum; tid++)
    {
        fcom[tid].threadid = tid;
        fcom[tid].imageDataPtr = imageDataPtr;
        fcom[tid].imageindex1Size = imageindex1Size;
        fcom[tid].imageindex2Size = imageindex2Size;
        fcom[tid].imageindex3Size = imageindex3Size;
        fcom[tid].sinoDataPtr = sinoDataPtr;
        fcom[tid].sinoindex1Size = sinoindex1Size;
        fcom[tid].sinoindex2Size = sinoindex2Size;
        fcom[tid].sinoindex3Size = sinoindex3Size;

        //create forward projection thread
        if ((ret = pthread_create(&forwardproj_thread[tid], NULL, (void *)forwardproj, (void *)&fcom[tid])) != 0)
            printf("Forward Projection %d thread create failed!\n", tid);
    }

    for (tid = 0; tid < threadnum; tid++)
    {
        pthread_join(forwardproj_thread[tid], NULL);
    }
}

#else

extern DataType TablePosition[Num_of_Source + 1];
void Forward_Projection_CPU(DataType *imageDataPtr,
                            int imageindex1Size,
                            int imageindex2Size,
                            int imageindex3Size,
                            DataType *sinoDataPtr,   //output
                            int sinoindex1Size,
                            int sinoindex2Size,
                            int sinoindex3Size,
                            DataType *sourcecosf,
                            DataType *sourcesinf,
                            DataType *detectorcosf,
                            DataType *detectorsinf)

{

    int N_x = imageindex1Size;
    printf("Imageindex1Size  %d\n", N_x);
    int N_y = imageindex2Size;
    int N_z = imageindex3Size;
    Vector3D source, detector;
    int NS = Num_of_Source;
    int i, j, k;

    //        printf("NS %d\n", NS);
    //NS 35 q 150 qz 128
    for (i = 0; i <= NS; i++)
    {

        source.z = initialz + TablePosition[i];
        //      printf("%f",source.z);
        source.x = DistSource * sourcecosf[i] + LenofInt;
        source.y = DistSource * sourcesinf[i] + LenofInt;
		//printf("pos_forwards: %f, %f, %f\n", source.x,source.y,source.z);
        for (j = -Nofdetector; j < Nofdetector; j++)
        {
            detector.x = DistDetector * detectorcosf[i * 512 + j + Nofdetector] + source.x;
            detector.y = DistDetector * detectorsinf[i * 512 + j + Nofdetector] + source.y;
            for (k = -Nofdetectorz; k < Nofdetectorz; k++) // -128  128+1
            {
                DataType aa;
                detector.z = (k + 0.5) * Lenofdetectorz + source.z;

                Ray_Tracer_Forward_CPU(source.x,
                                       source.y,
                                       source.z,
                                       detector.x,
                                       detector.y,
                                       detector.z,
                                       N_x,
                                       N_y,
                                       N_z,
                                       imageDataPtr,
                                       &aa);
                Data(sinoDataPtr, sinoindex1Size, sinoindex2Size, sinoindex3Size, (j + Nofdetector), (k + Nofdetectorz), i) = aa;
            }
        }
        DBG1(fprintf(stdout, "Forward_Projection %d \r", i);)

    }
}
#endif
