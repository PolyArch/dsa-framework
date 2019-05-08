/*
 * Copyright (c) 2017 ARM Limited
 * All rights reserved.
 *
 * The license below extends only to copyright in the software and shall
 * not be construed as granting a license to any other intellectual
 * property including but not limited to intellectual property relating
 * to a hardware implementation of the functionality of the software
 * licensed hereunder.  You may use the software subject to the license
 * terms below provided that you ensure that this notice is replicated
 * unmodified and in its entirety in all distributions of the software,
 * modified or unmodified, in source code or in binary form.
 *
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

#include "mem/ruby/network/Network.hh"

#include "base/logging.hh"
#include "mem/ruby/common/MachineID.hh"
#include "mem/ruby/network/BasicLink.hh"
#include "mem/ruby/system/RubySystem.hh"
// #include "mem/ruby/system/Sequencer.hh"
#include "cpu/minor/cpu.hh"

uint32_t Network::m_virtual_networks;
uint32_t Network::m_control_msg_size;
uint32_t Network::m_data_msg_size;

Network::Network(const Params *p)
    : ClockedObject(p)
{
    m_virtual_networks = p->number_of_virtual_networks;
    m_control_msg_size = p->control_msg_size;

    // Total nodes/controllers in network
    m_nodes = MachineType_base_number(MachineType_NUM);
    assert(m_nodes != 0);
    assert(m_virtual_networks != 0);

    m_topology_ptr = new Topology(p->routers.size(), p->ext_links,
                                  p->spu_ext_links, p->int_links);

	// int mem_ctrl_size = p->ext_links.size();
	// printf("NUMBER OF MEMORY CONTROLLERS: %d\n",mem_ctrl_size);
    // Allocate to and from queues
    // Queues that are getting messages from protocol
    m_toNetQueues.resize(m_nodes);

    // Queues that are feeding the protocol
    m_fromNetQueues.resize(m_nodes);

    m_ordered.resize(m_virtual_networks);
    m_vnet_type_names.resize(m_virtual_networks);

    for (int i = 0; i < m_virtual_networks; i++) {
        m_ordered[i] = false;
    }

    params()->ruby_system->registerNetwork(this);

    // Initialize the controller's network pointers
    for (std::vector<BasicExtLink*>::const_iterator i = p->ext_links.begin();
         i != p->ext_links.end(); ++i) {
        BasicExtLink *ext_link = (*i);
        AbstractController *abs_cntrl = ext_link->params()->ext_node;
        abs_cntrl->initNetworkPtr(this);
        const AddrRangeList &ranges = abs_cntrl->getAddrRanges();
        if (!ranges.empty()) {
            MachineID mid = abs_cntrl->getMachineID();
            AddrMapNode addr_map_node = {
                .id = mid.getNum(),
                .ranges = ranges
            };
            addrMap.emplace(mid.getType(), addr_map_node);
        }
    }

    // Register a callback function for combining the statistics
    Stats::registerDumpCallback(new StatsCallback(this));

    for (auto &it : dynamic_cast<Network *>(this)->params()->ext_links) {
        it->params()->ext_node->initNetQueues();
    }

	// int n_spu_cores = p->spu_ext_links.size();
	// printf("NUMBER OF SPU CORES: %d\n",n_spu_cores);

	for (std::vector<SpuExtLink*>::const_iterator i = p->spu_ext_links.begin();
         i != p->spu_ext_links.end(); ++i) {
        SpuExtLink *spu_ext_link = (*i);
	    MinorCPU *accel = spu_ext_link->params()->spu_ext_node;
        accel->initNetworkPtr(this);
    }

	for (auto &it : dynamic_cast<Network *>(this)->params()->spu_ext_links) {
        it->params()->spu_ext_node->initNetQueues();
    }
	

}

Network::~Network()
{
    for (int node = 0; node < m_nodes; node++) {

        // Delete the Message Buffers
        for (auto& it : m_toNetQueues[node]) {
            delete it;
        }

        for (auto& it : m_fromNetQueues[node]) {
            delete it;
        }
    }
    delete m_topology_ptr;
}

void
Network::init()
{
    m_data_msg_size = RubySystem::getBlockSizeBytes() + m_control_msg_size;
}

uint32_t
Network::MessageSizeType_to_int(MessageSizeType size_type)
{
    switch(size_type) {
      case MessageSizeType_Control:
      case MessageSizeType_Request_Control:
      case MessageSizeType_Reissue_Control:
      case MessageSizeType_Response_Control:
      case MessageSizeType_Writeback_Control:
      case MessageSizeType_Broadcast_Control:
      case MessageSizeType_Multicast_Control:
      case MessageSizeType_Forwarded_Control:
      case MessageSizeType_Invalidate_Control:
      case MessageSizeType_Unblock_Control:
      case MessageSizeType_Persistent_Control:
      case MessageSizeType_Completion_Control:
        return m_control_msg_size;
      case MessageSizeType_Data:
      case MessageSizeType_Response_Data:
      case MessageSizeType_ResponseLocal_Data:
      case MessageSizeType_ResponseL2hit_Data:
      case MessageSizeType_Writeback_Data:
        return m_data_msg_size;
      case MessageSizeType_SpuIndirect_Data:
            return m_data_msg_size; // this is currently cache block size (I can vary it) -- but not sure how network responds to it
      default:
        panic("Invalid range for type MessageSizeType");
        break;
    }
}

void
Network::checkNetworkAllocation(NodeID id, bool ordered,
                                        int network_num,
                                        std::string vnet_type)
{
    // printf("id: %d, m_nodes: %d\n",id,m_nodes);
    fatal_if(id >= m_nodes, "Node ID is out of range");
    fatal_if(network_num >= m_virtual_networks, "Network id is out of range");

    if (ordered) {
        m_ordered[network_num] = true;
    }

    m_vnet_type_names[network_num] = vnet_type;
}


void
Network::setToNetQueue(NodeID id, bool ordered, int network_num,
                                 std::string vnet_type, MessageBuffer *b)
{
  // printf("id check in settonetqueue: %d\n",id);
    checkNetworkAllocation(id, ordered, network_num, vnet_type);
    while (m_toNetQueues[id].size() <= network_num) {
        m_toNetQueues[id].push_back(nullptr);
    }
    m_toNetQueues[id][network_num] = b;
}

void
Network::setFromNetQueue(NodeID id, bool ordered, int network_num,
                                   std::string vnet_type, MessageBuffer *b)
{
  // printf("CHECK NETWORK NUM AT NETWORK.CC:setNormQueue %d, with node id: %d\n", network_num, id);
  // it's 3 for some and 4 for some
    checkNetworkAllocation(id, ordered, network_num, vnet_type);
    while (m_fromNetQueues[id].size() <= network_num) {
        m_fromNetQueues[id].push_back(nullptr);
    }
    m_fromNetQueues[id][network_num] = b;
}

NodeID
Network::addressToNodeID(Addr addr, MachineType mtype)
{
    // Look through the address maps for entries with matching machine
    // type to get the responsible node for this address.
    const auto &matching_ranges = addrMap.equal_range(mtype);
    for (auto it = matching_ranges.first; it != matching_ranges.second; it++) {
        AddrMapNode &node = it->second;
        auto &ranges = node.ranges;
        for (AddrRange &range: ranges) {
            if (range.contains(addr)) {
                return node.id;
            }
        }
    }
    return MachineType_base_count(mtype);
}
