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

  stimulus.cpp -- 

  Original Author: Rocco Jonack, Synopsys, Inc., 1999-07-22

 *****************************************************************************/

/*****************************************************************************

  MODIFICATION LOG - modifiers, enter your name, affiliation, date and
  changes you are making here.

      Name, Affiliation, Date:
  Description of Modification:

 *****************************************************************************/


#include "stimulus.h"

void stimulus::entry() {

int i;

  // sending some reset values
  reset.write(true);
  stim1.write(0);
  stim2.write(0);
  stim3.write(0);
  wait();
  reset.write(false);
  wait();
  for  (i=0; i<= 15; i++) {
    stim1.write(i);
    stim2.write(i);
    stim3.write(i);
    input_valid.write(true);
    cout << "Stimuli: stim1= " << i << " stim2= " << i << " stim3= " << i 
	 << " at " << sc_time_stamp() << endl; 
    wait();
    input_valid.write(false);
    wait(15);
  }
 
  sc_stop();
}

// EOF
