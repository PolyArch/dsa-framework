#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <pthread.h>
#define __USE_GNU
#include <sched.h>

#ifndef WIN32
#include "../config.h"
#endif

#include "define.h"
#include "Array3D.h"
#include "EMTV3D.h"
#include "Forward_Tracer_3D.h"
#include "Backward_Tracer_3D.h"
#include "EMupdate.h"
#include "TVupdate.h"
#include "utilities.h"

int loadTableAnglePosition(DataType *TablePosition, DataType *sourcecosf, DataType *sourcesinf, DataType *detectorcosf, DataType *detectorsinf, char *sino_file, char *pos_file, int ordered[], int ordered_size)
{
    int i, j;
	int total_num_lines = 0;
    FILE *inputfile;
    int ret;
    float fValue;
    float first;
    float TempTable[Num_of_Source + 1];
    float TempAngle[Num_of_Source + 1];
    DataType thetaS = 0.;
    DataType dtheta2 = dtheta * PI / 180.;
    DataType ss2 = Lenofdetector * PI / 180.;
	int NumArray[Num_of_Source+1];
	int y, a;
	
	inputfile = fopen(pos_file, "r");
    if ( inputfile == NULL )
    {
        printf("\nCannot open the input file!\n");
        //       exit(1);
        for (i = 0; i < Num_of_Source+1; i++)
        {
            TablePosition[i] = ResoRate * speed * i;
        }

        for (j = -Nofdetector; j < Nofdetector; j++)
        {
            detectorcosf[j + Nofdetector] = cosf((j - OffCenter) * ss2 + PI);
            detectorsinf[j + Nofdetector] = sinf((j - OffCenter) * ss2 + PI);
        }

        for (i = 0; i < Num_of_Source+1; i++)
        {
            thetaS = InitialTheta + i * dtheta2;
            sourcecosf[i] = cosf(thetaS);
            sourcesinf[i] = sinf(thetaS);
        }
    } else {
        printf("Loading table position information from %s\n", pos_file);
	
		int x = 0, y = 0;
		//int increment = (Total_Num_of_Source+1)/(Num_of_Source+1);
		//printf("Increment=%d\n",increment);
		//int count = Num_Start;
		srand(time(NULL));
		while(x < (Num_of_Source+1))
		{
		//	NumArray[x] =  count;
		//	count = count + increment;
			y = 0;
			NumArray[x] = (rand() % ((Total_Num_of_Source+1)-Num_Start)) + Num_Start;
			while(y < (Num_of_Source+1))
			{
				if(NumArray[y] == NumArray[x] && x != y) //If another number is equal to this one.
				{
					--x; //Set x back one. 
					y = 0; //Reset y;
					break; //Try again.
				}
				else
				{
					 ++y; //Otherwise, check the next number.
				}
			}
			++x; //When we reach here, if --x happened, we are about to re-do a number that was not unique. Otherwise, do the next number.
		}
	
	for (i=0; i<(Num_of_Source+1); i++){
        ordered[i]=NumArray[i];
	}

    for (i=0; i<(Num_of_Source+1); i++) {
		for (a=0; a<(Num_of_Source); a++) {
			if(ordered[a]>ordered[a+1]){
				int temp=ordered[a+1];
				ordered[a+1]=ordered[a];
				ordered[a]=temp;
			}
		}
	}

	FILE *countInputFile;
	int ch;
	countInputFile = fopen(pos_file, "r");
	while (EOF != (ch=getc(countInputFile)))
		if ('\n' == ch)
			++total_num_lines;
	fclose(countInputFile);
	
	printf("num_lines==%d\n",total_num_lines);
	
	int j = 0;
    for ( i = 0; i < total_num_lines; i++) {
        float posValue;
        float angleValue;
		float initialAngle;
		int inputtedValue = 1;
			
		if (check1(ordered, ordered_size, i)){
			ret = fscanf(inputfile, "%f,%f\n", &posValue, &angleValue);
			TempTable[j] = posValue;
			TempAngle[j] = angleValue;
			//if(j == array_size/2)
			//printf("    read iter=%d, proj_num=%d, angle=%f, pos=%f\n", j, i, angleValue, posValue);
						//if(j == 0)
						//	InitialTheta = (angleValue * PI/180.0);
			inputtedValue = 0;
			j++;
		}

        if(inputtedValue) {
			float temp1;
			float temp2;
			ret = fscanf(inputfile, "%f,%f\n", &temp1, &temp2);
			//printf("   not being read angle=%f, pos=%f\n", temp1, temp2);
        }
        if (ret == EOF) {
            printf("\nError in Read TablePosition i=%d\n", i);
            exit(0);
        }
    }
	
	fclose(inputfile);

	int rotation_count = 0;
	float tempAngle = 0;
	//float angleIncrement = ((((total_num_lines/1160.0)*360.0)/total_num_lines)*increment)*(PI/180.);
    //float posIncrement = ((28.8/1160.0)*increment);
		
	for ( i = 0; i < Num_of_Source+1; i++) {
        if (i == 0) {
            TablePosition[i] = 0;
			tempAngle = (TempAngle[0] * PI / 180.);
				sourcecosf[i] = cosf(tempAngle);
				sourcesinf[i] = sinf(tempAngle);
				//printf("    i=%d position=%f, angle=%f\n", i, TablePosition[i], (tempAngle * 180/PI));
        } else {
				if(TempAngle[i-1] >= 359. && TempAngle[i] < 1.){
					rotation_count = rotation_count + 1;
				}
				
                float val = (TempTable[0] - TempTable[i]) / (1000.0);
				
				float tempAngle = ((TempAngle[i] * PI / 180.) + (2*PI*rotation_count));
				//val = val + posIncrement;
				//tempAngle = tempAngle + angleIncrement;
				sourcecosf[i] = cosf(tempAngle);
				sourcesinf[i] = sinf(tempAngle);
                TablePosition[i] = ABS_VALUE(val);
				
				//printf("i=%d position=%f, angle=%f\n", i, TablePosition[i], (tempAngle * 180/PI));
            }
        }
        for (j = -Nofdetector; j < Nofdetector; j++)
        {
            detectorcosf[j + Nofdetector] = cosf((j - OffCenter) * ss2 + PI);
            detectorsinf[j + Nofdetector] = sinf((j - OffCenter) * ss2 + PI);
        }
		
		// for ( i = 0; i < Num_of_Source+1; i++)
        // {
            // float tempAngle = InitialTheta + TempAngle[i] * PI / 180.;
            // sourcecosf[i] = cosf(tempAngle);
            // sourcesinf[i] = sinf(tempAngle);
        // }
    }
	
	return (total_num_lines);
}

