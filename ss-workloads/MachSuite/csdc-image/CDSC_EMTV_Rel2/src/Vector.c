/******************************************************************************
 * Vector.c: EMTV for 3D image reconstruction
 *   
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version : 
 ******************************************************************************/

#include "Vector.h"

void Vector3D_Set(Vector3D *vec, DataType x, DataType y, DataType z)
{
   vec->x= x; 
   vec->y= y; 
   vec->z= z; 
}


void Vector3D_Sub(Vector3D *vecdst, Vector3D *vec0, Vector3D *vec1)
{
  vecdst->x = vec0->x - vec1->x;
  vecdst->y = vec0->y - vec1->y;
  vecdst->z = vec0->z - vec1->z;
}

void Vector3D_Add(Vector3D *vecdst, Vector3D *vec0, Vector3D *vec1)
{
  vecdst->x = vec0->x + vec1->x;
  vecdst->y = vec0->y + vec1->y;
  vecdst->z = vec0->z + vec1->z;
}

DataType Vector3D_Dot(Vector3D *vec0, Vector3D *vec1)
{
   return (vec0->x * vec1->x + vec0->y * vec1->y + vec0->z * vec1->z); 
}

