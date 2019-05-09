import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties
import os, sys, numpy, imp

font = FontProperties()
font.set_weight('bold')

util = imp.load_source('util', './util.py')

colors = ['#cd0000', '#ef6a65', '#9e6c19', '#000066', '#ffd700', '#ff7f00', '#20b2ad', '#999999'] #print colors

types = ['multi-issued', 'issued', 'drain', 'scr-b/w', 'scr-barier', 'stream-dpd', 'ctrl-ovhd', 'config']

#print types

fig, ax = plt.subplots(1, 2)

fig.tight_layout(pad = 0.1)

bar_width = 0.3

x_labls = []
artists = []
below_x = []

to_show = [
[
["../svd/log_12/sb-origin.log", "../svd/log_12/sb-new.log"],
"=====",
["../fft/log_64/sb-origin.log", "../fft/log_64/sb-new.log"],
"=====",
["../qr/log_12/sb-origin.log", "../qr/log_12/sb-new.log", "../qr2/log_12/sb-new.log"],
"=====",
["../cholesky/log_12/sb-origin.log", "../cholesky/log_12/sb-new.log", "../cholesky/log_12/sb-latency.log"],
"=====",
["../centro-fir/log_37/sb-origin.log", "../centro-fir/log_37/sb-new.log", "../centro-fir/log_164_37/sb-origin.log", "../centro-fir/log_37/sb-latency.log"],
"=====",
["../gemm/log_12/sb-origin.log", "../gemm/log_12/sb-new.log", "../gemm/log_2/sb-origin.log", "../gemm/log_12/sb-latency.log"],
],
[
["../svd/log_32/sb-origin.log", "../svd/log_32/sb-new.log"],
"=====",
["../fft/log_1024/sb-origin.log", "../fft/log_1024/sb-new.log"],
"=====",
["../qr/log_32/sb-origin.log", "../qr/log_32/sb-new.log", "../qr2/log_32/sb-new.log"],
"=====",
["../cholesky/log_32/sb-origin.log", "../cholesky/log_32/sb-new.log", "../cholesky/log_32/sb-latency.log"],
"=====",
["../centro-fir/log_199/sb-origin.log", "../centro-fir/log_199/sb-new.log", "../centro-fir/log_326_199/sb-origin.log", "../centro-fir/log_199/sb-latency.log"],
"=====",
["../gemm/log_96/sb-origin.log", "../gemm/log_96/sb-new.log", "../gemm/log_8/sb-origin.log", "../gemm/log_96/sb-latency.log"]
]
]

names = [
'softb', 'revel',
'softb', 'revel',
'softb', 'revel', 'multi-revel',
'softb', 'revel', 'multi-revel',
'softb', 'revel', 'multi-softb', 'multi-revel',
'softb', 'revel', 'multi-softb', 'multi-revel',
]

breakdown = {
        "config": ["CONFIG"],
		"ctrl-ovhd": ["CORE_WAIT", "NOT_IN_USE", "CMD_QUEUE"],
		"stream-dpd":["REC_WAIT"],
		"scr-barier":["SCR_BAR_WAIT"],
		"scr-b/w":   ["SCR_FILL", "DMA_FILL", "DMA_WRITE"],
		"drain":     ["DRAIN"],
		"issued":      ["ISSUED"],
		"multi-issued":["ISSUED_MULTI"]
}

for no in range(2):
    start = 0.0
    x_ticks = []
    below_x = []
    for paths in to_show[no]:
        if paths == '=====':
            start += bar_width
            continue
        
        opt = 'merge-compute' if paths[0].startswith('../svd/') else 'default'
        
        n = len(paths)
        parsed = map(lambda x: util.parse_log(x, 'single-core', opt), paths)
        height = [0.] * n
        ratio  = [1.] + [i['mus'] / parsed[0]['mus'] for i in parsed[1:]]
        
        stripped = paths[0].lstrip('./')
        stripped = stripped[:stripped.find('/')]
        below_x.append((start - 0.5 * bar_width, stripped))
        
        for i in xrange(n):
            for portion, color in zip(types, colors):
                delta =  sum(parsed[i]['breakdowns'][key] for key in breakdown[portion])
                artists.append(ax[no].bar([start], [delta * ratio[i]], bar_width, color = color, bottom = height[i])[0])
                height[i] += ratio[i] * delta
                #print portion, color
            x_ticks.append(start)
            start += bar_width * 1.1
        #x_ticks.append(start)
        #x_ticks.append(start + bar_width * 1.02)
        #x_labls.append('sb-%s-%d' % (benchmark, size))
        #x_labls.append('rv-%s-%d' % (benchmark, size))
        #start += bar_width * 2.2
    ax[no].set_xticks(numpy.array(x_ticks))
    ax[no].set_xticklabels(names, rotation = 90)
    for axis, name in below_x:
        ax[no].text(axis - 0.1, -0.3, name, size=10, rotation = 80, fontproperties = font)

#ax[0].set_title('small')
#ax[1].set_title('large')
ax[0].legend(
    artists[-len(types):], types, ncol = len(types), fontsize='small', handlelength = 0.7,
    loc  = 3,
    bbox_to_anchor=(0, 1, 1, 1),
)
ax[0].set_ylabel('Relative Execution Time')

#below_x = [(-0.15, 'svd'), (0.72, 'fft'), (1.59, 'qr'), (2.5, 'cholesky'), (4, 'centro-fir'), (5.5200000000000005, 'gemm'), (7.260000000000001, 'svd'), (8.13, 'fft'), (9.000000000000002, 'qr'), (10.200000000000003, 'cholesky'), (11.400000000000004, 'centro-fir'), (12.930000000000005, 'gemm')]


plt.show()
