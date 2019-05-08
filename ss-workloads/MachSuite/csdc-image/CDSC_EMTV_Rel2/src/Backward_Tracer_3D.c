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
#include "Backward_Tracer_3D.h"
#include "utilities.h"

extern int  threadnum;  //For CPU Routine, single thread is default

unsigned int mem_load;
unsigned int mem_store;

pthread_t backwardproj_thread[MAX_THREAD_NUM];

void Ray_Tracer_Backward_CPU(DataType sx, //the postion for suorce and detecto x, y,z
                             DataType sy,
                             DataType sz,
                             DataType dx,
                             DataType dy,
                             DataType dz,
                             int N_x, //iimage size x,y,z
                             int N_y,
                             int N_z,
                             DataType value, //sinogram value for current position
                             DataType *imageDataPtr,
                             DataType *image_denoteDataPtr)
//the function is used to perform backward projection: x=A'*b
//this process is the invert of forward projection
//please refer to paper: please refer to paper: Fast Ray-Tracing Technique to Calculate Line Integral Paths in Voxel Arrays
{
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
    Vector3DInt  v;
    int index;

    //get the direction of ray from source to detector, detector position minus source position
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

    //increment in x, y and z direction
    absvalue_x = ABS_VALUE(Ray_Dir.x);
    absvalue_y = ABS_VALUE(Ray_Dir.y);
    absvalue_z = ABS_VALUE(Ray_Dir.z);
    //get x=1 Lx Ly Lz, calculate the parameter intersection lengths between two neighbouring x, y, z planes
    //the dx, dy, dz is considered to be 1. The whole equation should be Lx=L*dx/absvalue_x
    Len.x = (absvalue_x > 1.e-4) ? (L / absvalue_x) * vx : 1.e6;
    Len.y = (absvalue_y > 1.e-4) ? (L / absvalue_y) * vy : 1.e6;
    Len.z = (absvalue_z > 1.e-4) ? (L / absvalue_z) * vz : 1.e6;


    //get the entry and exit point between Ray & Image
    //distance between source and entry point
    DataType tempx;
    DataType tempy;
    DataType tempz;

    tempx = N_x * vx;
    tempy = N_y * vy;
    tempz = N_z * vz;

    //lambda_min and lambda_max indicate the entry point and exit point of the line with the array
    //lambaa_min=max[lambda0_x, lambda0_y, lambda0_z]
    //lambaa_max=min[lambdaN_x, lambdaN_y, lambdaN_z]

    lambda0 = LAMBDA_X(0, sx, dx, L); //=sx/(sx-dx)
    lambdaN = LAMBDA_X(tempx, sx, dx, L); //=(tempx*L-sx)/(dx-sx)
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


    //calculate the indices o f the starting point V(v.x,v.y,v.z)
    //lambda.x , lambda.y, lambda,z are the length from current position to the start point
    // start x plane
    if (index == 1)
    {
        if (signx == 1)
            v.x = 0;
        else
            //calculate the index x
            v.x = N_x - 1;
        //update the length Lx
        lambda.x = lambda0 + Len.x;
        //calculate the index y
        v.y = (sy + lambda0 * Ray_Dir.y / L) / vy;
        tempy = v.y * vy;
        //update the length Ly
        lambda.y = (absvalue_y < 1.e-4) ? 1.e6 : LAMBDA_Y(tempy + (signy > 0) * vy, sy, dy, L);
        //calculate the index z
        v.z = (sz + lambda0 * Ray_Dir.z / L) / vz;
        tempz = v.z * vz;
        //update the length Ly
        lambda.z = (absvalue_z < 1.e-4) ? 1.e6 : LAMBDA_Z(tempz + (signz > 0) * vz, sz, dz, L);
    }
    // start y plane
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
    //  start z plane
    else // if (index == 3)
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

    /// the main loop
    while (lambda0 < lambda_max - 5.e-2)
    {
        if (lambda.x <= lambda.y && lambda.x <= lambda.z)
        {
            mem_store++;
            mem_load++;
            //calculate the weighted value for image data according to the intersection lengh of the ray in current pixel
            if (Data(image_denoteDataPtr, N_x, N_y, N_z, v.x, v.y, v.z) > 0)
                Data(imageDataPtr, N_x, N_y, N_z, v.x, v.y, v.z) += (lambda.x - lambda0) * value;
            lambda0   = lambda.x;
            lambda.x += Len.x ;
            v.x      += signx;
        }
        else if (lambda.y <= lambda.z)
        {
            mem_store++;
            mem_load++;
            //calculate the weighted value for image data according to the intersection lengh of the ray in current pixel
            if (Data(image_denoteDataPtr, N_x, N_y, N_z, v.x, v.y, v.z) > 0)
                Data(imageDataPtr, N_x, N_y, N_z, v.x, v.y, v.z) += (lambda.y - lambda0) * value;
            lambda0   = lambda.y;
            lambda.y += Len.y;
            v.y      += signy;
        }
        else
        {
            mem_store++;
            mem_load++;
            //calculate the weighted value for image data according to the intersection lengh of the ray in current pixel
            if (Data(image_denoteDataPtr, N_x, N_y, N_z, v.x, v.y, v.z) > 0)
                Data(imageDataPtr, N_x, N_y, N_z, v.x, v.y, v.z) += (lambda.z - lambda0) * value;
            lambda0   = lambda.z;
            lambda.z += Len.z;
            v.z      += signz;
        }
    }
    return;
}

