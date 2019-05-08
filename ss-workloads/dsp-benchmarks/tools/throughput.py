import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties
import os, sys, numpy, imp, operator

font = FontProperties()
font.set_weight('bold')

util = imp.load_source('util', './util.py')

ooo = 130.94 * 0.75
c66 = 1.005315
revel = 1.24

bar_width = 0.3
start = 0.0

x_ticks = []
x_labls = []
artists = []

config = eval(''.join(open('config.json').readlines()))
dsp_data = config['dsp_data']

ax = plt.subplot(111)

for benchmark in dsp_data.iterkeys():
    print benchmark
    dsps = []
    revs = []
    ax.text(start - bar_width * len(benchmark) * 0.1, -3, benchmark, size = 10, fontproperties = font)
    for size, dsp_exec in dsp_data[benchmark]:
        direct = ('../%s/log_%d/' % (benchmark, size)) + '%s'
        opt = 'merge-compute' if benchmark == 'svd' else 'default'
        mkl_exec = util.parse_log(direct % '1-mkl.log', 'single-core', opt)['mus']
        rev_exec = util.parse_log(direct % 'sb-new.log', 'single-core', opt)['mus']
        #print mkl_exec, dsp_exec, rev_exec
        mkl_thp = 1.0
        dsp_thp = (mkl_exec / dsp_exec) * (ooo / c66) - mkl_thp
        rev_thp = (mkl_exec / rev_exec) * (ooo / revel) - dsp_thp - mkl_thp
        print mkl_thp, dsp_thp, rev_exec
        artists.append(plt.bar([start], [mkl_thp], bar_width, bottom = 0, color = '#fcbc22')[0])
        artists.append(plt.bar([start], [dsp_thp], bar_width, bottom = mkl_thp, color = '#66cdaa')[0])
        artists.append(plt.bar([start], [rev_thp], bar_width, bottom = dsp_thp + mkl_thp, color = '#fe6f5e')[0])
        print size, 1.0, (mkl_exec / dsp_exec) * (ooo / c66), (mkl_exec / rev_exec) * (ooo / revel)
        #print size, 1.0, (dsp_exec / rev_exec) * (c66 / revel)
        dsps.append(dsp_thp + 1.0)
        revs.append(rev_thp + dsp_thp + 1.0)
        x_ticks.append(start)
        #x_labls.append("%s-%d" % (benchmark, size))
        x_labls.append(str(size))
        start += bar_width * 1.04
    #dsp_geo = reduce(operator.mul, dsps) ** (1.0 / len(dsps)) - 1.0
    #rev_geo = reduce(operator.mul, revs) ** (1.0 / len(revs)) - dsp_geo - 1.0
    print 'geo-mean', 1.0, reduce(operator.mul, dsps) ** (1.0 / len(dsps)), reduce(operator.mul, revs) ** (1.0 / len(revs))
    #artists.append(plt.bar([start], [1.0], bar_width, bottom = 0, color = '#fcbc22')[0])
    #artists.append(plt.bar([start], [dsp_geo], bar_width, bottom = 1.0, color = '#66cdaa')[0])
    #artists.append(plt.bar([start], [rev_geo], bar_width, bottom = dsp_geo + 1.0, color = '#fe6f5e')[0])
    #x_ticks.append(start)
    #x_labls.append("%s-gmean" % (benchmark))
    start += bar_width * 1.1 * 2

ax.set_yscale('symlog')
ax.set_ylabel('Relative Throughput')
ax.set_ylim(ymin = 0.0)
ax.set_xticks(numpy.array(x_ticks))
ax.set_xticklabels(x_labls, rotation = 90)
ax.legend(artists[-3:], ['OoO', 'DSP', 'REVEL'], fontsize = 'small')
plt.show()

