/******************************************************************************
 * Array3D.c: EMTV for 3D image reconstruction
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
#include <string.h>
#include "define.h"
#include "Array3D.h"


/*
#ifdef DEBUG_CHECK
static void boundsCheck (int i, int begin, int end, int coordinate)
  {
    if ((i < begin) || (i > end))
    {
      printf ("Array index %d out of bounds \n", coordinate);
      printf ("Offending index value %d : Acceptable Range [%d, %d] \n", i, begin, end);
    }
  }
#else
static void boundsCheck (int i, int i, int i, int coordinate)
  {
  }
#endif


#ifdef DEBUG_CHECK
static void sizeCheck (int Msize1, int Msize2, int Nsize1, int Nsize2, int Psize1, int Psize2)
  {
    if (Msize1 != Msize2)
    {
      printf ("1st Dimension Sizes Are Incompatable  %d != %d \n", Msize1, Msize2);
    }
    if (Nsize1 != Nsize2)
    {
      printf ("2nd Dimension Sizes Are Incompatable  %d != %d \n", Nsize1, Nsize2);
    }
    if (Psize1 != Psize2)
    {
      printf ("3nd Dimension Sizes Are Incompatable  %d != %d \n", Psize1, Psize2);
    }
  }
#else
static void sizeCheck (int Msize1, int Msize2, int Nsize1, int Nsize2, int Psize1, int Psize2)
  {
  }
#endif
*/


int Array3D_malloc(Array3D *array3d, int m, int n, int p)
{
   DataType *dataPtr=NULL; // data pointer
   int index1Size;    // coordinate 1 size
   int index2Size;    // coordinate 2 size
   int index3Size;    // coordinate 3 size

   array3d->index1Size = index1Size = m;
   array3d->index2Size = index2Size = n;
   array3d->index3Size = index3Size = p;

   if ((index1Size != 0) && (index2Size != 0) && (index3Size != 0))
    {
      dataPtr = (DataType*) malloc(m * n * p * sizeof(DataType));
    }
   else
   {
    printf("Array3D_malloc: use zero size and malloc failed\n");
    return -1;
   }

   //init the data to 0.0 
    memset(dataPtr, 0, m * n * p * sizeof(DataType) );
   //for (i = 0; i < index1Size * index2Size * index3Size; i++)
   //{
   //   dataPtr[i] = 0.0;
   //}


   array3d->dataPtr = dataPtr;

   return 0;
};

int Array3D_free(Array3D *array3d)
{

   if(array3d->dataPtr!=NULL)
     free(array3d->dataPtr);

   return 0;
};


void Array3D_initialize_value (DataType *dataPtr, DataType value, int m, int n, int p)
{
   int index1Size;    // coordinate 1 size
   int index2Size;    // coordinate 2 size
   int index3Size;    // coordinate 3 size
   int i;

   index1Size = m;
   index2Size = n;
   index3Size = p;

   for (i = 0; i < index1Size * index2Size * index3Size; i++)
   {
      dataPtr[i] = value;
   }
};

void Array3D_initialize_array (DataType *dataPtr, DataType *dataPtrSrc, int m, int n, int p)
{
   int index1Size;    // coordinate 1 size
   int index2Size;    // coordinate 2 size
   int index3Size;    // coordinate 3 size
   int i;

   index1Size = m;
   index2Size = n;
   index3Size = p;

   for (i = 0; i < index1Size * index2Size * index3Size; i++)
   {
      dataPtr[i] = dataPtrSrc[i];
   }
};


DataType Array3D_getData(DataType *dataPtr, int index1Size,int index2Size,int index3Size,int i1,int i2,int i3)
{
   return *(dataPtr + i3 + index3Size *(i2 + i1 * index2Size));
}

DataType *Array3D_getAdd(DataType *dataPtr, int index1Size,int index2Size,int index3Size,int i1,int i2,int i3)
{
   return dataPtr + i3 + index3Size *(i2 + i1 * index2Size);
}


DataType *Array3D_add(DataType *dataPtr1, DataType *dataPtr2, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dataPtr1[i] += dataPtr2[i];
    }
    return dataPtr1;
}

DataType *Array3D_sub(DataType *dataPtr1, DataType *dataPtr2, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dataPtr1[i] -= dataPtr2[i];
    }
    return dataPtr1;
}

DataType *Array3D_mul(DataType *dataPtr1, DataType *dataPtr2, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dataPtr1[i] = dataPtr1[i] * dataPtr2[i];
    }
    return dataPtr1;
}

DataType *Array3D_div(DataType *dataPtr1, DataType *dataPtr2, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dataPtr1[i] = dataPtr1[i] / dataPtr2[i];
    }
    return dataPtr1;
}

int Array3D_copy(DataType *dstPtr1, DataType *srcPtr2, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dstPtr1[i] = srcPtr2[i];
    }
    return 0;
}



void Array3D_add_value(Array3D *array3d, DataType value, int i1, int i2, int i3)
{
     int index2Size = array3d->index2Size;
     int index3Size = array3d->index3Size;

     *(array3d->dataPtr + i3 + index3Size *(i2 + i1 * index2Size)) += value; ;
}

void Array3D_sub_value(Array3D *array3d, DataType value, int i1, int i2, int i3)
{
     int index2Size = array3d->index2Size;
     int index3Size = array3d->index3Size;

     *(array3d->dataPtr + i3 + index3Size *(i2 + i1 * index2Size)) -= value; ;
}

void Array3D_mul_value(Array3D *array3d, DataType value, int i1, int i2, int i3)
{
     int index2Size = array3d->index2Size;
     int index3Size = array3d->index3Size;

     *(array3d->dataPtr + i3 + index3Size *(i2 + i1 * index2Size)) *= value; ;
}

void Array3D_div_value(Array3D *array3d, DataType value, int i1, int i2, int i3)
{
     int index2Size = array3d->index2Size;
     int index3Size = array3d->index3Size;

     *(array3d->dataPtr + i3 + index3Size *(i2 + i1 * index2Size)) /= value; ;
}

DataType *Array3D_all_add_1value(DataType *dataPtr1, DataType value, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dataPtr1[i] += value;
    }
    return dataPtr1;
}

DataType *Array3D_all_sub_1value(DataType *dataPtr1, DataType value, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dataPtr1[i] -= value;
    }
    return dataPtr1;
}

DataType *Array3D_all_mul_1value(DataType *dataPtr1, DataType value, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dataPtr1[i] = dataPtr1[i] * value;
    }
    return dataPtr1;
}

DataType *Array3D_all_div_1value(DataType *dataPtr1, DataType value, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dataPtr1[i] = dataPtr1[i] / value;
    }
    return dataPtr1;
}

int Array3D_all_set_1value(DataType *dstPtr1, DataType value, int index1Size, int index2Size, int index3Size)
{
    int i;
    for (i = 0; i < index1Size * index2Size * index3Size; i++)
    {
      dstPtr1[i] = value;
    }
    return 0;
}





