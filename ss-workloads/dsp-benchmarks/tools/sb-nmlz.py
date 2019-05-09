import matplotlib.pyplot as plt
import numpy

colors = [
    '#FF69B4', '#BDB76B', '#FF7F50', '#DDA0DD',
    '#4B0082', '#4B0082', '#006400', '#008080',
    '#00CED1', '#00008B', '#A9A9A9', '#DC143C',
    '#DAA520', '#4169E1'
]

labels = [
    'GeMM',
    'FFT',
    'Cholesky',
    'QR',
    'SVD'
]

val = [
    [(7.38, 2.07), (5.57 / 25, 0.836), (3.45, 4.59), (10.81, 7.96), (39.0, 82.89)],
    [(26.7, 7.44), (177.27 / 25, 8.155), (9.98, 16.65), (38.98, 51.73), (252, 496.63)]
]

fig, ax = plt.subplots(1, 2)
n = len(labels)

arch = ['MKL', 'SOFTBRAIN']

for i in xrange(2):
    for j in xrange(2):
        ax[i].bar(numpy.arange(n) + j * 0.4, [(a[j] / a[0]) if j == 0 else a[0] / a[j] for a in val[i]], width = 0.37, label = arch[j])
    ax[i].set_xticks(numpy.arange(n))
    ax[i].set_xticklabels(labels, rotation = 75)
    ax[i].set_ylim(0, 3.7)

ax[0].set_title('Small')
ax[1].set_title('Large')
ax[1].legend()

plt.show()

