/******************************************************************************
 * EMTV3D.h: EMTV parameters for 3D image reconstruction
 *   
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version : 
 ******************************************************************************/

#ifndef __EMTV3D_H__
#define __EMTV3D_H__

#ifdef _MSC_VER
  typedef __int64 int64_t;
#else
  #include <stdint.h>		// Use the C99 official header
#endif

#include "define.h"

int configure(int argc, char*argv[]);

#endif
