/******************************************************************************
 * Vector.h: EMTV for 3D image reconstruction
 *   
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version : 
 ******************************************************************************/
#ifndef _VECTOR_H_
#define _VECTOR_H_

#include "define.h"

typedef struct _Vector3D
{
  DataType x; 
  DataType y;
  DataType z;
}Vector3D;

typedef struct _Vector3DINT
{
  int x; 
  int y;
  int z;
}Vector3DInt;

void Vector3D_Set(Vector3D *vec, DataType x, DataType y, DataType z);
void Vector3D_Sub(Vector3D *vecdst, Vector3D *vec0, Vector3D *vec1);
void Vector3D_Add(Vector3D *vecdst, Vector3D *vec0, Vector3D *vec1);
DataType Vector3D_Dot(Vector3D *vec0, Vector3D *vec1);

#endif

