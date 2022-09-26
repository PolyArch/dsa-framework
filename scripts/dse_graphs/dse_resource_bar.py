import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import pandas as pd
import os
import glob
import sys
from os.path import dirname
from multiprocessing import Process

from math import atan2, degrees
import numpy as np
import json
font = {'family' : 'normal',
        'weight' : 'normal',
        'size'   : 12}

def get_resources(token, dtype=int):
    "total lut: 0.16748, logic lut: 0.164201, srl: 0, ram lut: 0.240064, ff: 0.00965791, ramb36: 0, ramb18: 0, uram: 0, dsp: 0.0585052"
    resources = token.str.split(',', expand=True)
    resources[0] = resources[0].str.extract('(\d+)', expand=False).astype(dtype)
    resources[1] = resources[1].str.extract('(\d+)', expand=False).astype(dtype)
    resources[2] = resources[2].str.extract('(\d+)', expand=False).astype(dtype)
    resources[3] = resources[3].str.extract('(\d+)', expand=False).astype(dtype)
    resources[4] = resources[4].str.extract('(\d+)', expand=False).astype(dtype)
    resources[5] = resources[5].str.rsplit(':').str[-1] 
    resources[5] = resources[5].str.extract('(\d+)', expand=False).astype(dtype)
    resources[6] = resources[6].str.rsplit(':').str[-1] 
    resources[6] = resources[6].str.extract('(\d+)', expand=False).astype(dtype)
    resources[7] = resources[7].str.extract('(\d+)', expand=False).astype(dtype)
    resources[8] = resources[8].str.extract('(\d+)', expand=False).astype(dtype)
    resources = resources.set_axis(['total lut', 'logic lut', 'srl', 'ram lut', 'ff', 'ramb36', 'ramb18', 'uram', 'dsp'], axis=1, copy=False)
    return resources

def get_resources_double(token, dtype=np.double):
    '''total lut: 65766.6, logic lut: 53923, ram lut: 12235.6, ff: 48493.8'''
    resources = token.str.split(',', expand=True)
    resources[0] = resources[0].str.extract('(\d+.\d+)', expand=False).astype(dtype)
    resources[1] = resources[1].str.extract('(\d+.\d+)', expand=False).astype(dtype)
    resources[2] = resources[2].str.extract('(\d+.\d+)', expand=False).astype(dtype)
    resources[3] = resources[3].str.extract('(\d+.\d+)', expand=False).astype(dtype)
    resources[4] = resources[4].str.extract('(\d+.\d+)', expand=False).astype(dtype)
    resources[5] = resources[5].str.extract('(\d+.\d+)', expand=False).astype(dtype)
    resources[6] = resources[6].str.extract('(\d+.\d+)', expand=False).astype(dtype)
    resources[7] = resources[7].str.extract('(\d+.\d+)', expand=False).astype(dtype)
    resources[8] = resources[8].str.extract('(\d+.\d+)', expand=False).astype(dtype)
    resources = resources.set_axis(['total lut', 'logic lut', 'srl', 'ram lut', 'ff', 'ramb36', 'ramb18', 'uram', 'dsp'], axis=1, copy=False)
    return resources

def get_dfg_performance(token):
    axis = []
    performances = token.str.split(',', expand=True)
    for i in range(performances.shape[1]):
        performances[i] = performances[i].str.split(':').str[1]
        performances[i] = performances[i].str.extract('(\d+)', expand=False).astype(int)
    return performances

def get_utilization(token):
    pass

def get_workload(token):
    axis = []
    workload = token.str.split(',', expand=True)
    for i in range(workload.shape[1]):
        axis.append(i)
        workload[i] = workload[i].str.extract('(\d+)', expand=False).astype(int)
    workload = workload.set_axis(axis)
    return workload

def load_to_global(index, data, global_data):
    for i in range(len(data)):
        global_data[i][index] = np.asarray(data[i])
    return global_data

