/******************************************************************************
 * TVupdate.cpp: EMTV for 3D image reconstruction
 *
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *
 * Version :
 ******************************************************************************/

#include <math.h>
#include <string.h>

#include "define.h"
#include "TVupdate.h"

void TVupdate_CPU(int imageindex1Size,//image inforamtion, inculding size and data
                  int imageindex2Size,
                  int imageindex3Size,
                  DataType *imageDataPtr,
                  int sumindex1Size,//sum of colomns, which is useful in the algorithm, inucluding index and value,
                  int sumindex2Size,
                  int sumindex3Size,
                  DataType *sumADataPtr,
                  DataType alpha,// the parameter is used to adjust the weight for EM and TV, refer to equation (3)
                  int max_iter,
                  DataType *imagetempDataPtr)

//TV regularizartion please refer to the paper:Expectation Maximization and Total Variation Based Model for Computed Tomography Reconstruction from
//Undersampled Data; the final equation
//since this algorithm is 3-D vergion, the equation is revised
{
    int N_x = imageindex1Size - 1;
    int N_y = imageindex2Size - 1;
    int N_z = imageindex3Size - 1;

    DataType c1, c2, c3, c4;
    DataType eps = (DataType)(1e-4);
    int i, j, k, l;
    int datasize = imageindex1Size * imageindex2Size * imageindex3Size * sizeof(DataType);

    for (i = 0; i < max_iter; i ++)
    {
        memcpy(imagetempDataPtr, imageDataPtr, datasize);
        for (j = 1; j < N_x; j ++)
            for (k = 1; k < N_y; k ++)
                for (l = 1; l < N_z; l ++)
                {
                    DataType data000;
                    DataType data100, data010, data001;
                    DataType datan100, datan110, datan101;
                    DataType data1n10, data0n10, data0n11;
                    DataType data10n1, data00n1, data01n1;
                    DataType sum;
                    DataType sub100, sub010, sub001;
                    DataType sub000, subn110, subn101;
                    DataType sub1n10, sub0002, sub0n11;
                    DataType sub10n1, sub01n1, sub0003;
                    DataType temp;
                    //v_i_j in TV equation, the sum of a_i_j i ranges from 1 to M
                    sum     = Data(sumADataPtr, sumindex1Size, sumindex2Size, sumindex3Size, j, k, l);
                    //current pixel value
                    data000 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, l);
                    //if (data000 > 1e-4 && sum > 1e-4)
                    //{
                        //intiallize the neighborhood value and their differences
                        data100 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, (j + 1), k, l);
                        data010 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size,  j, (k + 1), l);
                        data001 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size,  j, k, (l + 1));
                        datan100 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, (j - 1), k, l);
                        datan110 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, (j - 1), (k + 1), l);
                        datan101 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, (j - 1), k, (l + 1));
                        data1n10 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, (j + 1), (k - 1), l);
                        data0n10 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, (k - 1), l);
                        data0n11 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, (k - 1), (l + 1));
                        data10n1 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, (j + 1), k, (l - 1));
                        data00n1 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, (l - 1));
                        data01n1 = Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, (k + 1), (l - 1));

                        sub100 = data100 - data000;
                        sub010 = data010 - data000;
                        sub001 = data001 - data000;

                        sub000  = data000  - datan100;
                        subn110 = datan110 - datan100;
                        subn101 = datan101 - datan100;

                        sub1n10 = data1n10 - data0n10;
                        sub0002 = data000  - data0n10;
                        sub0n11 = data0n11 - data0n10;

                        sub10n1 = data10n1 - data00n1;
                        sub01n1 = data01n1 - data00n1;
                        sub0003 = data000  - data00n1;

                        //calculate the weight for each differential item
                        c1 = data000 / SQRT(eps + sub100 * sub100  + sub010 * sub010  + sub001 * sub001) / sum;
                        c2 = data000 / SQRT(eps + sub000 * sub000  + subn110 * subn110 + subn101 * subn101) / sum;
                        c3 = data000 / SQRT(eps + sub1n10 * sub1n10 + sub0002 * sub0002 + sub0n11 * sub0n11) / sum;
                        c4 = data000 / SQRT(eps + sub10n1 * sub10n1 + sub01n1 * sub01n1 + sub0003 * sub0003) / sum;
                        //calcualte the numerator for the output
                        temp = alpha * Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, l)
                               + c1 * (  Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, (j + 1), k, l)
                                         + Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size,  j, (k + 1), l)
                                         + Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size,  j, k, (l + 1))
                                      )
                               + c2 * Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, (j - 1), k, l)
                               + c3 * Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, (k - 1), l)
                               + c4 * Data(imagetempDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, (l - 1));
                        //the final ouput
                        Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, l)
                            = (DataType)(temp / (alpha + 3.0 * c1 + c2 + c3 + c4));
                        if (Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, l) > 1000)
                            Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, l) = 1000;
                    //}
                };

        //for the boundary pixels, their values are copied from the inside neighborhood
        for (j = 1; j < N_x; j ++)
            for (k = 1; k < N_y; k ++)
            {
                Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, 0)
                    = Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, 1);
                Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, N_z)
                    = Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, k, (N_z - 1));
            }
        for (j = 1; j < N_x; j ++)
            for (l = 0; l <= N_z; l++)
            {
                Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, 0, l)
                    = Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, 1, l);
                Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, N_y, l)
                    = Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, j, N_y - 1, l);
            }
        for (k = 0; k <= N_y; k++)
            for (l = 0; l <= N_z; l++)
            {
                Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, 0, k, l)
                    = Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, 1, k, l);
                Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, N_x, k, l)
                    = Data(imageDataPtr, imageindex1Size, imageindex2Size, imageindex3Size, N_x - 1, k, l);
            }
    }
}

