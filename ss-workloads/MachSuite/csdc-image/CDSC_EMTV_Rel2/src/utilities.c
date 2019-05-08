/******************************************************************************
 * utilities: useful tools for emtv3d
 *
 ******************************************************************************
 * Copyright (C) 2010~2011 EMTV 3D Reconstruction project
 * Authors: "Jianwen Chen" <jwchen@ee.ucla.edu>
 *          "Ming Yan" <basca.yan@gmail.com>
 *          "Yi Zou" <zouyi@cs.ucla.edu>
 *          "Shiwen Shen" <sshen@mednet.ucla.edu>
 *
 * Version : 1.1
 ******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <sys/time.h>
#include <sys/times.h>
#include <pthread.h>

#define __USE_GNU
#define _GNU_SOURCE
#include <sched.h>

#include "Array3D.h"
#include "utilities.h"

int64_t emtv3d_timer_us( void )
{
#ifdef WIN32
    struct _timeb tb;
    _ftime(&tb);
    return ((int64_t)tb.time * (1000) + (int64_t)tb.millitm) * (1000);
#else
    struct timeval tv_date;
    gettimeofday( &tv_date, NULL );
    return ( (int64_t) tv_date.tv_sec * 1000000 + (int64_t) tv_date.tv_usec );
#endif
}

int64_t emtv3d_timer_s( void )
{
#ifdef WIN32
    struct _timeb tb;
    _ftime(&tb);
    return ((int64_t)tb.time);
#else
    struct timeval tv_date;
    gettimeofday( &tv_date, NULL );
    return ( (int64_t) tv_date.tv_sec);
#endif
}

void readImgArray3D_bin(char *filename, Array3D *image)
{
    int img_size = image->index1Size * image->index2Size * image->index3Size;
    FILE *fin;

    fin = fopen(filename, "rb");
    if (!fin)
    {
        printf("Cannot open the input image file!\n");
        exit(1);
    }
    fread(image->dataPtr, sizeof(DataType), img_size, fin);
    fclose(fin);
}

void writeImgArray3D_bin(char *filename, Array3D *image)
{
    int img_size = image->index1Size * image->index2Size * image->index3Size;
    FILE *fout;

    fout = fopen(filename, "wb+");
    if (!fout)
    {
        printf("Cannot open the output image file!\n");
        exit(1);
    }
    fwrite(image->dataPtr, sizeof(DataType), img_size, fout);
    fclose(fout);
}

void readImgArray3D(char *in_file, Array3D *image, int total_num_lines, int ordered[], int ordered_size)
{
    int N_x = image->index1Size;
    int N_y = image->index2Size;
    int N_z = image->index3Size;
    FILE *inputfile;
    int i, j, k;
    int ret;

    inputfile = fopen(in_file, "r");
    if ( inputfile == NULL )
    {
        printf("Cannot open the input image file!\n");
        exit(1);
    }

    fprintf(stdout, "sinogram index1Size=%d \n", image->index1Size);
    fprintf(stdout, "sinogram index2Size=%d \n", image->index2Size);
    fprintf(stdout, "sinogram index3Size=%d \n", image->index3Size);

	int x = 0;
	int count = 0;
    for (i = 0; i < total_num_lines; i++){
        if (check(ordered, ordered_size, i)){
			for (j = 0; j < N_y; j++){ // N_y!!!
				for (k = 0; k < N_x; k++) {
					//if(j == 0 && k == 0)
						//printf("1. Saving sinogram data %d\n", i);
#ifdef SINGLEPOINT			
					ret = fscanf(inputfile, "%f ", ARRAY3DAddr(image, k, j, count));
					//if(j == 0 && k == 0)
					//	printf("Reading %d %d %d\n", (i - Num_Start), j, k);
#else
					ret = fscanf(inputfile, "%lf ", ARRAY3DAddr(image, k, j, count));
#endif
					if (ret == EOF) {
						printf("\nError reading sinogram at i=%d\n", i);
						exit(0);
					}
				}
			}
			count++;
		} else {
			for (j = 0; j < N_y; j++){ // N_y!!!
				for (k = 0; k < N_x; k++) {
					//if(j == 0 && k == 0)
						//printf("   Skipping sinogram data %d\n", i);
					float temp;
					ret = fscanf(inputfile, "%f ", &temp);
					//if(j == 0 && k == 0)
					//	printf("    Skipping %d %d %d\n", (i - Num_Start), j, k);
					if (ret == EOF) {
						printf("\nError reading sinogram at i=%d\n", i);
						exit(0);
					}
				}
			}
		}
	}
    fclose(inputfile);
}

int check(int ordered[], int ordered_size, int val){
	int x;
	for(x = 0; x < ordered_size; x++){
		if(ordered[x] == val){
			return(1);
		}
	}
	return(0);

}
void writeImgArray3DDataPtr(char *out_file, DataType *DataPtr, int index1size, int index2size, int index3size)
{
    printf("Saving %s!\n", out_file);
    int N_x = index1size;
    int N_y = index2size;
    int N_z = index3size;
    FILE *outputfile;
    int i, j, k;

    outputfile = fopen(out_file, "w");
    if ( outputfile == NULL)
    {
        printf("Cannot open the output file!\n");
        exit(1);
    }
    for (k = 0; k < N_z; k++)
        for (i = 0; i < N_x; i++)
            for (j = 0; j < N_y; j++)
            {
#ifdef SINGLEPOINT
                fprintf(outputfile, "%f\n", Data(DataPtr, index1size, index2size, index3size, i, j, k));
#else
                fprintf(outputfile, "%lf\n", Data(DataPtr, index1size, index2size, index3size, i, j, k));
#endif
            };
    fclose(outputfile);
}

void writeImgArray3D(char *out_file, Array3D *image)
{
    int N_x = image->index1Size;
    int N_y = image->index2Size;
    int N_z = image->index3Size;
    FILE *outputfile;
    int i, j, k;

    outputfile = fopen(out_file, "w");
    if ( outputfile == NULL)
    {
        printf("Cannot open the output file!\n");
        exit(1);
    }
    for (k = 0; k < N_z; k++)
        for (i = 0; i < N_x; i++)
            for (j = 0; j < N_y; j++)
            {
#ifdef SINGLEPOINT
                fprintf(outputfile, "%f\n", ARRAY3DData(image, i, j, k));
#else
                fprintf(outputfile, "%lf\n", ARRAY3DData(image, i, j, k));
#endif
            };
    fclose(outputfile);
}

void writeImgArray3DAnalyze(char *out_file, Array3D *image)
{
    int img_size = image->index1Size * image->index2Size * image->index3Size;
    int N_x = image->index1Size;
    int N_y = image->index2Size;
    int N_z = image->index3Size;
    int i, j, k;
    float tempData;
    char outImageName[80], outHeadName[80];
    strcpy(outImageName, out_file);
    strcat(outImageName, ".img");
    strcpy(outHeadName, out_file);
    strcat(outHeadName, ".hdr");

    FILE *fout = fopen(outImageName, "wb");
    if (!fout)
    {
        printf("Cannot open the output image file!\n");
        exit(1);
    }
    //fwrite(image->dataPtr, sizeof(DataType), img_size, fout);

    for (k = 0; k < N_z; k++)
        for (i = 0; i < N_x; i++)
            for (j = 0; j < N_y; j++)
            {
                tempData = ARRAY3DData(image, i, j, k);
                fwrite(&tempData, sizeof(DataType), 1, fout);
            };
    fclose(fout);


    int width = N_x; //width
    int height = N_y; //height
    int z = N_z; //depth
    int numOfVolum = 1; //number of volumes

    struct dsr hdr;
    FILE *fp;

    float max = ARRAY3DData(image, 0, 0, 0); //maximum voxel value
    float min = ARRAY3DData(image, 0, 0, 0); //minimum voxel value
    for (k = 0; k < N_z; k++)
        for (i = 0; i < N_x; i++)
            for (j = 0; j < N_y; j++)
            {
                max = MAX(ARRAY3DData(image, i, j, k), max);
                min = MIN(ARRAY3DData(image, i, j, k), min);
            };
    max = floor(max);
    min = floor(min);


    memset(&hdr, 0, sizeof(struct dsr));
    for (i = 0; i < 8; i++)
        hdr.dime.pixdim[i] = 0.0;

    hdr.dime.vox_offset = 0.0;
    hdr.dime.roi_scale  = 1.0;
    hdr.dime.funused1   = 0.0;
    hdr.dime.funused2   = 0.0;
    hdr.dime.cal_max    = 0.0;
    hdr.dime.cal_min    = 0.0;


    hdr.dime.datatype = -1;

#ifdef SINGLEPOINT
    hdr.dime.datatype = 16;
    hdr.dime.bitpix = 32;
#else
    hdr.dime.datatype = 64;
    hdr.dime.bitpix = 64;
#endif



    if ((fp = fopen(outHeadName, "w")) == 0)
    {
        printf("unable to create: %s\n", outHeadName);
        exit(0);
    }

    hdr.dime.dim[0] = 4;  /* all Analyze images are taken as 4 dimensional */
    hdr.hk.regular = 'r';
    hdr.hk.hkey_un0 = 16;
    hdr.hk.sizeof_hdr = sizeof(struct dsr);

    hdr.dime.dim[1] = width;  /* slice width  in pixels */
    hdr.dime.dim[2] = height;  /* slice height in pixels */
    hdr.dime.dim[3] = z;  /* volume depth in slices */
    hdr.dime.dim[4] = numOfVolum;  /* number of volumes per file */

    hdr.dime.glmax  = (int)max;  /* maximum voxel value  */
    hdr.dime.glmin  = (int)min;  /* minimum voxel value */

    /*  Set the voxel dimension fields:
           A value of 0.0 for these fields implies that the value is unknown.
             Change these values to what is appropriate for your data
             or pass additional command line arguments     */

    hdr.dime.pixdim[1] = 0.0; /* voxel x dimension */
    hdr.dime.pixdim[2] = 0.0; /* voxel y dimension */
    hdr.dime.pixdim[3] = 0.0; /* pixel z dimension, slice thickness */

    /*   Assume zero offset in .img file, byte at which pixel
           data starts in the image file */

    hdr.dime.vox_offset = 0.0;

    /*   Planar Orientation;    */
    /*   Movie flag OFF: 0 = transverse, 1 = coronal, 2 = sagittal
         Movie flag ON:  3 = transverse, 4 = coronal, 5 = sagittal  */

    hdr.hist.orient     = 0;

    /*   up to 3 characters for the voxels units label; i.e.
            mm., um., cm.               */

    strcpy(hdr.dime.vox_units, " ");

    /*   up to 7 characters for the calibration units label; i.e. HU */

    strcpy(hdr.dime.cal_units, " ");

    /*     Calibration maximum and minimum values;
           values of 0.0 for both fields imply that no
           calibration max and min values are used    */

    hdr.dime.cal_max = 0.0;
    hdr.dime.cal_min = 0.0;

    fwrite(&hdr, sizeof(struct dsr), 1, fp);
    fclose(fp);

}


