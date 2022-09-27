import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import pandas as pd

import os.path
from os import path
import sys

from math import atan2, degrees, ceil, floor
import numpy as np

def get_resources(token):
    '''total lut: 65766.6, logic lut: 53923, ram lut: 12235.6, ff: 48493.8'''
    resources = token.str.split(',', expand=True)
    resources[0] = resources[0].str.extract('(\d+)', expand=False).astype(int)
    resources[1] = resources[1].str.extract('(\d+)', expand=False).astype(int)
    resources[2] = resources[2].str.extract('(\d+)', expand=False).astype(int)
    resources[3] = resources[3].str.extract('(\d+)', expand=False).astype(int)
    resources[4] = resources[4].str.extract('(\d+)', expand=False).astype(int)
    resources[5] = resources[5].str.extract('(\d+)', expand=False).astype(int)
    resources[6] = resources[6].str.extract('(\d+)', expand=False).astype(int)
    resources[7] = resources[7].str.extract('(\d+)', expand=False).astype(int)
    resources[8] = resources[8].str.extract('(\d+)', expand=False).astype(int)
    resources = resources.set_axis(['total lut', 'logic lut', 'srl', 'ram lut', 'ff', 'ramb36', 'ramb18', 'uram', 'dsp'], axis=1, copy=False)
    return resources

def get_dfg_performance(token):
    axis = []
    performances = token.str.split(',', expand=True)
    for i in range(performances.shape[1]):
        performances[i] = performances[i].str.split(':').str[1]
        performances[i] = performances[i].str.extract('(\d+)', expand=False).astype(float)
    return performances

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
    line_data =  pd.read_csv(filepath)
    line_data_resources =  get_resources(line_data['Best Single-Core Resources'])
    '''
    fu_resources = get_resources(line_data['Best FU Resource'])
    switch_resources = get_resources(line_data['Best Switch Resource'])
    vport_resources = get_resources(line_data['Best VPort Resource'])
    mem_resources = get_resources(line_data['Best Mem Resource'])
    '''

    output_data = [0] * 18


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

    line_data =  pd.read_csv(filepath)
    output_data[0] = line_data['Time'].to_numpy()
    output_data[1] = line_data['Iteration'].to_numpy()
    output_data[2] = line_data['Best Objective Performance'].to_numpy()
    output_data[3] = line_data['Best Objective Area'].to_numpy()
    output_data[4] = line_data['Best Number of Cores']
    output_data[5] = line_data['Best System Bus Size']
    load_resources(line_data_resources, 6)
    return output_data


def plot_lines(axis, colors, x_axis, y_axis):
    lines = []
    lines.append(axis.plot(x_axis[0], y_axis[0], color=colors[0]))

    return lines

def set_axis(axis, x_axis_name, y_axis_name):
    axis.set_xlabel(x_axis_name)
    axis.set_ylabel(y_axis_name)
    axis.axis(xmin=0, ymin=0)
    axis.grid(axis='y')

def create_graph(filename, graph_folder):
    colors = ['red']

    metrics = ['Time', 'Iteration', 'Best Objective', 'Best Performance', 'Best Resources', 'total lut', 'logic lut', 'ram lut', 'ff', 'total lut', 'logic lut', 'ram lut', 'ff', 'total lut', 'logic lut', 'ram lut', 'ff', 'total lut', 'logic lut', 'ram lut', 'ff', 'total lut', 'logic lut', 'ram lut', 'ff']

    data = np.zeros((len(metrics) + 30, 1), dtype=object)
    data = load_to_global(i, load_line(filename), data)

    times = data[0, :]
    iterations = data[1, :]
    performance = data[2, :]
    area = data[3, :]
    cores = data[4, :]
    system_bus = data[5, :]
    total_lut = data[6, :]
    logic_lut = data[7, :]
    ram_lut = data[8, :]
    srl = data[9, :]
    ff = data[10, :]

    fig, (ax1, ax2) = plt.subplots(1, 2)

    p1axis = plot_lines(ax1, colors, times, performance)
    set_axis(ax1, 'Time (S)', 'Estimated IPC')

    p2axis = plot_lines(ax2, colors, times, area)
    set_axis(ax2, 'Time (S)', 'Single-Core-Area')

    fig.set_size_inches(14, 8)
    fig.tight_layout()
    plt.savefig(graph_folder + '/dse-objective.png', dpi=300)

    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2)
    fig.tight_layout()

    p5axis = plot_lines(ax1, colors, times, total_lut)
    set_axis(ax1, 'Time (S)', 'Total Lut')

    p6axis =  plot_lines(ax2, colors, times, logic_lut)
    set_axis(ax2, 'Time (S)', 'Logic LUT')

    p7axis =  plot_lines(ax3, colors, times, ram_lut)
    set_axis(ax3, 'Time (S)', 'Ram LUT')

    p8axis =  plot_lines(ax4, colors, times, ff)
    set_axis(ax4, 'Time (S)', 'Flip Flop')

    fig.set_size_inches(14, 8)
    plt.savefig(graph_folder + '/dse-resources.png', dpi=300)


objectives_csv = sys.argv[1]

if objectives_csv.endswith('.csv'):
    split_path = objectives_csv.split('/')
    dse_folder = '.'
    for i in range(len(split_path) - 1):
        dse_folder += '/' + split_path[i]
    dse_folder += '/dse_graphs'

    if not os.path.exists(dse_folder):
        os.mkdir(dse_folder)
    
    create_graph(objectives_csv, dse_folder)
else:
    print('Objectives CSV File Required. Given: ' + objectives_csv)

