#ifndef __DEFINE_H__
#define __DEFINE_H__

/********************************/
/******   Debug Options    ******/
/********************************/
//#define DEBUG_FILEOUTPUT
#define DEBUG_CHECK

#define DEBUG_LEVEL1
#ifdef  DEBUG_LEVEL1
#define DBG1(expr)  expr
#else
#define DBG1(expr)
#endif

//#define DEBUG_LEVEL2
#ifdef  DEBUG_LEVEL2
#define DBG2(expr)  expr
#else
#define DBG2(expr)
#endif

//#define BackwardProjection_DEBUG
//#define EMupdate_DEBUG

/********************************/
/******Algorithms Selection******/
/********************************/
//#define Old_Backward
//#define Initialization_ENABLE

#define New_Backward
#ifdef New_Backward
#define INTERVAL 16
#endif

/********************************/
/*****CPU Routine configuration**/
/********************************/
#define MULTITHREAD       //enable multithread 
#define MAX_THREAD_NUM 16  //set max thread number
//#define SET_CPU_AFFINITY  //set CPU binding on CPU


/********************************/
/*****GPU Routine configuration**/
/********************************/
//#define Host
#define Device
//#define MultiGPU

#ifdef Host
#define Forward_NVCC_Host
#define Backward_NVCC_Host
#define EMupdate_NVCC_Host
#define TVupdate_NVCC_Host
#endif

#ifdef Device
#define Forward_NVCC_GPU
#define Backward_NVCC_GPU
#define EMupdate_NVCC_GPU
#define TVupdate_NVCC_GPU
#endif

#ifdef Forward_NVCC_GPU
//#define Forward_GPU_Serial_Version
#define Forward_GPU_Parallel_Version
#endif

#ifdef Backward_NVCC_GPU
//#define Backward_GPU_Serial_Version
#define Backward_GPU_Parallel_Version
#endif

/********************************/
/*****   Macro Definition    ****/
/********************************/
#define SINGLEPOINT
#ifdef SINGLEPOINT
#define DataType float
#define SQRT  sqrtf
#define FLOOR floorf
#else
#define DataType double
#define SQRT  sqrt
#define FLOOR floor
#endif

#define PI 3.14159265358979323846
extern float ResoRate;

#define vx 1.00
#define vy 1.00
extern float vz;

extern int N_x_global;
extern int N_y_global;
extern int N_z_global;
extern float DistSource;
extern float DistDetector;

#define LenofInt (N_x_global / 2.0)

extern int Num_Start;
extern int Total_Num_of_Source;
extern int Num_of_Source;
extern float dtheta;
extern int Nofdetector;
extern int Nofdetectorz;
#define Lenofdetector 360/4640.0
extern float Lenofdetectorz;

// If using position file, speed is automatically set. Otherwise, set value
// distance per rotation / (number of projections per rotation / undersample))
#define speed 0.0
#define initialz 0.0

// If using position file, InitialTheta is automatically set, otherwise set value
extern float InitialTheta;

// Offset of where x-rays hit detector channel
// per manufacturer: even = 335.625, odd = 335.125 | non-FFS = 335.25
extern float OffCenter;

// Settings to specify EM and TV steps 1 = perform; 0 = skip
extern int max_iter_EM_global;
extern int max_iter_TV_global;
extern int alpha_global;
extern int Max_Iter_global;

#ifndef MAX
#define MAX( x, y ) ( ((x) > (y)) ? (x) : (y) )
#endif
#ifndef MIN
#define MIN( x, y ) ( ((x) < (y)) ? (x) : (y) )
#endif
#ifndef MAX3
#define MAX3(x,y,z) MAX(MAX(x,y),z)
#endif
#ifndef MIN3
#define MIN3(x,y,z) MIN(MIN(x,y),z)
#endif
#define ABS_VALUE(x) ( (x < 0) ? -(x) : (x) )

#define LAMBDA_X(i, x_s, x_d, L) (L*((DataType)i-x_s)/(x_d-x_s))
#define LAMBDA_Y(j, y_s, y_d, L) (L*((DataType)j-y_s)/(y_d-y_s))
#define LAMBDA_Z(k, z_s, z_d, L) (L*((DataType)k-z_s)/(z_d-z_s))
#define ARRAY_SIZE(array) (sizeof((array))/sizeof((array[0])))

#endif