void writeImgArray3DWithIndex(char *out_file, Array3D *image)
{
    int N_x = image->index1Size;
    int N_y = image->index2Size;
    int N_z = image->index3Size;
    FILE *outputfile;
    int i, j, k;

    outputfile = fopen(out_file, "w");
    if ( outputfile == NULL)
    {
        printf("Cannot open the output file!\n");
        exit(1);
    }
    for (i = 0; i < N_x; i++)
        for (j = 0; j < N_y; j++)
            for (k = 0; k < N_z; k++)
            {
#ifdef SINGLEPOINT
                fprintf(outputfile, "%3d, %3d, %3d, %f\n", i, j, k, ARRAY3DData(image, i, j, k));
#else
                fprintf(outputfile, "%3d, %3d, %3d, %lf\n", i, j, k, ARRAY3DData(image, i, j, k));
#endif
            };
    fclose(outputfile);
}

void CompareVector(DataType *data0, DataType *data1, int size)
{
    int i;
    DataType diff = 0.0;
    DataType diffall = 0.0;

    for (i = 0; i < size; i++)
    {
        diff = data0[i] - data1[i];

        if (diff > 0.1)
#ifdef SINGLEPOINT
            fprintf(stdout, "%d, data0[%d] = %f data1[%d]= %f \n", i, i, data0[i], i, data1[i]);
#else
            fprintf(stdout, "%d, data0[%d] = %lf data1[%d]= %lf \n", i, i, data0[i], i, data1[i]);
#endif

        diffall += diff;
    }

#ifdef SINGLEPOINT
    fprintf(stdout, "diffall = %f\n", diffall);
#else
    fprintf(stdout, "diffall = %lf\n", diffall);
#endif
}

/*************************************************************
 *  input: sector  the sector thread number (1~8)
 *************************************************************/
#ifdef SET_CPU_AFFINITY
int setcpu(int cpuidx)
{
    cpu_set_t mask;
    pthread_attr_t attr;

    //printf("bind to CPU %d\n", cpuidx);

    pthread_attr_init(&attr);
    if (pthread_attr_getaffinity_np(&attr, sizeof(cpu_set_t) , &mask) < 0)
    {
        return -1;
    }

    CPU_ZERO(&mask);
    CPU_SET(cpuidx, &mask);

    if (pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t) , &mask) < 0)
    {
        return -1;
    }
    pthread_attr_destroy(&attr);

    return 0;
}
#endif