def load_line(filepath):
    if not os.path.exists(filepath):
        print("File doesnt exist: " + filepath)
        exit(1)
    '''Best OVport Resources,Best Scratchpad Resources,Best DMA Resources,Best Recurrance Resources,Best Generate Resources,Best Register Resources,Best Core Resources,Best System Bus Resources
    '''
    line_data =  pd.read_csv(filepath)
    line_data_resources =  get_resources(line_data['Best Resources'])
    best_single_normalized_resources = get_resources_double(line_data['Best Single-Core Normalized Resources'], np.double)
    best_normalized_resources = get_resources_double(line_data['Best Normalized Resources'], np.double)
    fu_resources = get_resources(line_data['Best Functional Unit Resources'])
    switch_resources = get_resources(line_data['Best Switch Resources'])
    ivport_resources = get_resources(line_data['Best IVPort Resources'])
    ovport_resources = get_resources(line_data['Best OVPort Resources'])
    scratchpad_resources = get_resources(line_data['Best Scratchpad Resources'])
    dma_resources = get_resources(line_data['Best DMA Resources'])
    recurrance_resources = get_resources(line_data['Best Recurrance Resources'])
    generate_resources = get_resources(line_data['Best Generate Resources'])
    register_resources = get_resources(line_data['Best Register Resources'])
    core_resources = get_resources(line_data['Best Core Resources'])
    system_bus_resources = get_resources(line_data['Best System Bus Resources'])

    output_data = [0] * 133
    '''
    def load_resources(line_data_resources, startpos):
        output_data[startpos] = line_data_resources['total lut'].to_numpy()
        output_data[startpos + 1] = line_data_resources['logic lut'].to_numpy()
        output_data[startpos + 2] = line_data_resources['ram lut'].to_numpy()
        output_data[startpos + 3] = line_data_resources['srl'].to_numpy()
        output_data[startpos + 4] = line_data_resources['ff'].to_numpy()
        output_data[startpos + 5] = line_data_resources['ramb36'].to_numpy()
        output_data[startpos + 6] = line_data_resources['ramb18'].to_numpy()
        output_data[startpos + 7] = line_data_resources['uram'].to_numpy()
        output_data[startpos + 8] = line_data_resources['dsp'].to_numpy()
    '''
    output_data[0] = line_data['Time'].to_numpy()
    output_data[1] = line_data['Iteration'].to_numpy()
    output_data[2] = line_data['Best Objective Area'].to_numpy()
    output_data[3] = line_data['Best Objective Performance'].to_numpy()
    output_data[4] = line_data['Best Util Overall'].to_numpy()
    output_data[5] = line_data['Best Link Util'].to_numpy()
    output_data[6] = line_data['Best Node Util'].to_numpy()
    '''
    load_resources(line_data_resources, 7)
    load_resources(best_single_normalized_resources, 16)
    load_resources(best_normalized_resources, 25)
    load_resources(fu_resources, 34)
    load_resources(switch_resources, 43)
    load_resources(ivport_resources, 52)
    load_resources(ovport_resources, 61)
    load_resources(scratchpad_resources, 70)
    load_resources(dma_resources, 79)
    load_resources(recurrance_resources, 88)
    load_resources(generate_resources, 97)
    load_resources(register_resources, 106)
    load_resources(core_resources, 115)
    load_resources(system_bus_resources, 124)
    '''
    output_data[7] = line_data_resources['total lut'].to_numpy()
    output_data[8] = line_data_resources['logic lut'].to_numpy()
    output_data[9] = line_data_resources['ram lut'].to_numpy()
    output_data[10] = line_data_resources['srl'].to_numpy()
    output_data[11] = line_data_resources['ff'].to_numpy()
    output_data[12] = line_data_resources['ramb36'].to_numpy()
    output_data[13] = line_data_resources['ramb18'].to_numpy()
    output_data[14] = line_data_resources['uram'].to_numpy()
    output_data[15] = line_data_resources['dsp'].to_numpy()

    output_data[16] = best_single_normalized_resources['total lut'].to_numpy()
    output_data[17] = best_single_normalized_resources['logic lut'].to_numpy()
    output_data[18] = best_single_normalized_resources['ram lut'].to_numpy()
    output_data[19] = best_single_normalized_resources['srl'].to_numpy()
    output_data[20] = best_single_normalized_resources['ff'].to_numpy()
    output_data[21] = best_single_normalized_resources['ramb36'].to_numpy()
    output_data[22] = best_single_normalized_resources['ramb18'].to_numpy()
    output_data[23] = best_single_normalized_resources['uram'].to_numpy()
    output_data[24] = best_single_normalized_resources['dsp'].to_numpy()

    output_data[25] = best_normalized_resources['total lut'].to_numpy()
    output_data[26] = best_normalized_resources['logic lut'].to_numpy()
    output_data[27] = best_normalized_resources['ram lut'].to_numpy()
    output_data[28] = best_normalized_resources['srl'].to_numpy()
    output_data[29] = best_normalized_resources['ff'].to_numpy()
    output_data[30] = best_normalized_resources['ramb36'].to_numpy()
    output_data[31] = best_normalized_resources['ramb18'].to_numpy()
    output_data[32] = best_normalized_resources['uram'].to_numpy()
    output_data[33] = best_normalized_resources['dsp'].to_numpy()

    output_data[34] = fu_resources['total lut'].to_numpy()
    output_data[35] = fu_resources['logic lut'].to_numpy()
    output_data[36] = fu_resources['ram lut'].to_numpy()
    output_data[37] = fu_resources['srl'].to_numpy()
    output_data[38] = fu_resources['ff'].to_numpy()
    output_data[39] = fu_resources['ramb36'].to_numpy()
    output_data[40] = fu_resources['ramb18'].to_numpy()
    output_data[41] = fu_resources['uram'].to_numpy()
    output_data[42] = fu_resources['dsp'].to_numpy()

    output_data[43] = switch_resources['total lut'].to_numpy()
    output_data[44] = switch_resources['logic lut'].to_numpy()
    output_data[45] = switch_resources['ram lut'].to_numpy()
    output_data[46] = switch_resources['srl'].to_numpy()
    output_data[47] = switch_resources['ff'].to_numpy()
    output_data[48] = switch_resources['ramb36'].to_numpy()
    output_data[49] = switch_resources['ramb18'].to_numpy()
    output_data[50] = switch_resources['uram'].to_numpy()
    output_data[51] = switch_resources['dsp'].to_numpy()

    output_data[52] = ivport_resources['total lut'].to_numpy()
    output_data[53] = ivport_resources['logic lut'].to_numpy()
    output_data[54] = ivport_resources['ram lut'].to_numpy()
    output_data[55] = ivport_resources['srl'].to_numpy()
    output_data[56] = ivport_resources['ff'].to_numpy()
    output_data[57] = ivport_resources['ramb36'].to_numpy()
    output_data[58] = ivport_resources['ramb18'].to_numpy()
    output_data[59] = ivport_resources['uram'].to_numpy()
    output_data[60] = ivport_resources['dsp'].to_numpy()

    output_data[61] = ovport_resources['total lut'].to_numpy()
    output_data[62] = ovport_resources['logic lut'].to_numpy()
    output_data[63] = ovport_resources['ram lut'].to_numpy()
    output_data[64] = ovport_resources['srl'].to_numpy()
    output_data[65] = ovport_resources['ff'].to_numpy()
    output_data[66] = ovport_resources['ramb36'].to_numpy()
    output_data[67] = ovport_resources['ramb18'].to_numpy()
    output_data[68] = ovport_resources['uram'].to_numpy()
    output_data[69] = ovport_resources['dsp'].to_numpy()

    output_data[70] = scratchpad_resources['total lut'].to_numpy()
    output_data[71] = scratchpad_resources['logic lut'].to_numpy()
    output_data[72] = scratchpad_resources['ram lut'].to_numpy()
    output_data[73] = scratchpad_resources['srl'].to_numpy()
    output_data[74] = scratchpad_resources['ff'].to_numpy()
    output_data[75] = scratchpad_resources['ramb36'].to_numpy()
    output_data[76] = scratchpad_resources['ramb18'].to_numpy()
    output_data[77] = scratchpad_resources['uram'].to_numpy()
    output_data[78] = scratchpad_resources['dsp'].to_numpy()

    output_data[79] = dma_resources['total lut'].to_numpy()
    output_data[80] = dma_resources['logic lut'].to_numpy()
    output_data[81] = dma_resources['ram lut'].to_numpy()
    output_data[82] = dma_resources['srl'].to_numpy()
    output_data[83] = dma_resources['ff'].to_numpy()
    output_data[84] = dma_resources['ramb36'].to_numpy()
    output_data[85] = dma_resources['ramb18'].to_numpy()
    output_data[86] = dma_resources['uram'].to_numpy()
    output_data[87] = dma_resources['dsp'].to_numpy()

    output_data[88] = recurrance_resources['total lut'].to_numpy()
    output_data[89] = recurrance_resources['logic lut'].to_numpy()
    output_data[90] = recurrance_resources['ram lut'].to_numpy()
    output_data[91] = recurrance_resources['srl'].to_numpy()
    output_data[92] = recurrance_resources['ff'].to_numpy()
    output_data[93] = recurrance_resources['ramb36'].to_numpy()
    output_data[94] = recurrance_resources['ramb18'].to_numpy()
    output_data[95] = recurrance_resources['uram'].to_numpy()
    output_data[96] = recurrance_resources['dsp'].to_numpy()

    output_data[97] = generate_resources['total lut'].to_numpy()
    output_data[98] = generate_resources['logic lut'].to_numpy()
    output_data[99] = generate_resources['ram lut'].to_numpy()
    output_data[100] = generate_resources['srl'].to_numpy()
    output_data[101] = generate_resources['ff'].to_numpy()
    output_data[102] = generate_resources['ramb36'].to_numpy()
    output_data[103] = generate_resources['ramb18'].to_numpy()
    output_data[104] = generate_resources['uram'].to_numpy()
    output_data[105] = generate_resources['dsp'].to_numpy()

    output_data[106] = register_resources['total lut'].to_numpy()
    output_data[107] = register_resources['logic lut'].to_numpy()
    output_data[108] = register_resources['ram lut'].to_numpy()
    output_data[109] = register_resources['srl'].to_numpy()
    output_data[110] = register_resources['ff'].to_numpy()
    output_data[111] = register_resources['ramb36'].to_numpy()
    output_data[112] = register_resources['ramb18'].to_numpy()
    output_data[113] = register_resources['uram'].to_numpy()
    output_data[114] = register_resources['dsp'].to_numpy()

    output_data[115] = core_resources['total lut'].to_numpy()
    output_data[116] = core_resources['logic lut'].to_numpy()
    output_data[117] = core_resources['ram lut'].to_numpy()
    output_data[118] = core_resources['srl'].to_numpy()
    output_data[119] = core_resources['ff'].to_numpy()
    output_data[120] = core_resources['ramb36'].to_numpy()
    output_data[121] = core_resources['ramb18'].to_numpy()
    output_data[122] = core_resources['uram'].to_numpy()
    output_data[123] = core_resources['dsp'].to_numpy()

    output_data[124] = system_bus_resources['total lut'].to_numpy()
    output_data[125] = system_bus_resources['logic lut'].to_numpy()
    output_data[126] = system_bus_resources['ram lut'].to_numpy()
    output_data[127] = system_bus_resources['srl'].to_numpy()
    output_data[128] = system_bus_resources['ff'].to_numpy()
    output_data[129] = system_bus_resources['ramb36'].to_numpy()
    output_data[130] = system_bus_resources['ramb18'].to_numpy()
    output_data[131] = system_bus_resources['uram'].to_numpy()
    output_data[132] = system_bus_resources['dsp'].to_numpy()

    return output_data