int check1(int ordered[], int ordered_size, int val){
	int x;
	for(x = 0; x < ordered_size; x++){
		if(ordered[x] == val){
			return(1);
		}
	}
	return(0);

}

int multithread_init(DataType *imageDataPtr, DataType *imagedenoteDataPtr, int imageindex1Size, int imageindex2Size, int imageindex3Size,
                     DataType *sinoDataPtr, int sinoindex1Size, int sinoindex2Size, int sinoindex3Size,
                     DataType *sourcecosf, DataType *sourcesinf, DataType *detectorcosf, DataType *detectorsinf, DataType *TablePosition, int threadnum)
{
    int i;
    for (i = 0; i < threadnum; i++)
    {
        //forward projection
        fcom[i].imageDataPtr = imageDataPtr;
        fcom[i].imageindex1Size = imageindex1Size;
        fcom[i].imageindex2Size = imageindex2Size;
        fcom[i].imageindex3Size = imageindex3Size;
        fcom[i].sinoDataPtr = sinoDataPtr;
        fcom[i].sinoindex1Size = sinoindex1Size;
        fcom[i].sinoindex2Size = sinoindex2Size;
        fcom[i].sinoindex3Size = sinoindex3Size;
        fcom[i].sourcecosf = sourcecosf;
        fcom[i].sourcesinf = sourcesinf;
        fcom[i].detectorcosf = detectorcosf;
        fcom[i].detectorsinf = detectorsinf;
		fcom[i].TablePosition = TablePosition;

        //backward projection
        bcom[i].imageDataPtr = imageDataPtr;
        bcom[i].image_denoteDataPtr = imagedenoteDataPtr;
        bcom[i].imageindex1Size = imageindex1Size;
        bcom[i].imageindex2Size = imageindex2Size;
        bcom[i].imageindex3Size = imageindex3Size;
        bcom[i].sinoDataPtr = sinoDataPtr;
        bcom[i].sinoindex1Size = sinoindex1Size;
        bcom[i].sinoindex2Size = sinoindex2Size;
        bcom[i].sinoindex3Size = sinoindex3Size;
        bcom[i].sourcecosf = sourcecosf;
        bcom[i].sourcesinf = sourcesinf;
        bcom[i].detectorcosf = detectorcosf;
        bcom[i].detectorsinf = detectorsinf;
		bcom[i].TablePosition = TablePosition;
    }
    return 0;
}