void CPU_Mem_Set(DataType *dataPtr, DataType value, int size)
{
    int i;
    for (i = 0; i < size; i++)
    {
        dataPtr[i] = value;
    }
}


#ifdef MULTITHREAD
void backwardproj(emtv3d_bcom *com)
{
    int i, j, k;
    Vector3D source, detector;
    DataType value;
    DataType *imageDataPtr = com->imageDataPtr;
    DataType *image_denoteDataPtr = com->image_denoteDataPtr;
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
    int offset;

#ifdef SET_CPU_AFFINITY  //set CPU binding on CPU 
    setcpu(tid);
#endif
    i = com->i;
    source.z = initialz + TablePosition[i];
    //  printf("multi %f\n",source.z);
    source.x = DistSource * sourcecosf[i] + LenofInt;
    source.y = DistSource * sourcesinf[i] + LenofInt;
	
	//printf("pos_backwards: %d, %f, %f, %f\n", i, source.x,source.y,source.z);
    //for (j = -q; j < q + 1; j++)
    //0 8 16 24
    //printf("i,z,x,y: %d %f  %f   %f\n",i,source.z,source.x,source.y);
    for (offset = 0; offset < INTERVAL; offset++)
    {
        for (j = -Nofdetector + tid * INTERVAL + offset; j < Nofdetector ; j += INTERVAL * threadnum) //[-150,150] // 301 blockx
        {
            detector.x = DistDetector * (detectorcosf[j + Nofdetector] * sourcecosf[i] - detectorsinf[j + Nofdetector] * sourcesinf[i]) + source.x;
            detector.y = DistDetector * (detectorsinf[j + Nofdetector] * sourcecosf[i] + detectorcosf[j + Nofdetector] * sourcesinf[i]) + source.y;
            //      printf("X-Y-D: %f %f\n",detector.x,detector.y);

            for (k = -Nofdetectorz; k < Nofdetectorz; k++)
            {
                mem_load++;
                value = Data(sinoDataPtr, sinoindex1Size, sinoindex2Size, sinoindex3Size, (j + Nofdetector), (k + Nofdetectorz), i);
                detector.z = k * Lenofdetectorz + source.z;
                if ( value > 1.e-4)
                    Ray_Tracer_Backward_CPU(source.x, source.y, source.z,
                                            detector.x, detector.y, detector.z,
                                            imageindex1Size, imageindex2Size, imageindex3Size,
                                            value,
                                            imageDataPtr,
                                            image_denoteDataPtr);
            }
        }
    }
}

