asic_cycles = [(4693, 20651), (364, 5344), (48, 1280), (1537, 12289), (1464, 17919), (8000, 95953)]
reve_cycles = [(5201, 22531), (934, 6691), (220, 1690), (2130, 12580), (3112, 20351), (26365, 187460)]
workloads   = ['centro-fir', 'cholesky', 'fft', 'gemm', 'qr', 'svd']
control_extra = 37.4 + 39.1
control_both  = 2.6 + 11.3 + 18.3
computation = [5.88, 11.46, 23.63, 23.63, 3.21, 0,99]
power_asic  = [190.4, 131.2, 9.52, 9.52, 230.3, 312.1]

for i in xrange(6):
    asic = asic_cycles[i]
    revel = reve_cycles[i]
    power = power_asic[i]
    compute = computation[i]
    smal_a, larg_a = asic
    smal_r, larg_r = revel
    print workloads[i]
    print (compute + power + control_both + control_extra) / (power + control_both)

