#!/usr/bin/env python3
import os, re, math, subprocess

#Global frequency set at 1000000000000 ticks per second
#*** ROI STATISTICS ***
#Simulator Time: 0.02552 sec
#Cycles: 4011
#Control Core Insts Issued: 1792
#Control Core Discarded Insts Issued: 355 #Control Core Discarded Inst Stalls: 0.1981
#Control Core Config Stalls: 0.04188
#Control Core Wait Stalls:  result correct!
# 0.1257 (ALL)  
#
#ACCEL 0 STATS ***
#Commands Issued: 98
#CGRA Instances: 352 -- Activity Ratio: 0.08776, DFGs / Cycle: 0.08776
#CGRA Insts / Computation Instance: 6.25
#CGRA Insts / Cycle: 0.5485 (overall activity factor)
#Mapped DFG utilization: 0
#Data availability ratio: -nan
#Percentage bank conflicts: -nan
#Allowed input port consumption rate: 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
#percentage time we could not serve input ports: 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
#Cycle Breakdown: CONFIG:0.0429 ISSUED:0.0878 ISSUED_MULTI:0.0000 TEMPORAL_ONLY:0.0000 CONST_FILL:0.0000 SCR_FILL:0.0000 DMA_FILL:0.0007 REC_WAIT:0.0000 CORE_WAIT:0.3141 SCR_BAR_WAIT:0.0000 DMA_WRITE:0.0000 CMD_QUEUE:0.0055 CGRA_BACK:0.0000 DRAIN:0.0631 NOT_IN_USE:0.4859 
#Bandwidth Table: (B/c=Bytes/cycle, B/r=Bytes/request) -- Breakdown (sources/destinatinos): 
#SP_READ:	0.0329(1.1B/c, 33.3B/r)  -- PORT:0.0329(1.1B/c, 33.3B/r) 
#SP_WRITE:	0.0686(0.548B/c, 8B/r)  -- PORT:0.0686(0.548B/c, 8B/r) 
#DMA_LOAD:	0.00922(0.307B/c, 33.3B/r)  -- PORT:0.00922(0.307B/c, 33.3B/r) 
#DMA_STORE:	0.0192(0.154B/c, 8B/r)  -- PORT:0.0192(0.154B/c, 8B/r) 
#REC_BUS_READ:	0.012(1.4B/c, 117B/r)  -- CONST:0.012(1.4B/c, 117B/r) 
#
#Cores used: 1bitmask: 1
#Total Memory Activity: 0.00922
#
# -- Parallelization Estimates --
#Multi-Pipeline Cycles (cores:cycles): 1:4011 2:2005 3:1337 4:1002 5:802 6:668 7:573 8:501 9:445 10:401 11:364 12:334 13:308 14:286 15:267 16:250 17:235 18:222 
#Multi-Pipeline Activity: (cores:cycles)1:0.0878 2:0.0878 3:0.0878 4:0.0878 5:0.0878 6:0.0878 7:0.0878 8:0.0878 9:0.0879 10:0.0878 11:0.0879 12:0.0878 13:0.0879 14:0.0879 15:0.0879 16:0.088 17:0.0881 18:0.0881 
#
#Exiting @ tick 807870500 because exiting with last active thread context


def run(sizes, template, softbrains, log = 'log'):

    subprocess.check_output(['make', 'ultraclean'])
    for i in sizes:
        subprocess.check_output(['make', 'clean'])

        env = template % i

        print('Run Case %s...' % env)
        for sb in softbrains:
            print('Run %s...' % sb)
            try:
                raw = subprocess.check_output('%s make sb-%s.log' % (env, sb), shell=True).decode('utf-8')

                cycle = None
                brkd  = None
                ipc   = None
                accel = []
                for line in raw.split('\n'):
                    if line.startswith('Cycles: '):
                        cycle = line.lstrip('Cycles: ').rstrip()
                    elif line.startswith('ACCEL'):
                        if brkd is not None:
                            accel.append((ipc, brkd))
                        brkd = {}
                    elif line.startswith('Cycle Breakdown: '):
                        for attr in line[len('Cycle Breakdown: '):].rstrip().split(' '):
                            cont, perc = attr.split(':')
                            brkd[cont] = float(perc)
                    elif line.startswith('CGRA Insts / Cycle: '):
                        ipc = float(line.lstrip('CGRA Insts / Cycle: ').rstrip(' (overall activity factor)'))
                
                if brkd is not None:
                    accel.append((ipc, brkd))
                with open(log, 'a') as f:
                    res = ('%s %s %s ' % (env, sb, cycle)) + str(accel)
                    f.write(res + '\n')
            except:
                print('%s %s error occur!\n' % (env, sb))
                quit()


if __name__ == '__main__':
    print('Nothing to be done yet')
