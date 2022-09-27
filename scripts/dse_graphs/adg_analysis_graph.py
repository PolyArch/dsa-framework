import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
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

def bar(axis, data, value, asStr=True):    
    indices = 0
    if asStr:
        indices = data[value].value_counts().index.to_numpy(dtype=str)
    else:
        indices = data[value].value_counts().index.to_numpy()
    values = data[value].value_counts().to_numpy()
    bar = axis.bar(indices, values)

    for index, value in enumerate(indices):
        axis.text(value, values[index], str(values[index]))
    return bar

def functional_unit_bar(axis, data):
    data_lists = data['instructions'].to_numpy().flatten()
    instructions = np.array([])
    for data in data_lists:
        instructions = np.append(instructions, data)

    indices = []
    values = []

    for instruction in instructions:
        if instruction in indices:
            values[np.where(np.array(indices)==instruction)[0][0]] += 1
        else:
            indices.append(instruction)
            values.append(1)
    
    bar = axis.bar(indices, values)
    axis.set_xticks(indices)
    axis.set_xticklabels(indices, rotation=45)

    for index, value in enumerate(indices):
        axis.text(value, values[index], str(values[index]))

def flow_control_pie(axis, data):
    indices = data['flow_control'].value_counts().index.to_numpy()
    values = data['flow_control'].value_counts().to_numpy()

    colors = ['#1f77b4', '#ff7f0e']
    if len(indices) == 0:
        axis.axis('off')
        return
    if len(indices) == 1:
        if indices[0]:
            colors = ['#1f77b4']
        else:
            colors = ['#ff7f0e']
    elif not indices[0]:
        colors = ['#ff7f0e', '#1f77b4']

    axis.axis('equal')
    return axis.pie(values, labels=indices, colors=colors, autopct='%1.1f%%')


def input_output_bar(axis, in_links, out_links):
    in_links_columns = np.unique(in_links)
    in_links_values = [0] * len(in_links_columns)
    for value in in_links:
        for i, column in enumerate(in_links_columns):
            if value == column:
                in_links_values[i] += 1
    
    out_links_columns = np.unique(out_links)
    out_links_values = [0] * len(out_links_columns)
    for value in out_links:
        for i, column in enumerate(out_links_columns):
            if value == column:
                out_links_values[i] += 1

    concatonated_index = np.unique(np.append(in_links_columns, out_links_columns))

    final_outlinks_values = [0] * len(concatonated_index)
    final_inlinks_values = [0] * len(concatonated_index)
    for i, index in enumerate(concatonated_index):
        if index in in_links_columns:
            for j, value in enumerate(in_links_columns):
                if index == value:
                    final_inlinks_values[i] = in_links_values[j]
        if index in out_links_columns:
            for j, value in enumerate(out_links_columns):
                if index == value:
                    final_outlinks_values[i] = out_links_values[j]
    
    width = 0.25
    r = np.arange(len(concatonated_index))

    bar = axis.bar(r, final_inlinks_values, width=width, facecolor='orange', edgecolor='black', label='in links')
    bar = axis.bar(r + width, final_outlinks_values, width=width, facecolor='blue', edgecolor='black', label='out links')

    axis.set_xticks(r)
    axis.set_xticklabels(concatonated_index)

def _finditem(obj, key):
    if key in obj: return obj[key]
    for k, v in obj.items():
        if isinstance(v,dict):
            item = _finditem(v, key)
            if item is not None:
                return item

