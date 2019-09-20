import numpy
import sys

numpy.set_printoptions(suppress = True, precision = 4., linewidth = 180, threshold = sys.maxsize)

def print_complex_array(filename, array):
    open(filename, 'w').writelines(['%f %f\n' % (i.real, i.imag) for i in array])

print('all the presets loaded!')
