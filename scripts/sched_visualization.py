import pandas as pd
import os
import sys
import random
import json

from pyvis.network import Network


def add_function_unit(network, node):
    name, number = node.split('.')
    operations = functional_units[node]['dsagen2.comp.config.CompKeys$DsaOperations$']['OperationDataTypeSet']
    datawidth = functional_units[node]['dsagen2.comp.config.CompKeys$CompNode$']['compBits']
    size = (int(datawidth) / 64) * 25

    title = name + ' ' + number + '\n'
    title += 'Operations: \n'
    for operation in operations:
        title += '\t' + operation + '\n'

    network.add_node(node, label='PE ' + number, title=title, size=size, shape='square', color='#f46049')


def add_switch(network, node):
    name, number = node.split('.')
    datawidth = switches[node]['dsagen2.comp.config.CompKeys$CompNode$']['compBits']
    size = (int(datawidth) / 64) * 25
        
    network.add_node(node, label='SW ' + number, size=size, color='#fbbe5b')

def generate_adg_graph(filename, graph_name, graph_folder, schedule):
    # First Check if file exists
    if (not os.path.exists(filename)):
        return
    with open(filename) as f:
        data = json.load(f)
    
    # Get the Data from the file
    links = pd.DataFrame.from_dict(data['DSAGenEdges'])
    nodes = pd.DataFrame(data['DSAGenNodes'])
    
    vertices = pd.DataFrame(schedule['Vertices'])
    edges = pd.DataFrame(schedule['Edges'])
    
    # Parse the input file
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


    # Add Functional Units
    for node in functional_units:
        name, number = node.split('.')
        operations = functional_units[node]['dsagen2.comp.config.CompKeys$DsaOperations$']['OperationDataTypeSet']
        datawidth = functional_units[node]['dsagen2.comp.config.CompKeys$CompNode$']['compBits']
        size = (int(datawidth) / 64) * 25

        title = name + ' ' + number + '\n'
        title += 'Operations: \n'
        for operation in operations:
            title += '\t' + operation + '\n'

        net.add_node(node, label='PE ' + number, title=title, size=size, shape='square', color='#CCDBDC')

    # Add Switches
    for node in switches:
        name, number = node.split('.')
        datawidth = switches[node]['dsagen2.comp.config.CompKeys$CompNode$']['compBits']
        size = (int(datawidth) / 64) * 25
        
        net.add_node(node, label='SW ' + number, shape='circle', size=size, color='#CCDBDC')
    

    # Add Input Vector Ports
    for node in ivp_nodes:
        name, number = node.split('.')
        net.add_node(node, label='IVP ' + number, shape='circle', color='#CCDBDC')
    
    # Add Output Vector Ports
    for node in ovp_nodes:
        name, number = node.split('.')
        net.add_node(node, label='OVP ' + number, shape='circle', color='#CCDBDC')

    # Add Direct Memory Access
    for node in dma_nodes:
        name, number = node.split('.')
        net.add_node(node, label='DMA ' + number, shape='circle', color='#CCDBDC')
    
    # Add Scratchpad
    for node in scratchpad_nodes:
        name, number = node.split('.')
        net.add_node(node, label='SPM ' + number, shape='circle', color='#CCDBDC')
    
    for node in rec_nodes:
        name, number = node.split('.')
        net.add_node(node, label='REC ' + number, shape='circle', color='#CCDBDC')
    
    for node in gen_nodes:
        name, number = node.split('.')
        net.add_node(node, label='GEN ' + number, shape='circle', color='#CCDBDC')
    
    for node in reg_nodes:
        name, number = node.split('.')
        net.add_node(node, label='REG ' + number, shape='circle', color='#CCDBDC')


    # Add Links
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
        net.add_edge(sourceId, sinkId, arrows='to', smooth=lineStyle, color={'color': '#CCDBDC', 'opacity': .1}, dashes=True, physics=physics, weight=1)
    
    # Add hidden links between similar data and vector port types
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

    # First change the colors for vertices
    vertexNodes = {}

    for _, row in vertices.iterrows():
        node = row['MappedNodeId']
        if node in vertexNodes:
            vertexNodes[node].append(row)
        else:
            vertexNodes[node] = [row]


    for node in net.nodes:
        if node['id'] in vertexNodes:
            vertex = vertexNodes[node['id']]
            instructions = ''
            vertexNames = ''
            for v in vertex:
                vertexNames += str(v['VertexId']) + '.'
                if v['VertexType'] == 'Instruction':
                    instructions += str(v['Instruction Name']) + '\n'
            
            if instructions != '':
                node['shape'] = 'box'
                node['label'] = instructions[:-1]
            else:
                node['shape'] = 'circle'

            node['vertices'] = vertexNames[:-1]

    default_colors = ['#ff8c00', '#00FF00', '#cd853f', '#00FFFF', '#FF00FF', '#5f9ea0', '#FCF5C7', '#FFC09F', '#ADF7B6', '#D7FFF1', '#9BC1BC', '#DAF7DC', '#FAFF81', '#BDA0BC', '#C1F7DC']
    num_used = 0
    
    valueColors = {}

    nodeColors = {}
    linkColors = {}

    for _, row in edges.iterrows():
        edge_links = row['Links']
        edge_id = int(row['EdgeId'])
        value = str(row['ValueNode']) + '.' + str(row['ValueIndex'])
        
        # Get the Color for this Value
        if value not in valueColors:
            if num_used >= len(default_colors):
                default_colors.append("#" + "%06x" % random.randint(0, 0xFFFFFF))
            valueColors[value] = default_colors[num_used]
            num_used += 1
        color = valueColors[value]

        for i, link in enumerate(edge_links):
            sourceId = str(link['SourceNodeType']) + '.' + str(link['SourceNodeId'])
            sinkId = str(link['SinkNodeType']) + '.' + str(link['SinkNodeId'])
            linkId = sourceId + '->' + sinkId

            if sourceId not in nodeColors:
                nodeColors[sourceId] = [color]
            elif color not in nodeColors[sourceId]:
                nodeColors[sourceId].append(color)
            
            # Only add sink if its not last node
            if i != len(edge_links) - 1:
                if sinkId not in nodeColors:
                    nodeColors[sinkId] = [color]
                elif color not in nodeColors[sinkId]:
                    nodeColors[sinkId].append(color)
            
            if linkId not in linkColors:
                linkColors[linkId] = [color]
            elif color not in linkColors[linkId]:
                linkColors[linkId].append(color)
    
    for node in net.nodes:
        name = node['id']
        if node['id'] in vertexNodes:
            if name in nodeColors:
                node['color'] = nodeColors[name][0]
    
    for link in net.edges:
        name = link['from'] + '->' + link['to']
        if name in linkColors:
            link['color'] = linkColors[name][0]
            link['dashes'] = False

    net.show(graph_folder + '/' + graph_name + '.html')
    print('Finished: ' + graph_folder + '/' + graph_name)

def adg_vis(filename, graph_name):
    # First Check if file exists
    if (not os.path.exists(filename)):
        return
    
    with open(filename) as f:
        data = json.load(f)

    if 'Workloads' not in data:
        return

    graph_folder = filename[:-5]

    if not os.path.exists(graph_folder):
        os.mkdir(graph_folder)

    for workload_id, workload in enumerate(data['Workloads']):
        for schedule_id, schedule in enumerate(workload['Schedules']):
            name = graph_name + '-' + str(workload_id) + '_' + str(schedule_id)
            generate_adg_graph(filename, name, graph_folder, schedule)
        
    print('Finished: ' + graph_name)


adg_file = sys.argv[1]

if adg_file.endswith('.json'):
    adg_vis(adg_file, adg_file.split('/')[-1][:-5])
else:
    print('ADG JSON File Required. Given: ' + adg_file)