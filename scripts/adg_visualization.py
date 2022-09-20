import pandas as pd
import os
import sys
from os.path import dirname
import json

from pyvis.network import Network

def add_function_unit(network, function_units, node):
    name, number = node.split('.')
    operations = function_units[node]['dsagen2.comp.config.CompKeys$DsaOperations$']['OperationDataTypeSet']
    datawidth = function_units[node]['dsagen2.comp.config.CompKeys$CompNode$']['compBits']
    size = (int(datawidth) / 64) * 25

    title = name + ' ' + number + '\n'
    title += 'Operations: \n'
    for operation in operations:
        title += '     ' + operation + '\n'

    network.add_node(node, label='PE ' + number, title=title, size=size, shape='square', color='#f46049')


def add_switch(network, switches, node):
    name, number = node.split('.')
    datawidth = switches[node]['dsagen2.comp.config.CompKeys$CompNode$']['compBits']
    size = (int(datawidth) / 64) * 25
        
    network.add_node(node, label='SW ' + number, size=size, color='#fbbe5b')


def adg_vis(filename, graph_name):
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
    dma_nodes = pd.DataFrame()
    scratchpad_nodes = pd.DataFrame()
    rec_nodes = pd.DataFrame()
    gen_nodes = pd.DataFrame()
    reg_nodes = pd.DataFrame()

    for node in nodes:
        if not isinstance(nodes[node]['dsagen2.sync.config.SyncKeys$IVPNode$'], float):
            ivp_nodes[node] = nodes[node].dropna()
        if not isinstance(nodes[node]['dsagen2.sync.config.SyncKeys$OVPNode$'], float):
            ovp_nodes[node] = nodes[node].dropna()
        if not isinstance(nodes[node]['dsagen2.mem.config.MemKeys$MemNode$'], float):
            mem_nodes[node] = nodes[node].dropna()
            if nodes[node]['dsagen2.mem.config.MemKeys$MemNode$']['nodeType'] == 'ScratchpadMemory':
                scratchpad_nodes[node] = nodes[node].dropna()
            elif nodes[node]['dsagen2.mem.config.MemKeys$MemNode$']['nodeType'] == 'DirectMemoryAccess':
                dma_nodes[node] = nodes[node].dropna()
            elif nodes[node]['dsagen2.mem.config.MemKeys$MemNode$']['nodeType'] == 'RecurrenceEngine':
                rec_nodes[node] = nodes[node].dropna()
            elif nodes[node]['dsagen2.mem.config.MemKeys$MemNode$']['nodeType'] == 'GenerateEngine':
                gen_nodes[node] = nodes[node].dropna()
            else:
                reg_nodes[node] = nodes[node].dropna()
        if not isinstance(nodes[node]['dsagen2.comp.config.CompKeys$CompNode$'], float):
            if nodes[node]['dsagen2.comp.config.CompKeys$CompNode$']['nodeType'] == "ProcessingElement":
                functional_units[node] = nodes[node].dropna()
            else:
                switches[node] = nodes[node].dropna()
            comp_nodes[node] = nodes[node].dropna()
    
    net = Network(directed=True, width='100%')

    for node in functional_units:
        add_function_unit(net, functional_units, node)

    for node in switches:
        add_switch(net, switches, node)
    
    for node in ivp_nodes:
        name, number = node.split('.')
        net.add_node(node, label='IVP ' + number, color='#c895f6')
    
    for node in ovp_nodes:
        name, number = node.split('.')
        net.add_node(node, label='OVP ' + number, color='#571a8e')

    for node in dma_nodes:
        name, number = node.split('.')
        net.add_node(node, label='DMA ' + number, color='#00B7EB')
    
    for node in scratchpad_nodes:
        name, number = node.split('.')
        net.add_node(node, label='SPM ' + number, color='#89CFF0')
    
    for node in rec_nodes:
        name, number = node.split('.')
        net.add_node(node, label='REC ' + number, color='#1e9ae0')
    
    for node in gen_nodes:
        name, number = node.split('.')
        net.add_node(node, label='GEN ' + number, color='#0067A5')
    
    for node in reg_nodes:
        name, number = node.split('.')
        net.add_node(node, label='REG ' + number, color='#0F3D92')



    for i in range(len(links)):
        link = links.iloc[i]
        physics = True
        lineStyle = {'type': 'dynamic'}
        if link['SourceNodeType'] != "ProcessingElement" and link['SourceNodeType'] != "Switch":
            physics = False
            lineStyle['type'] = 'discrete'
        if link['SinkNodeType'] != "ProcessingElement" and link['SinkNodeType'] != "Switch":
            physics = False
            lineStyle['type'] = 'discrete'
            

        sourceId = str(link['SourceNodeType']) + '.' + str(link['SourceNodeId'])
        sinkId = str(link['SinkNodeType']) + '.' + str(link['SinkNodeId'])
        net.add_edge(sourceId, sinkId, arrows='to', smooth=lineStyle, physics=physics, weight=1)
    
    
    for node in ivp_nodes:
        name, number = node.split('.')
        connectId = int(number) + 1
        if connectId < len(ivp_nodes.columns):
            net.add_edge(node, name + '.' + str(connectId), hidden=True)
            net.add_edge(name + '.' + str(connectId), node, hidden=True)

    
    for node in ovp_nodes:
        name, number = node.split('.')
        connectId = int(number) + 1
        if connectId < len(ovp_nodes.columns):
            net.add_edge(node, name + '.' + str(connectId), hidden=True)
            net.add_edge(name + '.' + str(connectId), node, hidden=True)
    
    for node in dma_nodes:
        name, number = node.split('.')
        connectId = int(number) + 1
        if connectId < len(dma_nodes.columns):
            net.add_edge(node, name + '.' + str(connectId), hidden=True)
            net.add_edge(name + '.' + str(connectId), node, hidden=True)
    
    for node in scratchpad_nodes:
        name, number = node.split('.')
        connectId = int(number) + 1
        if connectId < len(scratchpad_nodes.columns):
            net.add_edge(node, name + '.' + str(connectId), hidden=True)
            net.add_edge(name + '.' + str(connectId), node, hidden=True)

    for node in rec_nodes:
        name, number = node.split('.')
        connectId = int(number) + 1
        if connectId < len(rec_nodes.columns):
            net.add_edge(node, name + '.' + str(connectId), hidden=True)
            net.add_edge(name + '.' + str(connectId), node, hidden=True)
    
    for node in gen_nodes:
        name, number = node.split('.')
        connectId = int(number) + 1
        if connectId < len(gen_nodes.columns):
            net.add_edge(node, name + '.' + str(connectId), hidden=True)
            net.add_edge(name + '.' + str(connectId), node, hidden=True)

    for node in reg_nodes:
        name, number = node.split('.')
        connectId = int(number) + 1
        if connectId < len(reg_nodes.columns):
            net.add_edge(node, name + '.' + str(connectId), hidden=True)
            net.add_edge(name + '.' + str(connectId), node, hidden=True)
    
    net.show(graph_name + '.html')

    print('Finished HTML: ' + graph_name)
    


adg_file = sys.argv[1]

if adg_file.endswith('.json'):
    adg_vis(adg_file, adg_file[:-5])
else:
    print('ADG JSON File Required. Given: ' + adg_file)