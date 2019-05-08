/******************************************************************************
 * EMupdate.h: EMTV for 3D image reconstruction
 *   
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version : 
 ******************************************************************************/

#ifndef __EMupdate_H__
#define __EMupdate_H__

#include "Array3D.h"
void EMupdate_CPU (int sinoindex1Size,
                   int sinoindex2Size,
                   int sinoindex3Size,
                   DataType *sinoDataPtr, 
                   int imageindex1Size,
                   int imageindex2Size,
                   int imageindex3Size,
                   DataType *imageDataPtr, 
                   DataType *image_denoteDataPtr, 
                   DataType *sumADataPtr, 
                   int max_iter, 
                   int Afindex1Size,
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
                   DataType *sourcecosf,
                   DataType *sourcesinf,
                   DataType *detectorcosf,
                   DataType *detectorsinf,
				   DataType *TablePosition);

#ifdef GPU_ROUTINE        
void EMupdate_GPU (int sinoindex1Size,
                   int sinoindex2Size,
                   int sinoindex3Size,
                   DataType *sinoDataPtr, 
                   int imageindex1Size,
                   int imageindex2Size,
                   int imageindex3Size,
                   DataType *imageDataPtr, 
                   //DataType *image_denoteDataPtr, 
                   DataType *sumADataPtr, 
                   int max_iter, 
                   int Afindex1Size,
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
                   DataType *TablePosition,
                   DataType *sourcecosf,
                   DataType *sourcesinf,
                   DataType *detectorcosf,
                   DataType *detectorsinf);
#endif
#endif

