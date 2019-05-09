import os, imp, numpy, sys, operator
import matplotlib.pyplot as plt
import matplotlib

config_json = sys.argv[1]
mode        = sys.argv[2]

util = imp.load_source('util', './util.py')

numpy.set_printoptions(precision = 2, suppress = True, threshold = 1000, linewidth = 150)
#matplotlib.rcParams['hendlelength'] = 2

fig, ax = plt.subplots(1, 2)

config = eval(''.join(open(config_json, 'r').readlines()))

dsp_data = config['dsp_data']
options  = config[mode]
benchmarks = list(dsp_data.iterkeys())
n = len(benchmarks)

print benchmarks

colors = ['#fcbc22', '#66cdaa', '#abcdef', '#fe6f5e']

default_arch  = {
    'mkl'        : '1-mkl',
    #'asic'       : 'gen',
    'softbrain'  : 'sb-origin',
    'revel'      : 'sb-new' if mode == 'single-core' else 'sb-latency',
}

nmlz = None
bar_width = 0.3
for case in range(2):
    start = 0.0
    cnt = 0
    for impl in ['dsp', 'mkl', 'softbrain', 'revel']:
        muses = []
        for benchmark in benchmarks:
            if impl == 'mkl':
                times = util.get_option_or_default(config, ['times', benchmark], 1.0)
            else:
                times = 1.0
            if impl == 'dsp':
                muses.append(dsp_data[benchmark][case * -1][1])
                continue
            path = util.get_path_by_option(options, case, benchmark, impl, default_arch[impl], str(dsp_data[benchmark][-1 * case][0]))
            print path
            #print parsed
            opt = util.get_option_or_default(options, [benchmark, impl, 'opt'], 'default')
            parsed = util.parse_log(path, mode, opt)
            mus = parsed['mus']
            mus /= times
            muses.append(mus)
        ind = start + bar_width * (n + 1) * numpy.arange(n + 1) * 0.65
        if impl == 'dsp':
            nmlz = numpy.array(muses)
            bar  = numpy.ones(n + 1)
            print muses
        else:
            _nmlz = nmlz
            print impl, _nmlz / numpy.array(muses)
            speedup = _nmlz / numpy.array(muses)
            geomean = reduce(operator.mul, speedup) ** (1.0 / n)
            print impl, geomean
            bar = _nmlz / numpy.array(muses)
            bar = numpy.concatenate((bar, [geomean]))
        #else:
        #    print arch, nmlz * 1000. / numpy.array(muses)
        #    speedup = nmlz / numpy.array(muses)
        #    geomean = reduce(operator.mul, speedup)
        #    bar  = nmlz * 1000. / numpy.array(muses)
        #    bar = numpy.concatenate(bar, [geomean])
        print ind, bar
        arch = util.get_option_or_default(config, ['arch', impl], impl)
        ax[case].bar(ind, bar, bar_width, label = arch, color = colors[cnt])
        cnt += 1
        start += bar_width * 1.02
    ax[case].set_xticks(bar_width * 1.1 + bar_width * (n + 1) * numpy.arange(n + 1) * 0.65)
    ax[case].set_xticklabels(benchmarks + ['geo-mean'], rotation = 60)
    if not case:
        ax[case].set_ylabel('Exec. Time Speedup')


ax[1 if mode == 'single-core' else 0].legend(prop = {'size': 6})

ax[0].set_title('small')
ax[1].set_title('large')

plt.show()

