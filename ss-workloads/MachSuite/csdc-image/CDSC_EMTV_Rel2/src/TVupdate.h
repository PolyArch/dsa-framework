/******************************************************************************
 * TVupdate.h: EMTV for 3D image reconstruction
 *   
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version : 
 ******************************************************************************/

#ifndef __TVupdate_H__
#define __TVupdate_H__

#include "define.h"
#include "Array3D.h"

void TVupdate_CPU(int imageindex1Size,
                  int imageindex2Size,
                  int imageindex3Size,
                  DataType *imageDataPtr,
                  int sumindex1Size,
                  int sumindex2Size,
                  int sumindex3Size,
                  DataType *sumADataPtr,
                  DataType alpha,
                  int max_iter,
                  DataType *imagetempDataPtr);

void TVupdate_GPU(int imageindex1Size,
                  int imageindex2Size,
                  int imageindex3Size,
                  DataType *imageDataPtr,
                  int sumindex1Size,
                  int sumindex2Size,
                  int sumindex3Size,
                  DataType *sumADataPtr,
                  DataType alpha,
                  int max_iter,
                  DataType *imagetempDataPtr);
#endif
