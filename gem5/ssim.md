# README File for stream-simulator infrastructure

# Readout of output when error occurs:

```
ACCEL 0 STATS ***        // current accelerator
Commands Issued: 1        // number of stream commands issued 
CGRA Instances: 0 -- Activity Ratio: -nan, DFGs / Cycle: -nan  // number of computation instances
CGRA Insts / Computation Instance: -nan              // number of instructions on cgra execute per computation instance, on average
CGRA Insts / Cycle: -nan (overall activity factor)  
Cycle Breakdown: CONFIG:-nan ISSUED:-nan ISSUED_MULTI:-nan CONST_FILL:-nan SCR_FILL:-nan DMA_FILL:-nan REC_WAIT:-nan CORE_WAIT:inf SCR_BAR_WAIT:-nan DMA_WRITE:-nan CMD_QUEUE:-nan CGRA_BACK:-nan DRAIN:inf NOT_IN_USE:inf 
Bandwidth Table: (B/c=Bytes/cycle, B/r=Bytes/request) -- Breakdown (sources/destinatinos): 
SP_READ:	(0 B/c, 0 B/r)  --   // This shows bytes/cycle and bytes/request for each resource
SP_WRITE:	(0 B/c, 0 B/r)  -- 
DMA_LOAD:	(0 B/c, 0 B/r)  -- 
DMA_STORE:	(0 B/c, 0 B/r)  -- 
REC_BUS_READ:	(0 B/c, 0 B/r)  -- 
---- ACCEL 0 STATUS ----
MEM REQs OUTSTANDING: 0  //number of outstanding requests that were not filled by memory yet
Active SEs: //streams which are cannot complete, due to data dependences
port->dma	port=2	acc_size=8 stride=8 bytes_comp=0 mem_addr=e4560 strides_left=512               ACTIVE
Waiting SEs: (0 0 0)  //command in the command queue
Ports:  //data waiting in ports
In Port 16:   Mem Size: 0  Num Ready: 0
Ind In Port 25:   Mem Size: 0  Num Ready: 0
Ind In Port 26:   Mem Size: 0  Num Ready: 0
Ind In Port 27:   Mem Size: 0  Num Ready: 0
Ind In Port 28:   Mem Size: 0  Num Ready: 0
Ind In Port 29:   Mem Size: 0  Num Ready: 0
Ind In Port 30:   Mem Size: 0  Num Ready: 0
Ind In Port 31:   Mem Size: 0  Num Ready: 0
Out Port 2:   In Flight: 0  Num Ready: 0  Mem Size: 0
PORT -> DMA Streams Not Empty
```


# The following flags are useful for debugging:

* Print information related to commands execution (prints the paramters of the command)
SB_COMMAND -- Command Creation (RISCV core sends a command to the accelerator)
SB_COMMAND_I -- Command Issue (When resources are free on accelerator/all dependences satisfied)
SB_COMMAND_O -- Command Completion

* See the cycle-by-cycle state of the accelerator, BLAME shows reason for stall, if there is one
DO_DBG(CYC_STAT)   
DO_DBG(SB_BLAME)

* Show memory requests
DO_DBG(MEM_REQ) 

* Show computation 
SB_COMP (show inputs, function unit outputs, and cgra outputs)


* Currently Defunct Commands
DO_DBG(RISCV_INSTS)

* Not documented yet... :  )
DO_DBG(VP_SCORE) 
DO_DBG(VP_SCORE2)

DO_DBG(SCR_ACC)    
DO_DBG(SCR_BARRIER)
DO_DBG(SB_MEM_RD)
DO_DBG(SB_MEM_WR)
DO_DBG(SB_WAIT)
DO_DBG(SB_CONTEXT)
DO_DBG(SB_ROI)

DO_DBG(SHOW_CONFIG)
DO_DBG(ACC_INDEX)



DO_DBG(SUPRESS_SB_STATS)

DO_DBG(VERIF_MEM)
DO_DBG(VERIF_PORT)
DO_DBG(VERIF_CGRA)
DO_DBG(VERIF_SCR)
DO_DBG(VERIF_CMD)
DO_DBG(VERIF_CGRA_MULTI)

DO_DBG(CHECK_SCR_ALIAS)

DO_DBG(UNREAL_INPUTS)
