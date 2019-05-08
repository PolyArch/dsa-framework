  /******************************************************************************
 * Array3D.h: EMTV for 3D image reconstruction
 *   
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version : 
 ******************************************************************************/

#ifndef _ARRAY3D_H_
#define _ARRAY3D_H_

#include "define.h"

#define ARRAY3DAddr_buffer(dataPtr, x, y, z, i1, i2, i3) ((dataPtr + i3 + z *(i2 + i1 * y))) 
#define ARRAY3DData_buffer(dataPtr, x, y, z, i1, i2, i3) (*(dataPtr + i3 + z *(i2 + i1 * y))) 

//#define ARRAY3DAddr(parray3d, i1, i2, i3) ((parray3d->dataPtr + i1 + parray3d->index3Size *(i2 + i3 * parray3d->index2Size))) 
//#define ARRAY3DData(parray3d, i1, i2, i3) (*(parray3d->dataPtr + i3 + parray3d->index3Size *(i2 + i1 * parray3d->index2Size)))
//#define Data(dataPtr, index1Size, index2Size, index3Size, i1, i2, i3) (*(dataPtr + i3 + index3Size *(i2 + i1 * index2Size)))
//#define Addr(dataPtr, index1Size, index2Size, index3Size, i1, i2, i3) ((dataPtr + i3 + index3Size *(i2 + i1 * index2Size)))

#define ARRAY3DAddr(parray3d, i1, i2, i3) ((parray3d->dataPtr + i3 + parray3d->index3Size *(i2 + i1 * parray3d->index2Size))) 
#define ARRAY3DData(parray3d, i1, i2, i3) (*(parray3d->dataPtr + i3 + parray3d->index3Size *(i2 + i1 * parray3d->index2Size)))
#define Data(dataPtr, index1Size, index2Size, index3Size, i1, i2, i3) (*(dataPtr + i3 + index3Size *(i2 + i1 * index2Size)))
#define Addr(dataPtr, index1Size, index2Size, index3Size, i1, i2, i3) ((dataPtr + i3 + index3Size *(i2 + i1 * index2Size)))

//#define ARRAY3DAddr(parray3d, i1, i2, i3) ((parray3d->dataPtr + i3 + (parray3d->index1Size * i2) + i1 * (parray3d->index1Size * parray3d->index2Size)))
//#define ARRAY3DData(parray3d, i1, i2, i3) (*(parray3d->dataPtr + i3 + parray3d->index3Size *(i2 + i1 * parray3d->index2Size)))
//#define Data(dataPtr, index1Size, index2Size, index3Size, i1, i2, i3) (*(dataPtr + i1 + (index1Size *i2) + i3 * (index1Size * index2Size)))
//#define Addr(dataPtr, index1Size, index2Size, index3Size, i1, i2, i3) ((dataPtr + i3 + index3Size *(i2 + i1 * index2Size)))

typedef struct _Array3D
{
  DataType *dataPtr;  // data pointer
  int index1Size;     // coordinate 1 size
  int index2Size;     // coordinate 2 size
  int index3Size;     // coordinate 3 size
}Array3D;

int Array3D_malloc(Array3D *array3d, int m, int n, int p);
int Array3D_free(Array3D *array3d);

void Array3D_initialize_value (DataType *dataPtr, DataType value, int m, int n, int p);
void Array3D_initialize_array (DataType *dataPtr, DataType *dataPtrSrc, int m, int n, int p);

DataType Array3D_getData(DataType *dataPtr,int index1Size,int index2Size,int index3Size,int i1,int i2,int i3);

DataType *Array3D_add(DataType *dataPtr1, DataType *dataPtr2, int index1Size, int index2Size, int index3Size);
DataType *Array3D_sub(DataType *dataPtr1, DataType *dataPtr2, int index1Size, int index2Size, int index3Size);
DataType *Array3D_mul(DataType *dataPtr1, DataType *dataPtr2, int index1Size, int index2Size, int index3Size);
DataType *Array3D_div(DataType *dataPtr1, DataType *dataPtr2, int index1Size, int index2Size, int index3Size);


void Array3D_add_value(Array3D *array3d, DataType value, int i1, int i2, int i3);
void Array3D_sub_value(Array3D *array3d, DataType value, int i1, int i2, int i3);
void Array3D_mul_value(Array3D *array3d, DataType value, int i1, int i2, int i3);
void Array3D_div_value(Array3D *array3d, DataType value, int i1, int i2, int i3);

int Array3D_copy(DataType *dstPtr1, DataType *srcPtr2, int index1Size, int index2Size, int index3Size);
int Array3D_set_value(DataType *dstPtr1, DataType value, int index1Size, int index2Size, int index3Size);

DataType *Array3D_getAdd(DataType *dataPtr, int index1Size,int index2Size,int index3Size,int i1,int i2,int i3);
int Array3D_all_set_1value(DataType *dstPtr1, DataType value, int index1Size, int index2Size, int index3Size);
DataType *Array3D_all_add_1value(DataType *dataPtr1, DataType value, int index1Size, int index2Size, int index3Size);
DataType *Array3D_all_sub_1value(DataType *dataPtr1, DataType value, int index1Size, int index2Size, int index3Size);
DataType *Array3D_all_mul_1value(DataType *dataPtr1, DataType value, int index1Size, int index2Size, int index3Size);
DataType *Array3D_all_div_1value(DataType *dataPtr1, DataType value, int index1Size, int index2Size, int index3Size);

#endif

