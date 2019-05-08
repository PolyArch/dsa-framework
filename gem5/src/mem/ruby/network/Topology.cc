/*
 * Copyright (c) 1999-2008 Mark D. Hill and David A. Wood
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met: redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer;
 * redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution;
 * neither the name of the copyright holders nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mem/ruby/network/Topology.hh"

#include <cassert>

#include "base/trace.hh"
#include "debug/RubyNetwork.hh"
#include "mem/ruby/common/NetDest.hh"
#include "mem/ruby/network/BasicLink.hh"
#include "mem/ruby/network/Network.hh"
#include "mem/ruby/slicc_interface/AbstractController.hh"

using namespace std;

const int INFINITE_LATENCY = 10000; // Yes, this is a big hack

// Note: In this file, we use the first 2*m_nodes SwitchIDs to
// represent the input and output endpoint links.  These really are
// not 'switches', as they will not have a Switch object allocated for
// them. The first m_nodes SwitchIDs are the links into the network,
// the second m_nodes set of SwitchIDs represent the the output queues
// of the network.

Topology::Topology(uint32_t num_routers,
                   const vector<BasicExtLink *> &ext_links,
                   const vector<SpuExtLink *> &spu_ext_links,
                   const vector<BasicIntLink *> &int_links)
    : m_nodes(ext_links.size()+spu_ext_links.size()), ctrl_nodes(ext_links.size()), m_number_of_switches(num_routers),
    // : m_nodes(ext_links.size()), m_number_of_switches(num_routers),
      m_ext_link_vector(ext_links), spu_ext_link_vector (spu_ext_links), m_int_link_vector(int_links)
{

    // Total nodes/controllers in network
    assert(m_nodes > 1);

    // analyze both the internal and external links, create data structures.
    // The python created external links are bi-directional,
    // and the python created internal links are uni-directional.
    // The networks and topology utilize uni-directional links.
    // Thus each external link is converted to two calls to addLink,
    // one for each direction.
    //
    // External Links
    for (vector<BasicExtLink*>::const_iterator i = ext_links.begin();
         i != ext_links.end(); ++i) {
        BasicExtLink *ext_link = (*i);
        AbstractController *abs_cntrl = ext_link->params()->ext_node;
        BasicRouter *router = ext_link->params()->int_node;

        int machine_base_idx = MachineType_base_number(abs_cntrl->getType());
        int ext_idx1 = machine_base_idx + abs_cntrl->getVersion(); // input link id
        int ext_idx2 = ext_idx1 + m_nodes; // output link id (different thing taking output?)
        int int_idx = router->params()->router_id + 2*m_nodes; // router id

        // create the internal uni-directional links in both directions
        // ext to int
        addLink(ext_idx1, int_idx, ext_link);
        // int to ext
        addLink(int_idx, ext_idx2, ext_link);
    }


	// printf("new m_nodes is %d, ctrl_nodes is %d\n",m_nodes,ctrl_nodes);
    // External Links from SPU
	// printf("Controller nodes: %d\n",ctrl_nodes);
	// TODO: temp hack for now!
	int temp=ctrl_nodes; // can i do num_dirs?
    for (vector<SpuExtLink*>::const_iterator i = spu_ext_links.begin();
         i != spu_ext_links.end(); ++i) {
        SpuExtLink *spu_ext_link = (*i);
        BasicRouter *router = spu_ext_link->params()->spu_int_node;
		// TODO: use it to get the coreid later on
        int machine_base_idx = temp;
        // machine_base_idx = MachineType_base_number(MachineType_Accel);
        int ext_idx1 = machine_base_idx;
        int ext_idx2 = ext_idx1 + m_nodes;
        int int_idx = 2*m_nodes + router->params()->router_id;

        // create the internal uni-directional links in both directions
        // ext to int
        addLink(ext_idx1, int_idx, spu_ext_link);
        // int to ext
        addLink(int_idx, ext_idx2, spu_ext_link);
		temp++;
    }

    // Internal Links
    for (vector<BasicIntLink*>::const_iterator i = int_links.begin();
         i != int_links.end(); ++i) {
        BasicIntLink *int_link = (*i);
        BasicRouter *router_src = int_link->params()->src_node;
        BasicRouter *router_dst = int_link->params()->dst_node;

        PortDirection src_outport = int_link->params()->src_outport;
        PortDirection dst_inport = int_link->params()->dst_inport;

        // Store the IntLink pointers for later
        m_int_link_vector.push_back(int_link);

        int src = router_src->params()->router_id + 2*m_nodes;
        int dst = router_dst->params()->router_id + 2*m_nodes;

        // create the internal uni-directional link from src to dst
        addLink(src, dst, int_link, src_outport, dst_inport);
    }
}

void
Topology::createLinks(Network *net)
{
    // Find maximum switchID
    SwitchID max_switch_id = 0;
    for (LinkMap::const_iterator i = m_link_map.begin();
         i != m_link_map.end(); ++i) {
        std::pair<SwitchID, SwitchID> src_dest = (*i).first;
        max_switch_id = max(max_switch_id, src_dest.first);
        max_switch_id = max(max_switch_id, src_dest.second);
    }

    // Initialize weight, latency, and inter switched vectors
    int num_switches = max_switch_id+1;

	// Ok this is num_switches = (8+4)*2+4
    // printf("NUM OF SWITCHES IN THE NETWORK: %d\n", num_switches);
    // printf("NUM OF SWITCHES IN THE NETWORK: %d\n", m_number_of_switches);

    Matrix topology_weights(num_switches,
            vector<int>(num_switches, INFINITE_LATENCY));
    Matrix component_latencies(num_switches,
            vector<int>(num_switches, -1));
    Matrix component_inter_switches(num_switches,
            vector<int>(num_switches, 0));

    // Set identity weights to zero
    for (int i = 0; i < topology_weights.size(); i++) {
        topology_weights[i][i] = 0;
    }

    // Fill in the topology weights and bandwidth multipliers
    for (LinkMap::const_iterator i = m_link_map.begin();
         i != m_link_map.end(); ++i) {
        std::pair<int, int> src_dest = (*i).first;
        BasicLink* link = (*i).second.link;
        int src = src_dest.first;
        int dst = src_dest.second;
        component_latencies[src][dst] = link->m_latency;
        topology_weights[src][dst] = link->m_weight;
    }

    // Walk topology and hookup the links
    Matrix dist = shortest_path(topology_weights, component_latencies,
                                component_inter_switches);

    for (int i = 0; i < topology_weights.size(); i++) {
        for (int j = 0; j < topology_weights[i].size(); j++) {
            int weight = topology_weights[i][j];
            if (weight > 0 && weight != INFINITE_LATENCY) {
                NetDest destination_set =
                        shortest_path_to_node(i, j, topology_weights, dist);
                makeLink(net, i, j, destination_set);
            }
        }
    }
}

void
Topology::addLink(SwitchID src, SwitchID dest, BasicLink* link,
                  PortDirection src_outport_dirn,
                  PortDirection dst_inport_dirn)
{
    assert(src <= m_number_of_switches+m_nodes+m_nodes);
    assert(dest <= m_number_of_switches+m_nodes+m_nodes);

    std::pair<int, int> src_dest_pair;
    LinkEntry link_entry;

    src_dest_pair.first = src;
    src_dest_pair.second = dest;
    link_entry.link = link;
    link_entry.src_outport_dirn = src_outport_dirn;
    link_entry.dst_inport_dirn  = dst_inport_dirn;
    m_link_map[src_dest_pair] = link_entry;
}

// spu: how would we make new links here?
// spu:if I use this new convention, I wouldn't need diff makeExtInLink
// FIXME: check if these conditions are correct!
void
Topology::makeLink(Network *net, SwitchID src, SwitchID dest,
                   const NetDest& routing_table_entry)
{
	// printf("START OF MAKE LINK\n");
    // Make sure we're not trying to connect two end-point nodes
    // directly together
	// printf("m_nodes: %u\n",m_nodes);
	// printf("src: %d dest: %d\n",src,dest);
    assert(src >= 2 * m_nodes || dest >= 2 * m_nodes);

    std::pair<int, int> src_dest;
    LinkEntry link_entry;

	// printf("This function should be called 24 times, src: %d dest %d\n",src,dest);
	// printf("m_nodes: %d ctrl_nodes %d\n",m_nodes,ctrl_nodes);
    if (src < m_nodes) { // dest >= 2 * m_nodes
	  // printf("first condition this time\n");
        src_dest.first = src;
        src_dest.second = dest;
        link_entry = m_link_map[src_dest];
		if(src < ctrl_nodes) {
          net->makeExtInLink(src, dest - (2 * m_nodes), link_entry.link,
                        routing_table_entry);
		} else {
	  // printf("spu ext in this time\n"); // issue in this function
	    // printf("src is external node, dest_id should be varying with switches: %d\n", dest);
		  net->makeSpuExtInLink(src, dest - (2 * m_nodes), link_entry.link,
                        routing_table_entry);
		}
    } else if (dest < 2*m_nodes) { // src >= 2 * m_nodes, switch->node
	  // printf("second condition this time\n");
        assert(dest >= m_nodes);
        NodeID node = dest - m_nodes;
        src_dest.first = src;
        src_dest.second = dest;
        link_entry = m_link_map[src_dest];
		// if(dest < m_nodes+ctrl_nodes) { // mem nodes
		if(node < ctrl_nodes) { // mem nodes
          net->makeExtOutLink(src - (2 * m_nodes), node, link_entry.link,
                         routing_table_entry);
		} else {
	  // printf("spu ext out this time\n");
          net->makeSpuExtOutLink(src - (2 * m_nodes), node, link_entry.link,
                         routing_table_entry);
		}
    } else {
        assert((src >= 2 * m_nodes) && (dest >= 2 * m_nodes));
        src_dest.first = src;
        src_dest.second = dest;
        link_entry = m_link_map[src_dest];
        net->makeInternalLink(src - (2 * m_nodes), dest - (2 * m_nodes),
                              link_entry.link,
                              routing_table_entry,
                              link_entry.src_outport_dirn,
                              link_entry.dst_inport_dirn);
    }
	// printf("END OF MAKE LINK\n");
}

// The following all-pairs shortest path algorithm is based on the
// discussion from Cormen et al., Chapter 26.1.
void
Topology::extend_shortest_path(Matrix &current_dist, Matrix &latencies,
    Matrix &inter_switches)
{
    bool change = true;
    int nodes = current_dist.size();

    while (change) {
        change = false;
        for (int i = 0; i < nodes; i++) {
            for (int j = 0; j < nodes; j++) {
                int minimum = current_dist[i][j];
                int previous_minimum = minimum;
                int intermediate_switch = -1;
                for (int k = 0; k < nodes; k++) {
                    minimum = min(minimum,
                        current_dist[i][k] + current_dist[k][j]);
                    if (previous_minimum != minimum) {
                        intermediate_switch = k;
                        inter_switches[i][j] =
                            inter_switches[i][k] +
                            inter_switches[k][j] + 1;
                    }
                    previous_minimum = minimum;
                }
                if (current_dist[i][j] != minimum) {
                    change = true;
                    current_dist[i][j] = minimum;
                    assert(intermediate_switch >= 0);
                    assert(intermediate_switch < latencies[i].size());
                    latencies[i][j] = latencies[i][intermediate_switch] +
                        latencies[intermediate_switch][j];
                }
            }
        }
    }
}

Matrix
Topology::shortest_path(const Matrix &weights, Matrix &latencies,
                        Matrix &inter_switches)
{
    Matrix dist = weights;
    extend_shortest_path(dist, latencies, inter_switches);
    return dist;
}

bool
Topology::link_is_shortest_path_to_node(SwitchID src, SwitchID next,
                                        SwitchID final, const Matrix &weights,
                                        const Matrix &dist)
{
    return weights[src][next] + dist[next][final] == dist[src][final];
}

NetDest
Topology::shortest_path_to_node(SwitchID src, SwitchID next,
                                const Matrix &weights, const Matrix &dist)
{
    NetDest result;
    int d = 0;
    int machines;
    int max_machines;

    machines = MachineType_NUM;
	// FIXME: hard-coded to debug
    // machines = MachineType_NUM+4;
    max_machines = MachineType_base_number(MachineType_NUM);

    for (int m = 0; m < machines; m++) {
        for (NodeID i = 0; i < MachineType_base_count((MachineType)m); i++) {
            // we use "d+max_machines" below since the "destination"
            // switches for the machines are numbered
            // [MachineType_base_number(MachineType_NUM)...
            //  2*MachineType_base_number(MachineType_NUM)-1] for the
            // component network
            if (link_is_shortest_path_to_node(src, next, d + max_machines,
                    weights, dist)) {
                MachineID mach = {(MachineType)m, i};
                result.add(mach);
            }
            d++;
        }
    }

    DPRINTF(RubyNetwork, "Returning shortest path\n"
            "(src-(2*max_machines)): %d, (next-(2*max_machines)): %d, "
            "src: %d, next: %d, result: %s\n",
            (src-(2*max_machines)), (next-(2*max_machines)),
            src, next, result);

    return result;
}
