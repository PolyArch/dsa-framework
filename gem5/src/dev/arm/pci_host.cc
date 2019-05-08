/*
 * Copyright (c) 2015 ARM Limited
 * All rights reserved
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
 *
 * Authors: Andreas Sandberg
 */

#include "dev/arm/pci_host.hh"

#include "params/GenericArmPciHost.hh"

GenericArmPciHost::GenericArmPciHost(const GenericArmPciHostParams *p)
    : GenericPciHost(p),
      intPolicy(p->int_policy), intBase(p->int_base),
      intCount(p->int_count)
{
}


uint32_t
GenericArmPciHost::mapPciInterrupt(const PciBusAddr &addr, PciIntPin pin) const
{
    fatal_if(pin == PciIntPin::NO_INT,
             "%02x:%02x.%i: Interrupt from a device without interrupts\n",
             addr.bus, addr.dev, addr.func);

    switch (intPolicy) {
      case Enums::ARM_PCI_INT_STATIC:
        return GenericPciHost::mapPciInterrupt(addr, pin);

      case Enums::ARM_PCI_INT_DEV:
        return intBase + (addr.dev % intCount);

      case Enums::ARM_PCI_INT_PIN:
        return intBase + ((static_cast<uint8_t>(pin) - 1) % intCount);

      default:
        fatal("Unsupported PCI interrupt routing policy.");
    }
}


GenericArmPciHost *
GenericArmPciHostParams::create()
{
    return new GenericArmPciHost(this);
}
