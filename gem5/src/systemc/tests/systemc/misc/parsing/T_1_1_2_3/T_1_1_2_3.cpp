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

  T_1_1_2_3.cpp -- 

  Original Author: Martin Janssen, Synopsys, Inc., 2002-02-15

 *****************************************************************************/

/*****************************************************************************

  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.

      Name, Affiliation, Date:
  Description of Modification:

 *****************************************************************************/

#include "systemc.h"

SC_MODULE( foo )
{
    SC_HAS_PROCESS( foo );

    sc_in_clk clk;

    const sc_signal<bool> &input;
    sc_signal<bool> &output;

    int state;

    foo( sc_module_name NAME,
	 sc_clock& CLK,
	 const sc_signal<bool>& INPUT,
	 sc_signal<bool>& OUTPUT )
        : input(INPUT), output(OUTPUT)
    {
        clk(CLK);
		SC_CTHREAD( entry, clk.pos() );
    }
    void entry();
};

int sc_main(int argc, char* argv[] )
{
  return 0;
}
