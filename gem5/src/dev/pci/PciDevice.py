# Copyright (c) 2013 ARM Limited
#  All rights reserved
#
# The license below extends only to copyright in the software and shall
# not be construed as granting a license to any other intellectual
# property including but not limited to intellectual property relating
# to a hardware implementation of the functionality of the software
# licensed hereunder.  You may use the software subject to the license
# terms below provided that you ensure that this notice is replicated
# unmodified and in its entirety in all distributions of the software,
# modified or unmodified, in source code or in binary form.
#
# Copyright (c) 2005-2007 The Regents of The University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met: redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer;
# redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution;
# neither the name of the copyright holders nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Authors: Nathan Binkert

from m5.SimObject import SimObject
from m5.params import *
from m5.proxy import *
from Device import DmaDevice
from PciHost import PciHost

class PciDevice(DmaDevice):
    type = 'PciDevice'
    cxx_class = 'PciDevice'
    cxx_header = "dev/pci/device.hh"
    abstract = True

    host = Param.PciHost(Parent.any, "PCI host")
    pci_bus = Param.Int("PCI bus")
    pci_dev = Param.Int("PCI device number")
    pci_func = Param.Int("PCI function code")

    pio_latency = Param.Latency('30ns', "Programmed IO latency")
    config_latency = Param.Latency('20ns', "Config read or write latency")

    VendorID = Param.UInt16("Vendor ID")
    DeviceID = Param.UInt16("Device ID")
    Command = Param.UInt16(0, "Command")
    Status = Param.UInt16(0, "Status")
    Revision = Param.UInt8(0, "Device")
    ProgIF = Param.UInt8(0, "Programming Interface")
    SubClassCode = Param.UInt8(0, "Sub-Class Code")
    ClassCode = Param.UInt8(0, "Class Code")
    CacheLineSize = Param.UInt8(0, "System Cacheline Size")
    LatencyTimer = Param.UInt8(0, "PCI Latency Timer")
    HeaderType = Param.UInt8(0, "PCI Header Type")
    BIST = Param.UInt8(0, "Built In Self Test")

    BAR0 = Param.UInt32(0x00, "Base Address Register 0")
    BAR1 = Param.UInt32(0x00, "Base Address Register 1")
    BAR2 = Param.UInt32(0x00, "Base Address Register 2")
    BAR3 = Param.UInt32(0x00, "Base Address Register 3")
    BAR4 = Param.UInt32(0x00, "Base Address Register 4")
    BAR5 = Param.UInt32(0x00, "Base Address Register 5")
    BAR0Size = Param.MemorySize32('0B', "Base Address Register 0 Size")
    BAR1Size = Param.MemorySize32('0B', "Base Address Register 1 Size")
    BAR2Size = Param.MemorySize32('0B', "Base Address Register 2 Size")
    BAR3Size = Param.MemorySize32('0B', "Base Address Register 3 Size")
    BAR4Size = Param.MemorySize32('0B', "Base Address Register 4 Size")
    BAR5Size = Param.MemorySize32('0B', "Base Address Register 5 Size")
    BAR0LegacyIO = Param.Bool(False, "Whether BAR0 is hardwired legacy IO")
    BAR1LegacyIO = Param.Bool(False, "Whether BAR1 is hardwired legacy IO")
    BAR2LegacyIO = Param.Bool(False, "Whether BAR2 is hardwired legacy IO")
    BAR3LegacyIO = Param.Bool(False, "Whether BAR3 is hardwired legacy IO")
    BAR4LegacyIO = Param.Bool(False, "Whether BAR4 is hardwired legacy IO")
    BAR5LegacyIO = Param.Bool(False, "Whether BAR5 is hardwired legacy IO")
    LegacyIOBase = Param.Addr(0x0, "Base Address for Legacy IO")

    CardbusCIS = Param.UInt32(0x00, "Cardbus Card Information Structure")
    SubsystemID = Param.UInt16(0x00, "Subsystem ID")
    SubsystemVendorID = Param.UInt16(0x00, "Subsystem Vendor ID")
    ExpansionROM = Param.UInt32(0x00, "Expansion ROM Base Address")
    CapabilityPtr = Param.UInt8(0x00, "Capability List Pointer offset")
    InterruptLine = Param.UInt8(0x00, "Interrupt Line")
    InterruptPin = Param.UInt8(0x00, "Interrupt Pin")
    MaximumLatency = Param.UInt8(0x00, "Maximum Latency")
    MinimumGrant = Param.UInt8(0x00, "Minimum Grant")

    # Capabilities List structures for PCIe devices
    # PMCAP - PCI Power Management Capability
    PMCAPBaseOffset = \
        Param.UInt8(0x00, "Base offset of PMCAP in PCI Config space")
    PMCAPNextCapability = \
        Param.UInt8(0x00, "Pointer to next capability block")
    PMCAPCapId = \
        Param.UInt8(0x00, "Specifies this is the Power Management capability")
    PMCAPCapabilities = \
        Param.UInt16(0x0000, "PCI Power Management Capabilities Register")
    PMCAPCtrlStatus = \
        Param.UInt16(0x0000, "PCI Power Management Control and Status")

    # MSICAP - Message Signaled Interrupt Capability
    MSICAPBaseOffset = \
        Param.UInt8(0x00, "Base offset of MSICAP in PCI Config space")
    MSICAPNextCapability = \
        Param.UInt8(0x00, "Pointer to next capability block")
    MSICAPCapId = Param.UInt8(0x00, "Specifies this is the MSI Capability")
    MSICAPMsgCtrl = Param.UInt16(0x0000, "MSI Message Control")
    MSICAPMsgAddr = Param.UInt32(0x00000000, "MSI Message Address")
    MSICAPMsgUpperAddr = Param.UInt32(0x00000000, "MSI Message Upper Address")
    MSICAPMsgData = Param.UInt16(0x0000, "MSI Message Data")
    MSICAPMaskBits = Param.UInt32(0x00000000, "MSI Interrupt Mask Bits")
    MSICAPPendingBits = Param.UInt32(0x00000000, "MSI Pending Bits")

    # MSIXCAP - MSI-X Capability
    MSIXCAPBaseOffset = \
        Param.UInt8(0x00, "Base offset of MSIXCAP in PCI Config space")
    MSIXCAPNextCapability = \
        Param.UInt8(0x00, "Pointer to next capability block")
    MSIXCAPCapId = Param.UInt8(0x00, "Specifices this the MSI-X Capability")
    MSIXMsgCtrl = Param.UInt16(0x0000, "MSI-X Message Control")
    MSIXTableOffset = \
        Param.UInt32(0x00000000, "MSI-X Table Offset and Table BIR")
    MSIXPbaOffset = Param.UInt32(0x00000000, "MSI-X PBA Offset and PBA BIR")

    # PXCAP - PCI Express Capability
    PXCAPBaseOffset = \
        Param.UInt8(0x00, "Base offset of PXCAP in PCI Config space")
    PXCAPNextCapability = Param.UInt8(0x00, "Pointer to next capability block")
    PXCAPCapId = Param.UInt8(0x00, "Specifies this is the PCIe Capability")
    PXCAPCapabilities = Param.UInt16(0x0000, "PCIe Capabilities")
    PXCAPDevCapabilities = Param.UInt32(0x00000000, "PCIe Device Capabilities")
    PXCAPDevCtrl = Param.UInt16(0x0000, "PCIe Device Control")
    PXCAPDevStatus = Param.UInt16(0x0000, "PCIe Device Status")
    PXCAPLinkCap = Param.UInt32(0x00000000, "PCIe Link Capabilities")
    PXCAPLinkCtrl = Param.UInt16(0x0000, "PCIe Link Control")
    PXCAPLinkStatus = Param.UInt16(0x0000, "PCIe Link Status")
    PXCAPDevCap2 = Param.UInt32(0x00000000, "PCIe Device Capabilities 2")
    PXCAPDevCtrl2 = Param.UInt32(0x00000000, "PCIe Device Control 2")
