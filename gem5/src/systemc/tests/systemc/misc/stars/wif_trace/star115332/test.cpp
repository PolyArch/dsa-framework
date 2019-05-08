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

/*****************************************************************************

  test.cpp -- 

  Original Author: Martin Janssen, Synopsys, Inc., 2002-02-15

 *****************************************************************************/

/*****************************************************************************

  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.

      Name, Affiliation, Date:
  Description of Modification:

 *****************************************************************************/

#include "flop.h"
#include "systemc.h"
  

int sc_main(int argc, char *argv[]) //(int ac, char** av)

{

  sc_signal<sc_uint<1> > int1 ;
  sc_signal<sc_uint<1> > int2 ;


  sc_clock clk("clk", 20, SC_NS, 0.5);



// instanciate Processes

  flop  FLOP("flip_flop");
  FLOP.clk(clk) ;
  FLOP.in(int1) ;
  FLOP.out(int2) ;

  sc_trace_file * tf = sc_create_wif_trace_file("test");
  sc_trace( tf, clk, "clk");
  sc_trace( tf, int1, "int1");
  sc_trace( tf, int2, "int2");

  /*
  sc_trace_file * tf2 = sc_create_vcd_trace_file("dump_vcd");
  sc_trace( tf2, clk, "clk");
  sc_trace( tf2, int1, "int1");
  sc_trace( tf2, int2, "int2");
  */

  sc_start(1000, SC_NS);
  sc_close_wif_trace_file( tf );
  return 0;
}
