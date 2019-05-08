/******************************************************************************
 * Ray_Tracer_3D.h: EMTV for 3D image reconstruction
 *   
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version : 
 ******************************************************************************/

#ifndef __Forward_Tracer_3D__
#define __Forward_Tracer_3D__
#include <pthread.h>

#ifndef WIN32
#include "../config.h"
#endif

#include "define.h"
#include "Array3D.h"
#include "Vector.h"




#ifdef GPU_ROUTINE        
void Forward_Projection_GPU(DataType *imageDataPtr,
                            int imageindex1Size,
                            int imageindex2Size,
                            int imageindex3Size,
                            DataType *sinoDataPtr,
                            int sinoindex1Size,
                            int sinoindex2Size,
                            int sinoindex3Size,   
							DataType *TablePosition,
                            DataType *sourcecosf,
                            DataType *sourcesinf,
                            DataType *detectorcosf,
                            DataType *detectorsinf);
#else

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
							DataType *TablePosition);
typedef struct {
     int threadid;
     int i;
     DataType *imageDataPtr;
     int imageindex1Size;
     int imageindex2Size;
     int imageindex3Size;
     DataType D;
     int q;
     DataType ss;
     DataType d;
     DataType theta;
     DataType ssz;
     int qz;
     DataType *sinoDataPtr;
     int sinoindex1Size;
     int sinoindex2Size;
     int sinoindex3Size;
     DataType *sourcecosf;
     DataType *sourcesinf;
     DataType *detectorcosf;
     DataType *detectorsinf;
	 DataType *TablePosition;
}emtv3d_fcom;

emtv3d_fcom fcom[MAX_THREAD_NUM];
pthread_t forwardproj_thread[MAX_THREAD_NUM];

#endif

#endif