int CPU_Routine(Array3D *image, Array3D *sumA, Array3D *image_denote, Array3D *sino, char *inputfilename, char *tablefilename, int threadnum)
//image: stores the reconstruction image
//sumA: stroes the summation of each colum: refer to EMTV algorithm details
//image_denote: mask image
//sino: the sinagram image
{
    int max_iter_EM = max_iter_EM_global;
    int max_iter_TV = max_iter_TV_global;
    //parameter used in EMTV to balance EM and TV regularation, referred to EMTV paper equation (3)
    DataType alpha = alpha_global;
    int iter;
    int64_t i_start_whole, i_end_whole;
    int64_t i_start_emtvupdate, i_end_emtvupdate;
    int64_t i_start, i_end;
    int N_x = image->index1Size;
    int N_y = image->index2Size;
    int N_z = image->index3Size;   // the size of the image, N_x * N_y * N_z


    DataType *sourcecosf;   //[128];
    DataType *sourcesinf;   //[128];
    DataType *detectorcosf; //[512*36];
    DataType *detectorsinf; //[512*36];
	DataType *TablePosition;

    //malloc for TVupdate temp buffer
    Array3D imagetemp;
    Array3D Af;
    Array3D gAf;
    Array3D AtAf;

    Array3D_malloc(&imagetemp, image->index1Size, image->index2Size, image->index3Size);
    Array3D_initialize_value((imagetemp.dataPtr), 1.0, image->index1Size,image->index2Size,image->index3Size);
    Array3D_malloc(&Af,  sino->index1Size, sino->index2Size, sino->index3Size);
    Array3D_initialize_value((Af.dataPtr), 1.0, sino->index1Size, sino->index2Size, sino->index3Size);
    Array3D_malloc(&gAf, sino->index1Size, sino->index2Size, sino->index3Size);
    Array3D_initialize_value((gAf.dataPtr), 1.0, sino->index1Size, sino->index2Size, sino->index3Size);
    Array3D_malloc(&AtAf, image->index1Size, image->index2Size, image->index3Size);
    Array3D_initialize_value((AtAf.dataPtr), 1.0, image->index1Size,image->index2Size,image->index3Size);

    fprintf(stdout, "Host Calculation Arrays and Variables\n");
    fprintf(stdout, "Allocating %d for arrays\n", (Num_of_Source + 1));
    sourcecosf = (DataType *) malloc((Num_of_Source + 1) * sizeof(DataType)); //[128];
    sourcesinf = (DataType *) malloc((Num_of_Source + 1) * sizeof(DataType)); //[128];
    detectorcosf = (DataType *) malloc((Num_of_Source + 1) * sizeof(DataType)); //[512*36];
    detectorsinf = (DataType *) malloc((Num_of_Source + 1) * sizeof(DataType)); //[512*36];
	TablePosition = (DataType *) malloc((Num_of_Source + 1) * sizeof(DataType)); //[512*36];

    //get the source position and store them in table
	int ordered[Num_of_Source+1];
	int ordered_size =  ARRAY_SIZE(ordered);
    int total_num_lines = loadTableAnglePosition(TablePosition, sourcecosf, sourcesinf, detectorcosf, detectorsinf, inputfilename, tablefilename, ordered, ordered_size);
	
#ifdef MULTITHREAD
    printf("Initializing threads...\n");
	multithread_init(image->dataPtr, image_denote->dataPtr, image->index1Size, image->index2Size, image->index3Size,
                     sino->dataPtr, sino->index1Size, sino->index2Size, sino->index3Size,
                     sourcecosf, sourcesinf, detectorcosf, detectorsinf, TablePosition, threadnum);
#endif



    printf("imageindex1Size=%d \n", sumA->index1Size);
    printf("imageindex2Size=%d \n", sumA->index2Size);
    printf("imageindex3Size=%d \n", sumA->index3Size);
    printf("sinoindex1Size=%d \n", sino->index1Size);
    printf("sinoindex2Size=%d \n", sino->index2Size);
    printf("sinoindex3Size=%d \n", sino->index3Size);

    fprintf( stdout, "CPU Backward_Projection begins \n");
    i_start = emtv3d_timer_us();
    //this backward projection is performed to intialize the sum matrix : sumA
    Backward_Projection_CPU(sumA->dataPtr,
                            image_denote->dataPtr,
                            sumA->index1Size, // size of x-plane
                            sumA->index2Size, // size of y-plane
                            sumA->index3Size, // slices in z-plane
                            sino->dataPtr,
                            sino->index1Size, // # of channels
                            sino->index2Size, // # of detector rows
                            sino->index3Size, // # of projections
                            sourcecosf,
                            sourcesinf,
                            detectorcosf,
                            detectorsinf,
							TablePosition);
    i_end = emtv3d_timer_us();
    fprintf( stdout, "CPU Backward_Projection uses %ld us\n", (i_end - i_start));

    //printf("reading filename : %s \n", inputfilename);
    //readImgArray3D(inputfilename, sino);
	printf("Reading sinogram data from file...\n");
	readImgArray3D(inputfilename, sino, total_num_lines, ordered, ordered_size);

    fprintf(stdout, "CPU Initialization\n");
    Array3D_initialize_value(image->dataPtr, 1.0, N_x, N_y, N_z);
    Array3D_copy(image->dataPtr, image_denote->dataPtr, N_x, N_y, N_z);

    fprintf(stdout, "CPU EMTV update process begins\n");
    i_start_emtvupdate = emtv3d_timer_s();
    for (iter = 0; iter < Max_Iter_global; iter++)
    {
        i_start = emtv3d_timer_us();
        EMupdate_CPU(sino->index1Size,
                     sino->index2Size,
                     sino->index3Size,
                     sino->dataPtr,
                     image->index1Size,
                     image->index2Size,
                     image->index3Size,
                     image->dataPtr,
                     image_denote->dataPtr,
                     sumA->dataPtr,
                     max_iter_EM,
                     Af.index1Size,
                     Af.index2Size,
                     Af.index3Size,
                     Af.dataPtr,
                     gAf.index1Size,
                     gAf.index2Size,
                     gAf.index3Size,
                     gAf.dataPtr,
                     AtAf.index1Size,
                     AtAf.index2Size,
                     AtAf.index3Size,
                     AtAf.dataPtr,
                     sourcecosf,
                     sourcesinf,
                     detectorcosf,
                     detectorsinf,
					 TablePosition);
        i_end = emtv3d_timer_us();
        fprintf( stdout, "CPU EMupdate %d uses %ld us\n", iter, (i_end - i_start));

        i_start = emtv3d_timer_us();
        TVupdate_CPU(image->index1Size,
                     image->index2Size,
                     image->index3Size,
                     image->dataPtr,
                     sumA->index1Size,
                     sumA->index2Size,
                     sumA->index3Size,
                     sumA->dataPtr,
                     alpha,
                     max_iter_TV,
                     imagetemp.dataPtr);
        i_end = emtv3d_timer_us();

        if((iter+1) % 5 == 0){
            char filename [100];
            sprintf(filename, "../output/EMupdate_Iter%d.dat", (iter+1));
            writeImgArray3DAnalyze(filename, image);
        }
        fprintf( stdout, "CPU TVupdate %d uses %ld us\n", iter, (i_end - i_start));

    }

    i_end_emtvupdate = emtv3d_timer_s();
    i_end_whole = emtv3d_timer_s();

    fprintf( stdout, "CPU total using %ld seconds\n", (i_end_whole - i_start_whole));
    fprintf( stdout, "CPU routine exit successfully\n");

    return 0;
}
