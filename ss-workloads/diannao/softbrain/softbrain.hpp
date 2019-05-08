/*
* MM Headerfile for softbrain parameters relevant to DNN
*/

// Scratchpad info
#define SCRATCHSIZE 2048 //  number of dnn elts that fit in scratch pad
#define SCRATCHSTART 0 // logical address for start of scratch

// Virtual port interfaces
// input ports for pipe0
#define INPUTNEURON0 0  // Wide & deep      
#define INPUTWEIGHT0 1  // Wide & deep
#define INPUTACC0    2  // Deep
#define INPUTPRED0   3  // Deep
// input ports for pipe1
#define INPUTNEURON1 4  // Wide & deep           
#define INPUTWEIGHT1 5  // Wide & deep
#define INPUTACC1    6  // Deep
#define INPUTPRED1   7  // Deep

// output ports
#define OUTPUT0 8  // Output for pipe 0: Deep (pipe is 16:1 reduce)
#define OUTPUT1 9  // Output for pipe 1: Deep

