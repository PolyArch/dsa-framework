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

  interface.h -- 

  Original Author: Martin Janssen, Synopsys, Inc., 2002-02-15

 *****************************************************************************/

/*****************************************************************************

  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.

      Name, Affiliation, Date:
  Description of Modification:

 *****************************************************************************/

/* Common interface file for test cases 
   Author: PRP 
   */

SC_MODULE( t )
{
    SC_HAS_PROCESS( t );

    sc_in_clk clk;

        const sc_signal<bool>& reset_sig;

	const sc_signal<int>& i1;
	const sc_signal<int>& i2;
	const sc_signal<int>& i3;
	const sc_signal<int>& i4;
	const sc_signal<int>& i5;

	sc_signal<int>& o1;
	sc_signal<int>& o2;
	sc_signal<int>& o3;
	sc_signal<int>& o4;
	sc_signal<int>& o5;

	// Constructor
	t (	
        sc_module_name NAME, 
	sc_clock& CLK,

        const sc_signal<bool>& RESET_SIG,

	const sc_signal<int>& I1,
	const sc_signal<int>& I2,
	const sc_signal<int>& I3,
	const sc_signal<int>& I4,
	const sc_signal<int>& I5,

	sc_signal<int>& O1,
	sc_signal<int>& O2,
	sc_signal<int>& O3,
	sc_signal<int>& O4,
	sc_signal<int>& O5)
	  : reset_sig(RESET_SIG), i1(I1),  i2(I2),  i3(I3),  i4(I4),  
	    i5(I5), o1(O1),  o2(O2),  o3(O3),  o4(O4),  o5(O5) 
        {
	  clk(CLK);
          SC_CTHREAD( entry, clk.pos() );
	  reset_signal_is(reset_sig,true);
	}

  void entry();
};
