import os, sys

#def run_scheduler(dfgs):
#    for depth in [15, 7, 3, 2, 1]:
#        for dfg in dfgs:
#            for sa in ["M.RT", "M.R.T", "MR.T", "MR.RT", "MRT"]:
#                if sa not in os.listdir('.'):
#                    print('Create directory %s' % sa)
#                    os.mkdir(sa)
#                sa_ = sa.replace('\'', '\\\'')
#                print('Schedule %s with FIFO depth %d, using sub-algorithm %s' % (dfg, depth, sa))
#                cmd = 'time sb_sched --algorithm gams --sub-alg %s -d %d --verbose -G $SS_TOOLS/configs/revel.sbmodel %s' % (sa_, depth, dfg)
#                print(cmd)
#                proc = os.popen(cmd)
#                open('%s/%s.%d.log' % (sa, dfg, depth), 'w').write(proc.read())
#                proc.close()
#                print("Scheduler done!\n")
#                try:
#                    os.rename('%s.h' % dfg, '%s/%s.%d.h' % (sa, dfg, depth))
#                except:
#                    pass
#
#            for sa in ["MRT", "MR'.RT", "MR.RT", "MRT'"]:
#                if ("s" + sa) not in os.listdir('.'):
#                    print('Create directory %s' % ("s" + sa))
#                    os.mkdir("s" + sa)
#                sa_ = sa.replace('\'', '\\\'')
#                print('Schedule %s with FIFO depth %d, using sub-algorithm s%s' % (dfg, depth, sa))
#                cmd = 'time sb_sched --algorithm gams --sub-alg %s -d %d --verbose --mipstart -G $SS_TOOLS/configs/revel.sbmodel %s' % (sa_, depth, dfg)
#                print(cmd)
#                proc = os.popen(cmd)
#                open('s%s/%s.%d.log' % (sa, dfg, depth), 'w').write(proc.read())
#                proc.close()
#                print("Scheduler done!\n")
#                try:
#                    os.rename('%s.h' % dfg, '%s/%s.%d.h' % (sa, dfg, depth))
#                except:
#                    pass
#

def run_scheduler(dfgs):
    #for depth in [15, 7, 3, 2, 1]:
    for depth in [3, 2, 1]:
        for dfg in dfgs:
            for sa in ["MR'.RT"]:
                if ("s" + sa) not in os.listdir('.'):
                    print('Create directory %s' % ("s" + sa))
                    os.mkdir("s" + sa)
                sa_ = sa.replace('\'', '\\\'')
                print('Schedule %s with FIFO depth %d, using sub-algorithm s%s' % (dfg, depth, sa))
                cmd = 'time sb_sched --algorithm gams --sub-alg %s -d %d --verbose --mipstart -G $SS_TOOLS/configs/revel.sbmodel %s' % (sa_, depth, dfg)
                print(cmd)
                proc = os.popen(cmd)
                open('s%s/%s.%d.log' % (sa, dfg, depth), 'w').write(proc.read())
                proc.close()
                print("Scheduler done!\n")
                try:
                    os.rename('%s.h' % dfg, 's%s/%s.%d.h' % (sa, dfg, depth))
                except:
                    pass