def adg_analysis(filename, name):
    if (not os.path.exists(filename)):
        return
    with open(filename) as f:
        data = json.load(f)

    links = pd.DataFrame.from_dict(data['DSAGenEdges'])
    nodes = pd.DataFrame(data['DSAGenNodes'])
    
    comp_nodes = pd.DataFrame()
    functional_units = pd.DataFrame()
    switches = pd.DataFrame()
    
    ivp_nodes = pd.DataFrame()
    ovp_nodes = pd.DataFrame()

    mem_nodes = pd.DataFrame()

    for node in nodes:
        if not isinstance(nodes[node]['dsagen2.sync.config.SyncKeys$IVPNode$'], float):
            ivp_nodes[node] = nodes[node].dropna()
        if not isinstance(nodes[node]['dsagen2.sync.config.SyncKeys$OVPNode$'], float):
            ovp_nodes[node] = nodes[node].dropna()
        if not isinstance(nodes[node]['dsagen2.mem.config.MemKeys$MemNode$'], float):
            mem_nodes[node] = nodes[node].dropna()

        if not isinstance(nodes[node]['dsagen2.comp.config.CompKeys$CompNode$'], float):
            if nodes[node]['dsagen2.comp.config.CompKeys$CompNode$']['nodeType'] == "ProcessingElement":
                functional_units[node] = nodes[node].dropna()
            else:
                switches[node] = nodes[node].dropna()
            comp_nodes[node] = nodes[node].dropna()
    if not os.path.exists(name):
        os.makedirs(name)
    fig, ax1 = plt.subplots(1, 1)

    x_values, y_values = ['links', 'nodes', 'processing elements', 'switches', 'input vector port', 'output vector port', 'memory node'], [len(links), len(nodes.columns), len(functional_units.columns), len(switches.columns), len(ivp_nodes.columns), len(ovp_nodes.columns), len(mem_nodes.columns)]

    ax1.bar(x_values, y_values)
    for index, value in enumerate(x_values):
        ax1.text(value, y_values[index], str(y_values[index]))
    ax1.set_title("Per-Unit Count")
    ax1.set_ylabel('Count')

    fig.set_size_inches(16, 12)
    fig.tight_layout()
    plt.savefig(name + '/' + name.split('/')[-1] + '-adg_count.png', dpi=300)
    plt.close()


    fig, (ax1, ax2, ax3, ax4) = plt.subplots(4, 1)

    pe_names = list(functional_units.columns)
    pe_inputs = [0 for i in range(len(pe_names))]
    pe_outputs = [0 for i in range(len(pe_names))]

    switch_names = list(switches.columns)
    switch_inputs = [0 for i in range(len(switch_names))]
    switch_outputs = [0 for i in range(len(switch_names))]

    ivp_names = list(ivp_nodes.columns)
    ivp_inputs = [0 for i in range(len(ivp_names))]
    ivp_outputs = [0 for i in range(len(ivp_names))]

    ovp_names = list(ovp_nodes.columns)
    ovp_inputs = [0 for i in range(len(ovp_names))]
    ovp_outputs = [0 for i in range(len(ovp_names))]

    for i in range(len(links)):
        link = links.iloc[i]
        if (link['SourceNodeType'] == "ProcessingElement"):
            for i, pe in enumerate(pe_names):
                pe_id = int(pe.split('.')[1])
                if (pe_id == link['SourceNodeId']):
                    pe_inputs[i] += 1
        if (link['SinkNodeType'] == "ProcessingElement"):
            for i, pe in enumerate(pe_names):
                pe_id = int(pe.split('.')[1])
                if (pe_id == link['SinkNodeId']):
                    pe_outputs[i] += 1
        if (link['SourceNodeType'] == "Switch"):
            for i, sw in enumerate(switch_names):
                sw_id = int(sw.split('.')[1])
                if (sw_id == link['SourceNodeId']):
                    switch_inputs[i] += 1
        if (link['SinkNodeType'] == "Switch"):
            for i, sw in enumerate(switch_names):
                sw_id = int(sw.split('.')[1])
                if (sw_id == link['SinkNodeId']):
                    switch_outputs[i] += 1
        if (link['SourceNodeType'] == "InputVectorPort"):
            for i, ivp in enumerate(ivp_names):
                ivp_id = int(ivp.split('.')[1])
                if (ivp_id == link['SourceNodeId']):
                    ivp_inputs[i] += 1
        if (link['SinkNodeType'] == "InputVectorPort"):
            for i, ivp in enumerate(ivp_names):
                ivp_id = int(ivp.split('.')[1])
                if (ivp_id == link['SinkNodeId']):
                    ivp_outputs[i] += 1
        if (link['SourceNodeType'] == "OutputVectorPort"):
            for i, ivp in enumerate(ovp_names):
                ovp_id = int(ivp.split('.')[1])
                if (ovp_id == link['SourceNodeId']):
                    ovp_inputs[i] += 1
        if (link['SinkNodeType'] == "OutputVectorPort"):
            for i, ovp in enumerate(ovp_names):
                ovp_id = int(ovp.split('.')[1])
                if (ovp_id == link['SinkNodeId']):
                    ovp_outputs[i] += 1

    input_output_bar(ax1, pe_outputs, pe_inputs)
    ax1.set_ylabel('# Processing Elements')
    ax1.set_xlabel('Degree')
    ax1.set_title("Processing Elements")

    input_output_bar(ax2, switch_outputs, switch_inputs)
    ax2.set_ylabel('# Switches')
    ax2.set_xlabel('Degree')
    ax2.set_title("Switches")

    input_output_bar(ax3, ivp_outputs, ivp_inputs)
    ax3.set_ylabel('# Input Vector Ports')
    ax3.set_xlabel('Degree')
    ax3.set_title("Input Vector Port")

    input_output_bar(ax4, ovp_outputs, ovp_inputs)
    ax4.set_ylabel('# Output Vector Ports')
    ax4.set_xlabel('Degree')
    ax4.set_title("Output Vector Port")
    fig.legend(handles=[Patch(facecolor='orange', edgecolor='black',
                         label='Input-Links'), Patch(facecolor='blue', edgecolor='black',
                         label='Output-Links')])


    fig.set_size_inches(16, 12)
    fig.tight_layout()
    plt.savefig(name + '/' + name.split('/')[-1] + '-in-out-adg.png', dpi=300)
    plt.close()

    print('Finished!')

adg_file = sys.argv[1]

if adg_file.endswith('.json'):
    graph_folder = adg_file[:-5]
    if not os.path.exists(graph_folder):
        os.mkdir(graph_folder)
    adg_analysis(adg_file, adg_file[:-5])
else:
    print('ADG JSON File Required. Given: ' + adg_file)