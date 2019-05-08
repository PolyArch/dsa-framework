/******************************************************************************
 * EMTV3D.cpp: EMTV for 3D image reconstruction
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
#include <math.h>
#include <string.h>

#include <time.h>
#include <sys/types.h>
#include <sys/timeb.h>

#ifdef WIN32
#include "extras/getopt.h"
#else
#include <sys/time.h>
#include <getopt.h>
#include "../config.h"
#endif

#include "define.h"
#include "Array3D.h"
#include "Vector.h"
#include "Forward_Tracer_3D.h"
#include "Backward_Tracer_3D.h"
#include "EMupdate.h"
#include "TVupdate.h"
#include "CPU_Routine.h"
#include "EMTV3D.h"
#include "utilities.h"


char inputfilename[50] = "../data/NLST_R0248A_FULL_RAW";
char outputfilename[100] = "../output/NLST_R0248_OUT1";
char tablefilename[50] = "../data/NLST_R0248A_FULL_ANGLE_POS";

int threadnum = 1; //For CPU Routine, single thread is default
int Nofdetector = 336; //default number of detector channels (half of total)
int Nofdetectorz = 8; //default number of detector rows (half of total)
float dtheta = 0.0; //computed from input file
float Lenofdetectorz = 18/16; // distance per rotation / number of detectors in z direction
float ResoRate = 1.00;
int Max_Iter_global = 100; //default number of iterations to run
int Num_Start = 0;
int Total_Num_of_Source = 1159;
int Num_of_Source = 1159;
int N_x_global = 512; // default width of reconstructed image
int N_y_global = 512; // default height of reconstructed image
int N_z_global = 50; //(((Num_of_Source+1) / (2320.0/(8047/1159))) * 28.8) / vz
int max_iter_EM_global = 1;
int max_iter_TV_global = 1;
int alpha_global = 5;
float vz = 1.00;
float InitialTheta = 0.0;
float DistSource = 570.0;
float DistDetector = 1040.0;// mm
float OffCenter = 0.0;

#ifdef GPU_ROUTINE
int  mode = 1; //For GPU Routine GPU Device is default
#else
int  mode = 0; //For CPU Routine CPU is selected
#endif

int help()
{
    printf("./emtv3d -i (filename of sinogram)  -o (filename of output) -f (filename of angle/table position) -X (num pixels x-dir) -Y (num pixels y-dir) -Z (num pixels z-dir) -s (number of projections to subsample) -m (1 GPU or 0 CPU)  -t (number of threads) -d (Number of detector channels) -z (number of detector rows) -g (total number of iterations) \n");
    printf("routine mode: 0 CPU, 1 GPU\n");
    return 0;
}

int configure(int argc, char *argv[])
{
    char option_def[] = "?vh:i:c:o:m:t:u:d:z:l:g:f:s:X:Y:Z:T:E:a:R:N:S:p:";
    int ch;

    for (;;)
    {
        ch = getopt(argc, argv, option_def);
        if (ch == -1)
            break;

        switch (ch)
        {
        case '?':
        case 'v':
        case 'h':
            help();
            return -1;
        case 'i':
            strcpy(inputfilename, optarg);
            printf("input filename : %s \n", inputfilename);
            break;
		case 'a':
            alpha_global = atoi(optarg);
            printf("alpha_global: %d \n", alpha_global);
            break;
		case 'N':
            Num_Start = atoi(optarg);
            printf("Starting projection number: %d \n", Num_Start);
            break;
		case 'p':
            Total_Num_of_Source = atoi(optarg);
			N_z_global = ((Num_of_Source+1)/(1160./((Total_Num_of_Source)/(Num_of_Source))))* 18 / vz;
            printf("Number of projections: %d \n", Total_Num_of_Source);
            break;
		case 'T':
            max_iter_TV_global = atoi(optarg);
            printf("max_iter_TV_global: %d \n", max_iter_TV_global);
            break;
		case 'S':
            vz = atof(optarg);
			N_z_global = ((Num_of_Source+1)/(1160./((Total_Num_of_Source)/(Num_of_Source))))* 18 / vz;
            printf("slice thickness: %f \n", vz);
            break;
		case 'E':
            max_iter_EM_global = atoi(optarg);
            printf("max_iter_EM_global: %d\n", max_iter_EM_global);
            break;
        case 'o':
            strcpy(outputfilename, optarg);
            printf("output filename : %s \n", outputfilename);
            break;
		case 's':
            Num_of_Source = atoi(optarg);
			N_z_global = ((Num_of_Source+1)/(1160./((Total_Num_of_Source)/(Num_of_Source))))* 18 / vz;
            printf("total number of subsampled projections: %d\n", Num_of_Source);
            break;
        case 'm':
            mode = atoi(optarg);
            printf("runtime mode:  %d\n", mode);
            break;
        case 't':
            threadnum = atoi(optarg);
            printf("using %d thread\n", threadnum);
            break;
        case 'd':
            Nofdetector = atoi(optarg);
            printf("number of detectors is %d\n", Nofdetector);
            break;
        case 'z':
            Nofdetectorz = atoi(optarg);
            printf("number of detector rows is %d\n", Nofdetectorz);
            break;
        case 'g':
            Max_Iter_global = atoi(optarg);
            printf("Max_Iter_global %d\n", Max_Iter_global);
            break;
        case 'f':
            strcpy(tablefilename, optarg);
			Total_Num_of_Source = 0;
			FILE *countInputFile;
			int ch;
			countInputFile = fopen(tablefilename, "r");
			while (EOF != (ch=getc(countInputFile)))
				if ('\n' == ch)
					++Total_Num_of_Source;
			fclose(countInputFile);
            printf("input table filename : %s \n", tablefilename);
            break;
        case 'X':
            N_x_global = atoi(optarg) * ResoRate;
            printf("width of image will be %d\n", N_x_global);
            break;
        case 'Y':
            N_y_global = atoi(optarg) * ResoRate;
            printf("height of image will be %d\n", N_y_global);
            break;
        case 'Z':
            N_z_global = atoi(optarg);
            printf("number of slices will be %d\n", N_z_global);
            break;
		case 'l':
            Lenofdetectorz = atof(optarg);
			Lenofdetectorz = Lenofdetectorz * ResoRate;
            printf("Length of detector in z-direction will be %f\n", Lenofdetectorz);
            break;
		case 'c':
            OffCenter = atof(optarg);
            printf("Off center distance will be %f\n", OffCenter);
            break;
		case 'R':
            ResoRate = atof(optarg);
			N_x_global = N_x_global * ResoRate;
			N_y_global = N_y_global * ResoRate;
			Lenofdetectorz = Lenofdetectorz * ResoRate;
			DistSource = DistSource * ResoRate;
			DistDetector = DistDetector * ResoRate;
            printf("ResoRate: %f \n", ResoRate);
            break;
        default:
            printf("Unknown option detection\n");
            return 0;
        }
    }

    return 0;
}


int main(int argc, char **argv)
{
    Array3D cpu_image;      // used to store the reconstructed image
    Array3D cpu_sumA;       // used to store the summation of columns
    Array3D cpu_image_denote;
    Array3D cpu_sino; // used to store the sinogram data
		
    if (configure(argc, argv) == -1)
        return -1;
		
	N_z_global = ((Num_of_Source+1)/(1160./((Total_Num_of_Source-Num_Start)/(Num_of_Source))))* 18 / vz;
		
	int q = Nofdetector;
    int qz = Nofdetectorz;

	printf("Num_of_Source=%d, number_of_lines=%d, vz=%f\n", Num_of_Source, Total_Num_of_Source, vz);
	printf("Number of slices: %d \n", N_z_global);
	
#ifdef SINGLEPOINT
    fprintf(stdout, "Using single point data-type\n");
#else
    fprintf(stdout, "Using double point data-type\n");
#endif
    fprintf(stdout, "input sinogram: %s\n", inputfilename);
    fprintf(stdout, "input position data: %s\n", tablefilename);
    fprintf(stdout, "output image: %s\n", outputfilename);
    fprintf(stdout, "mode %d is used (0 CPU, 1 GPU)\n", mode);
    fprintf(stdout, "Thread num %d \n", threadnum);

    if (mode == 0)
    {
        //initial global buffers
        Array3D_malloc(&cpu_image,  N_x_global, N_y_global, N_z_global);
        Array3D_initialize_value ((cpu_image.dataPtr), 1.0, N_x_global, N_y_global, N_z_global);
        Array3D_malloc(&cpu_sumA,  N_x_global, N_y_global, N_z_global);
        Array3D_initialize_value ((cpu_sumA.dataPtr), 1.0, N_x_global, N_y_global, N_z_global);
        Array3D_malloc(&cpu_image_denote,  N_x_global, N_y_global, N_z_global);
        Array3D_initialize_value ((cpu_image_denote.dataPtr), 1.0, N_x_global, N_y_global, N_z_global);
        Array3D_malloc(&cpu_sino, 2 * q, 2 * qz, Num_of_Source + 1);
        Array3D_initialize_value((cpu_sino.dataPtr), 1.0, 2 * q, 2 * qz, Num_of_Source + 1);
    }

    fprintf(stdout, "image: %dx%dx%d\n", N_x_global, N_y_global, N_z_global);
    fprintf(stdout, "sumA:  %dx%dx%d\n", N_x_global, N_y_global, N_z_global);
    fprintf(stdout, "image_denote:  %dx%dx%d\n", N_x_global, N_y_global, N_z_global);
    fprintf(stdout, "sino:  %dx%dx%d\n", 2 * q, 2 * qz, Num_of_Source + 1);
    fprintf(stdout, "New Backward Projection is used, INTERVAL=%d\n", INTERVAL);

    CPU_Routine(&cpu_image, &cpu_sumA, &cpu_image_denote, &cpu_sino, inputfilename, tablefilename, threadnum);

    writeImgArray3D(outputfilename, &cpu_image);
    writeImgArray3DAnalyze(outputfilename, &cpu_image);

    //free the global buffers
    Array3D_free(&cpu_image);
    Array3D_free(&cpu_sumA);
    Array3D_free(&cpu_image_denote);
    Array3D_free(&cpu_sino);

    return 0;
}

