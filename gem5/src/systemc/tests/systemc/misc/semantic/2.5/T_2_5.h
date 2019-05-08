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

  T_2_5.h -- 

  Original Author: Martin Janssen, Synopsys, Inc., 2002-02-15

 *****************************************************************************/

/*****************************************************************************

  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.

      Name, Affiliation, Date:
  Description of Modification:

 *****************************************************************************/

SC_MODULE( MYNAME )
{
    SC_HAS_PROCESS( MYNAME );

    sc_in_clk clk;

    const signal_vector& x;
    const signal_vector& y;
          signal_vector& z;

    sc_bigint<WIDTH>    a1;
    sc_bigint<WIDTH+2>  a2;
    sc_biguint<WIDTH>   b1;
    sc_biguint<WIDTH+2> b2;
    sc_bv<WIDTH>        c1;
    sc_bv<WIDTH+2>      c2;
    sc_lv<WIDTH>        d1;
    sc_lv<WIDTH+2>      d2;

    MYNAME( sc_module_name NAME,
            sc_clock& CLK,
            const signal_vector& X,
            const signal_vector& Y,
            signal_vector& Z )
        : 
          x(X), y(Y), z(Z)
    {
        clk(CLK);
		SC_CTHREAD( entry, clk.pos() );
    }
    void entry();
};
