import os, sys, subprocess

def run_schedule(dfg, fifo, algo, model, timeout):
    cmd = ['sb_sched', '--verbose', '-d', str(fifo), '--timeout', str(timeout)]
    if algo == 'prior':
        cmd += ['--max-iters', '1']
    elif algo == "sMRT'":
        cmd += ['--max-iters', '1000000000']
    else:
        if algo[0] == 's':
            cmd += ['--mipstart']
            algo = algo[1:]
        cmd += ['-G', '--algorithm', 'gams', '--sub-alg', algo.replace('\'', '\\\'')]
    cmd += [model, dfg]
    print(' '.join(cmd))
    try:
        res = subprocess.check_output(' '.join(cmd), shell=True)
        return res.decode('utf-8')
    except Exception as e:
        print(str(e))
        return "Schedule Failed!"
 

def extract_sched_and_mis(raw):
    sched_time = "sched_time: "
    sched_fail = "Schedule Failed!"
    sched_miss = "latency mismatch: "
    sched_latc = "latency: "
    calc_mis   = False
    mis        = 0
    sch        = 0
    lat        = 0
    if raw == sched_fail:
        return None
    for i in raw.split('\n'):
        if i.startswith(sched_fail):
            return None
        if i.startswith(sched_time):
            sch = float(i[len(sched_time):].rstrip().rstrip('seconds'))
        if i.startswith(sched_miss):
            mis = int(i[len(sched_miss):].rstrip())
        if i.startswith(sched_latc):
            lat = int(i[len(sched_latc):].rstrip())
    return (sch, mis, lat)

def schedule_and_simulate(
    log_file,
    exe, dfgs,
    model,
    algs = ["prior", "M.RT", "M.R.T", "MR.T", "MR.RT", "MRT", "sMRT", "sMR'.RT", "sMR.RT", "sMRT'"],
    deps = [15, 7, 3, 2, 1],
    time_cuts = [20 * 60],
    sim = True
):
    for sa in algs:
        last_failure = {}
        for timeout in sorted(time_cuts)[::-1]:
            for depth in sorted(deps)[::-1]:
                schedule_perf = []
                for dfg in dfgs:

                    def must_fail(dfg):
                        status = last_failure.get(dfg, None)
                        if status is None:
                            last_failure[dfg] = []
                            return False
                        for _dep, _timeout in status:
                            if depth <= _dep and timeout <= _timeout:
                                print('(%d, %d) failed before, (%d, %d) must fail too' % (_dep, _timeout, depth, timeout))
                                return True
                        return False

                    if not must_fail(dfg):
                        print('Schedule %s with FIFO depth %d, using sub-algorithm %s' % (dfg, depth, sa))
                        schedule_log = run_schedule(dfg, depth, sa, model, timeout)
                        tup = extract_sched_and_mis(schedule_log)
                        print("Scheduler analysis done!\n")
                        if tup is None:
                            print(tup)
                            last_failure[dfg].append((depth, timeout))
                        else:
                            print(tup)
                    else:
                        tup = None
                    schedule_perf.append(tup)

                if None not in schedule_perf:
                    if sim:
                        env = "FU_FIFO_LEN=%d SBCONFIG=%s " % (depth, model)
                        cmd = '%s make %s' % (env, exe)
                        try:
                            subprocess.check_output(cmd, shell=True)
                            print("Executable made!")
                            cmd = [env, 'gem5.opt', '$SS/gem5/configs/example/se.py', '--cpu-type=MinorCPU', '--l1d_size=2048kB']
                            cmd += ['--l1d_assoc=8', '--l1i_size=16kB', '--l2_size=16384kB', '--caches', '--cmd=./%s' % exe]
                            cmd = ' '.join(cmd)
                            print(cmd)
                            try:
                                sim_log = subprocess.check_output(cmd, shell=True).decode('utf-8')
                                print('Analyzing the simulation log...')
                                cyc = "Cycle not found!"
                                for line in sim_log.split('\n'):
                                    if line.startswith("Cycles: "):
                                        print(int(line[len("Cycles: "):]))
                                        cyc = line[len("Cycles: "):]
                            except:
                                cyc = "[Simulator]RE"
                            os.remove(exe)
                        except Exception as e:
                            cyc = 'CE'
                            print('Compilation error!')
                            print(str(e))
                    else:
                        cyc = '9999999'
                else:
                    cyc = "[Schedule]Failed"
                log = exe + "\t" + sa + "\t" + str(depth) + "\t" + str(timeout)
                schedule_perf = map(lambda x: (timeout, 0.0, 0) if x is None else (min(x[0], timeout), float(depth) / (depth + x[1]), x[2]), schedule_perf)
                for dfg, perf in zip(dfgs, schedule_perf):
                    sch, mis, lat = perf
                    log = log + ("\t%s\t%f\t%f\t%d" % (dfg, sch, mis, lat))
                log = log + "\t" + cyc
                print(log)
                open(log_file, 'a').write(log + "\n")