def plot_lines(axis, line_label, colors, x_axis, y_axis):
    lines = []
    for i in range(len(line_label)):
        lines.append(axis.plot(x_axis[i], y_axis[i], color=colors[i], label=line_label[i]))

    return lines

def set_axis(axis, x_axis_name, y_axis_name):
    axis.set_xlabel(x_axis_name)
    axis.set_ylabel(y_axis_name)
    axis.axis(xmin=0, ymin=0)
    axis.grid(axis='y')

    

# Create Pie chart with following figures
def resource_pie(title, axis, fu, switch, ivport, ovport, scratchpad, dma, recurrance, generate, register, core, system_bus):
    if (fu == 0 and switch == 0 and ivport == 0 and ovport == 0 and scratchpad == 0 and dma == 0 and recurrance == 0 and generate == 0 and register == 0 and core == 0 and system_bus == 0):
        axis.axis('off')
        return

    labels = ['FU', 'Switch', 'IVPort', 'OVPort', 'Scratchpad', 'DMA', 'Recurrance', 'Generate', 'Register', 'Core', 'System Bus']
    sizes = [fu, switch, ivport, ovport, scratchpad, dma, recurrance, generate, register, core, system_bus]
    colors = ['red', 'orange', 'yellow', 'green', 'dodgerblue', 'indigo', 'violet', 'teal', 'lime', 'pink', 'coral']

    zero_index = []
    for i in range(len(sizes)):
        if sizes[i] == 0:
            zero_index.append(i)

    for i in range(len(zero_index)):
        labels.pop(zero_index[i])
        sizes.pop(zero_index[i])
        colors.pop(zero_index[i])
        for j in range(len(zero_index)):
            if zero_index[j] > zero_index[i]:
                zero_index[j] -= 1
    


    axis.pie(sizes, labels=labels, colors=colors, autopct='%1.1f%%', shadow=False, startangle=90)
    axis.axis('equal')  # Equal aspect ratio ensures that pie is drawn as a circle.
    axis.set_title(title)