void Backward_Projection_CPU( DataType *imageDataPtr,          //output
                              DataType *image_denoteDataPtr, //image mask
                              int imageindex1Size,//image information
                              int imageindex2Size,
                              int imageindex3Size,
                              DataType *sinoDataPtr,//sinogram data and size information
                              int sinoindex1Size,
                              int sinoindex2Size,
                              int sinoindex3Size,
                              DataType *sourcecosf,//position information for source and detector
                              DataType *sourcesinf,
                              DataType *detectorcosf,
                              DataType *detectorsinf,
							  DataType *TablePosition)
{
    int ret;
    int tid;
    int imagesize = imageindex1Size * imageindex2Size * imageindex3Size;
    CPU_Mem_Set(imageDataPtr, 0.0, imagesize);
    int NS = Num_of_Source;
    int i;

    for (tid = 0; tid < threadnum; tid++)
    {
        bcom[tid].threadid = tid;
        bcom[tid].imageDataPtr = imageDataPtr;
        bcom[tid].image_denoteDataPtr = image_denoteDataPtr;
        bcom[tid].imageindex1Size = imageindex1Size;
        bcom[tid].imageindex2Size = imageindex2Size;
        bcom[tid].imageindex3Size = imageindex3Size;
        bcom[tid].sinoDataPtr = sinoDataPtr;
        bcom[tid].sinoindex1Size = sinoindex1Size;
        bcom[tid].sinoindex2Size = sinoindex2Size;
        bcom[tid].sinoindex3Size = sinoindex3Size;
    }

    mem_store = 0;
    mem_load = 0;

    for (i = 0; i < NS + 1; i++)
    {

        for (tid = 0; tid < threadnum; tid++)
        {
            bcom[tid].i = i;
            //create forward projection thread
            if ((ret = pthread_create(&backwardproj_thread[tid], NULL, (void *)backwardproj, (void *)&bcom[tid])) != 0)
                printf("Forward Projection %d thread create failed!\n", tid);
        }

        for (tid = 0; tid < threadnum; tid++)
        {
            pthread_join(backwardproj_thread[tid], NULL);
        }
    }
}

#else

extern DataType TablePosition[Num_of_Source + 1];
void Backward_Projection_CPU_kernel( DataType *imageDataPtr,          //output
                                     DataType *image_denoteDataPtr,
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
                                     int UnderSample)
{
    int N_x = imageindex1Size;
    int N_y = imageindex2Size;
    int N_z = imageindex3Size;
    int NS = Num_of_Source;
    int i, j, k;
    DataType value;
    Vector3D source, detector;

    //printf("NS %d q %d qz %d \n", NS, q ,qz);
    //NS 35 q 150 qz 128
    for (i = 0; i <= NS; i++)
    {
        source.z = initialz + TablePosition[i];
        source.x = DistSource * sourcecosf[i] + LenofInt;
        source.y = DistSource * sourcesinf[i] + LenofInt;
        for (j = -Nofdetector; j < Nofdetector; j++)
        {
            detector.x = DistDetector * detectorcosf[i * 512 + j + Nofdetector] + source.x;
            detector.y = DistDetector * detectorsinf[i * 512 + j + Nofdetector] + source.y;
            for (k = -Nofdetectorz; k < Nofdetectorz; k++)
            {
                value = Data(sinoDataPtr, sinoindex1Size, sinoindex2Size, sinoindex3Size, (j + Nofdetector), (k + Nofdetectorz), i);
                detector.z = (k + 0.5) * Lenofdetectorz + source.z;
                if ( value > 1.e-4)
                    Ray_Tracer_Backward_CPU(source.x, source.y, source.z,
                                            detector.x, detector.y, detector.z,
                                            N_x, N_y, N_z,
                                            value,
                                            imageDataPtr,
                                            image_denoteDataPtr);
            }
        }
        DBG1(fprintf(stdout, "Backward_Projection %d \r", i);)
    }
}

void Backward_Projection_CPU( DataType *imageDataPtr,          //output
                              DataType *image_denoteDataPtr,
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
                              DataType *detectorsinf)
{
    int imagesize = imageindex1Size * imageindex2Size * imageindex3Size;
    CPU_Mem_Set(imageDataPtr, 0.0, imagesize);
    Backward_Projection_CPU_kernel(imageDataPtr,
                                   image_denoteDataPtr,
                                   imageindex1Size,
                                   imageindex2Size,
                                   imageindex3Size,
                                   sinoDataPtr,
                                   sinoindex1Size,
                                   sinoindex2Size,
                                   sinoindex3Size,
                                   sourcecosf,
                                   sourcesinf,
                                   detectorcosf,
                                   detectorsinf
                                   UnderSample);
}
#endif
