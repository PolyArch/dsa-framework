/*****************************************************************************

  Licensed to Accellera Systems Initiative Inc. (Accellera) under one or
  more contributor license agreements.  See the NOTICE file distributed
  with this work for additional information regarding copyright ownership.
  Accellera licenses this file to you under the Apache License, Version 2.0
  (the "License"); you may not use this file except in compliance with the
  License.  You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
  implied.  See the License for the specific language governing
  permissions and limitations under the License.

 *****************************************************************************/

#ifndef __TLM_ANALYSIS_FIFO_H__
#define __TLM_ANALYSIS_FIFO_H__

#include "tlm_core/tlm_1/tlm_req_rsp/tlm_channels/tlm_fifo/tlm_fifo.h"
#include "tlm_core/tlm_1/tlm_analysis/tlm_analysis_if.h"
#include "tlm_core/tlm_1/tlm_analysis/tlm_analysis_triple.h"

namespace tlm {

template< typename T >
class tlm_analysis_fifo :
  public tlm_fifo< T > ,
  public virtual tlm_analysis_if< T > ,
  public virtual tlm_analysis_if< tlm_analysis_triple< T > > {

 public:

 // analysis fifo is an unbounded tlm_fifo

  tlm_analysis_fifo( const char *nm ) : tlm_fifo<T>( nm , -16 ) {}
  tlm_analysis_fifo() : tlm_fifo<T>( -16 ) {}

  void write( const tlm_analysis_triple<T> &t ) {
    nb_put( t );
  }

  void write( const T &t ) {
    nb_put( t );
  }

};

} // namespace tlm

#endif