def resource_area(filename, dse_folder):
    
    data = load_line(filename)

    times = data[0]
    iterations = data[1]
    best = data[2]
    performance = data[3]
    overall_util = data[4]
    link_util = data[5]
    node_util = data[6]

    normalized_total_lut = data[16]
    normalized_logic_lut = data[17]
    normalized_ram_lut = data[18]
    normalized_srl = data[19]
    normalized_ff = data[20]
    normalized_ramb36 = data[21]
    normalized_ramb18 = data[22]
    normalized_uram = data[23]
    normalized_dsp = data[24]

    scaled_normalized_total_lut = data[25]
    scaled_normalized_logic_lut = data[26]
    scaled_normalized_ram_lut = data[27]
    scaled_normalized_srl = data[28]
    scaled_normalized_ff = data[29]
    scaled_normalized_ramb36 = data[30]
    scaled_normalized_ramb18 = data[31]
    scaled_normalized_uram = data[32]
    scaled_normalized_dsp = data[33]

    fu_total_lut = data[34]
    fu_logic_lut = data[35]
    fu_ram_lut = data[36]
    fu_srl = data[37]
    fu_ff = data[38]
    fu_ramb36 = data[39]
    fu_ramb18 = data[40]
    fu_uram = data[41]
    fu_dsp = data[42]

    switch_total_lut = data[43]
    switch_logic_lut = data[44]
    switch_ram_lut = data[45]
    switch_srl = data[46]
    switch_ff = data[47]
    switch_ramb36 = data[48]
    switch_ramb18 = data[49]
    switch_uram = data[50]
    switch_dsp = data[51]
    
    ivport_total_lut = data[52]
    ivport_logic_lut = data[53]
    ivport_ram_lut = data[54]
    ivport_srl = data[55]
    ivport_ff = data[56]
    ivport_ramb36 = data[57]
    ivport_ramb18 = data[58]
    ivport_uram = data[59]
    ivport_dsp = data[60]

    ovport_total_lut = data[61]
    ovport_logic_lut = data[62]
    ovport_ram_lut = data[63]
    ovport_srl = data[64]
    ovport_ff = data[65]
    ovport_ramb36 = data[66]
    ovport_ramb18 = data[67]
    ovport_uram = data[68]
    ovport_dsp = data[69]

    scratch_total_lut = data[70]
    scratch_logic_lut = data[71]
    scratch_ram_lut = data[72]
    scratch_srl = data[73]
    scratch_ff = data[74]
    scratch_ramb36 = data[75]
    scratch_ramb18 = data[76]
    scratch_uram = data[77]
    scratch_dsp = data[78]

    dma_total_lut = data[79]
    dma_logic_lut = data[80]
    dma_ram_lut = data[81]
    dma_srl = data[82]
    dma_ff = data[83]
    dma_ramb36 = data[84]
    dma_ramb18 = data[85]
    dma_uram = data[86]
    dma_dsp = data[87]

    recurrance_total_lut = data[88]
    recurrance_logic_lut = data[89]
    recurrance_ram_lut = data[90]
    recurrance_srl = data[91]
    recurrance_ff = data[92]
    recurrance_ramb36 = data[93]
    recurrance_ramb18 = data[94]
    recurrance_uram = data[95]
    recurrance_dsp = data[96]

    generate_total_lut = data[97]
    generate_logic_lut = data[98]
    generate_ram_lut = data[99]
    generate_srl = data[100]
    generate_ff = data[101]
    generate_ramb36 = data[102]
    generate_ramb18 = data[103]
    generate_uram = data[104]
    generate_dsp = data[105]

    register_total_lut = data[106]
    register_logic_lut = data[107]
    register_ram_lut = data[108]
    register_srl = data[109]
    register_ff = data[110]
    register_ramb36 = data[111]
    register_ramb18 = data[112]
    register_uram = data[113]
    register_dsp = data[114]

    core_total_lut = data[115]
    core_logic_lut = data[116]
    core_ram_lut = data[117]
    core_srl = data[118]
    core_ff = data[119]
    core_ramb36 = data[120]
    core_ramb18 = data[121]
    core_uram = data[122]
    core_dsp = data[123]

    system_bus_total_lut = data[124]
    system_bus_logic_lut = data[125]
    system_bus_ram_lut = data[126]
    system_bus_srl = data[127]
    system_bus_ff = data[128]
    system_bus_ramb36 = data[129]
    system_bus_ramb18 = data[130]
    system_bus_uram = data[131]
    system_bus_dsp = data[132]


    fig, ((ax1, ax2, ax3), (ax4, ax5, ax6), (ax7, ax8, ax9)) = plt.subplots(3, 3)
    fig.set_size_inches(16, 12)
    
    resource_pie('Total Lut', ax1, fu_total_lut[-1], switch_total_lut[-1], ivport_total_lut[-1], ovport_total_lut[-1], scratch_total_lut[-1], dma_total_lut[-1], recurrance_total_lut[-1], generate_total_lut[-1], register_total_lut[-1], core_total_lut[-1], system_bus_total_lut[-1])
    
    resource_pie('Logic Lut', ax2, fu_logic_lut[-1], switch_logic_lut[-1], ivport_logic_lut[-1], ovport_logic_lut[-1], scratch_logic_lut[-1], dma_logic_lut[-1], recurrance_logic_lut[-1], generate_logic_lut[-1], register_logic_lut[-1], core_logic_lut[-1], system_bus_logic_lut[-1])
    
    resource_pie('Ram Lut', ax3, fu_ram_lut[-1], switch_ram_lut[-1], ivport_ram_lut[-1], ovport_ram_lut[-1], scratch_ram_lut[-1], dma_ram_lut[-1], recurrance_ram_lut[-1], generate_ram_lut[-1], register_ram_lut[-1], core_ram_lut[-1], system_bus_ram_lut[-1])
    
    resource_pie('SRL', ax4, fu_srl[-1], switch_srl[-1], ivport_srl[-1], ovport_srl[-1], scratch_srl[-1], dma_srl[-1], recurrance_srl[-1], generate_srl[-1], register_srl[-1], core_srl[-1], system_bus_srl[-1])

    resource_pie('FF', ax5, fu_ff[-1], switch_ff[-1], ivport_ff[-1], ovport_ff[-1], scratch_ff[-1], dma_ff[-1], recurrance_ff[-1], generate_ff[-1], register_ff[-1], core_ff[-1], system_bus_ff[-1])

    resource_pie('Ramb36', ax6, fu_ramb36[-1], switch_ramb36[-1], ivport_ramb36[-1], ovport_ramb36[-1], scratch_ramb36[-1], dma_ramb36[-1], recurrance_ramb36[-1], generate_ramb36[-1], register_ramb36[-1], core_ramb36[-1], system_bus_ramb36[-1])

    resource_pie('Ramb18', ax7, fu_ramb18[-1], switch_ramb18[-1], ivport_ramb18[-1], ovport_ramb18[-1], scratch_ramb18[-1], dma_ramb18[-1], recurrance_ramb18[-1], generate_ramb18[-1], register_ramb18[-1], core_ramb18[-1], system_bus_ramb18[-1])

    resource_pie('Uram', ax8, fu_uram[-1], switch_uram[-1], ivport_uram[-1], ovport_uram[-1], scratch_uram[-1], dma_uram[-1], recurrance_uram[-1], generate_uram[-1], register_uram[-1], core_uram[-1], system_bus_uram[-1])

    resource_pie('Dsp', ax9, fu_dsp[-1], switch_dsp[-1], ivport_dsp[-1], ovport_dsp[-1], scratch_dsp[-1], dma_dsp[-1], recurrance_dsp[-1], generate_dsp[-1], register_dsp[-1], core_dsp[-1], system_bus_dsp[-1])

    plt.savefig(dse_folder + '/dse-final-resources.png', dpi=300)
    
    fig, ax1 = plt.subplots(1, 1)
    fig.set_size_inches(16, 12)

    ax1.plot(iterations, normalized_total_lut, color='blue', label='Total Lut')
    ax1.plot(iterations, normalized_logic_lut, color='green', label='Logic Lut')
    ax1.plot(iterations, normalized_ram_lut, color='red', label='Ram Lut')
    ax1.plot(iterations, normalized_srl, color='purple', label='SRL')
    ax1.plot(iterations, normalized_ff, color='orange', label='Flip Flop')
    ax1.plot(iterations, normalized_ramb36, color='purple', label='Ramb36')
    ax1.plot(iterations, normalized_ramb18, color='cyan', label='Ramb18')
    ax1.plot(iterations, normalized_uram, color='yellow', label='Uram')
    ax1.plot(iterations, normalized_dsp, color='lime', label='DSP')

    ax1.set_ylim(ymin=0)
    ax1.set_xlim(xmin=0)
    ax1.set_xlabel('Iterations')
    ax1.set_ylabel('Normalized Resources')
    ax1.legend(loc='upper left')

    plt.savefig(dse_folder + '/dse-normalized-resources.png', dpi=300)
    plt.close()

    fig, ax1 = plt.subplots(1, 1)
    fig.set_size_inches(16, 12)

    
    ax1.plot(iterations, scaled_normalized_total_lut, color='blue', label='Total Lut')
    ax1.plot(iterations, scaled_normalized_logic_lut, color='green', label='Logic Lut')
    ax1.plot(iterations, scaled_normalized_ram_lut, color='red', label='Ram Lut')
    ax1.plot(iterations, scaled_normalized_srl, color='purple', label='SRL')
    ax1.plot(iterations, scaled_normalized_ff, color='orange', label='Flip Flop')
    ax1.plot(iterations, scaled_normalized_ramb36, color='purple', label='Ramb36')
    ax1.plot(iterations, scaled_normalized_ramb18, color='cyan', label='Ramb18')
    ax1.plot(iterations, scaled_normalized_uram, color='yellow', label='Uram')
    ax1.plot(iterations, scaled_normalized_dsp, color='lime', label='DSP')
    
    ax1.set_ylim(ymin=0)
    ax1.set_xlim(xmin=0)
    ax1.set_xlabel('Iterations')
    ax1.set_ylabel('Scaled Normalized Resources')
    ax1.legend(loc='upper left')

    plt.savefig(dse_folder + '/dse-scaled-normalized-resources.png', dpi=300)
    plt.close()

    fig, ax1 = plt.subplots(1, 1)
    fig.set_size_inches(16, 12)
    ax1.plot(iterations, overall_util, color='blue', label='Overall Utilization')
    ax1.plot(iterations, link_util, color='green', label='Link Utilization')
    ax1.plot(iterations, node_util, color='red', label='Node Utilization')
    ax1.set_ylim(ymin=0)
    ax1.set_xlim(xmin=0)
    ax1.set_xlabel('Iterations')
    ax1.set_ylabel('Percent Utilized')
    ax1.legend(loc='upper left')

    plt.savefig(dse_folder + '/dse-utilization.png', dpi=300)
    plt.close()

    print("Finished!")

objectives_csv = sys.argv[1]

if objectives_csv.endswith('.csv'):
    split_path = objectives_csv.split('/')
    dse_folder = '.'
    for i in range(len(split_path) - 1):
        dse_folder += '/' + split_path[i]
    dse_folder += '/dse_graphs'

    if not os.path.exists(dse_folder):
        os.mkdir(dse_folder)
    
    resource_area(objectives_csv, dse_folder)
else:
    print('Objectives CSV File Required. Given: ' + objectives_csv)