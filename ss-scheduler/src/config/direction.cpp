#include "direction.h"
#include "model_parsing.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

using namespace SS_CONFIG;
using namespace std;

SwitchDir::SwitchDir() {

  //Adding the encoding for each direction
  add_encode(SwitchDir::N,3);
  add_encode(SwitchDir::NE,4);
  add_encode(SwitchDir::E,5);
  add_encode(SwitchDir::SE,6);
  add_encode(SwitchDir::S,7);
  add_encode(SwitchDir::SW,0);
  add_encode(SwitchDir::W,1);
  add_encode(SwitchDir::NW,2);
 
  //The following functions map the input directions and correspoding tuple
  //to an index 

  //TOP bottom left right
  add_encode(SwitchDir::IP0, std::make_tuple(true,false,false,false),encode(SwitchDir::NW)); 
  add_encode(SwitchDir::IP1, std::make_tuple(true,false,false,false),encode(SwitchDir::N));
  add_encode(SwitchDir::IP2, std::make_tuple(true,false,false,false),encode(SwitchDir::NE));
  
  //top bottom LEFT right
  add_encode(SwitchDir::IP0, std::make_tuple(false,false,true,false),encode(SwitchDir::SW)); 
  add_encode(SwitchDir::IP1, std::make_tuple(false,false,true,false),encode(SwitchDir::W));
  add_encode(SwitchDir::IP2, std::make_tuple(false,false,true,false),encode(SwitchDir::NW));
  
  //top bottoSbleft RIGHT
  add_encode(SwitchDir::IP0, std::make_tuple(false,false,false,true),encode(SwitchDir::SE)); 
  add_encode(SwitchDir::IP1, std::make_tuple(false,false,false,true),encode(SwitchDir::E));
  add_encode(SwitchDir::IP2, std::make_tuple(false,false,false,true),encode(SwitchDir::NE));
  
  //top BOTTOSbleft right
  add_encode(SwitchDir::IP0, std::make_tuple(false,true,false,false),encode(SwitchDir::SW)); 
  add_encode(SwitchDir::IP1, std::make_tuple(false,true,false,false),encode(SwitchDir::S));
  add_encode(SwitchDir::IP2, std::make_tuple(false,true,false,false),encode(SwitchDir::SE));
}


int SwitchDir::encode(DIR myDir) {
  return encode(myDir,false,false,false,false);
}

//preferred directions
void set_pref_dirs(bool& top,bool& bottom, bool& left, bool& right) {
  if(top && right)   {right=false;} //top
  if(top && left)    {left=false;}  //top
  if(bottom && left) {bottom=false;} //left
  if(bottom && right){bottom=false;} //right
}

//returns index of the direction and tuple using the io_enc map
int SwitchDir::encode(DIR myDir, bool top, bool bottom, bool left, bool right) {
  set_pref_dirs(top,bottom,left,right);
  std::pair<DIR,epos> pair = make_pair(myDir,epos(top,bottom,left,right));
  assert(io_enc.count(pair));
  return io_enc[pair];
}

//decode func with index and tuple returning the direction
SwitchDir::DIR SwitchDir::decode(int i, bool top, bool bottom, bool left, bool right) {
  set_pref_dirs(top,bottom,left,right);

  std::pair<int,epos> pair = make_pair(i,epos(top,bottom,left,right));
  assert(io_dec.count(pair));
  return io_dec[pair];
}

//position of output direction
//TODO: generalize for more than one output side
int SwitchDir::slot_for_dir(DIR myDir, bool top, bool bottom, bool left, bool right) {
  set_pref_dirs(top,bottom,left,right);

  if(isOutputDir(myDir)) {
    myDir=reverse(myDir,true);
  }
  return encode(myDir, top, bottom, left, right);
}

SwitchDir::DIR SwitchDir::dir_for_slot(int index, bool top, bool bottom, bool left, bool right) {
  set_pref_dirs(top,bottom,left,right);

  SwitchDir::DIR myDir = decode(index, top, bottom, left, right);
  if(isInputDir(myDir)) {
    myDir=reverse(myDir,true);
  }
  return myDir;
}

int SwitchDir::encode_fu_dir(DIR myDir) {
  switch(myDir) {
    case NE:  return  1;
    case SE:  return  2;
    case SW:  return  3;
    case NW:  return  4;
    case IM:  return  5;
    default:  assert(0 && "no encoding");
  }
  assert(0 && "not reachable");
}

SwitchDir::DIR SwitchDir::fu_dir_of(int i) {
  switch(i) {
    case 0:  return  END_DIR;
    case 1:  return  NE;
    case 2:  return  SE;
    case 3:  return  SW;
    case 4:  return  NW;
    case 5:  return  IM;
    default:  assert(0);
  }
  assert(0 && "not reachable");
}

//  switch(myDir) {
//    case N:   return  3;
//    case NE:  return  4;
//    case E:   return  5;
//    case SE:  return  6;
//    case S:   return  7;
//    case SW:  return  0;
//    case W:   return  1;
//    case NW:  return  2;
//    case OP0: return  pos_of(SW);
//    case OP1: return  pos_of(S);
//    case OP2: return  pos_of(SE);
//    default:  assert(0);
//  }



SwitchDir::DIR SwitchDir::toDir(string qs, bool outgoing) {
    if (false) return END_DIR;
    else if(ModelParsing::StartsWith(qs,"NW")) return outgoing ? NW  : reverse(NW);  
    else if(ModelParsing::StartsWith(qs,"NE")) return outgoing ? NE  : reverse(NE);
    else if(ModelParsing::StartsWith(qs,"SE")) return outgoing ? SE  : reverse(SE);
    else if(ModelParsing::StartsWith(qs,"SW")) return outgoing ? SW  : reverse(SW);
    else if (ModelParsing::StartsWith(qs,"N" )) return outgoing ? N   : reverse(N);
    else if(ModelParsing::StartsWith(qs,"E" )) return outgoing ? E   : reverse(E);
    else if(ModelParsing::StartsWith(qs,"S" )) return outgoing ? S   : reverse(S);
    else if(ModelParsing::StartsWith(qs,"W" )) return outgoing ? W   : reverse(W);
    else if(ModelParsing::StartsWith(qs,"P0")) return outgoing ? OP0 : IP0;  
    else if(ModelParsing::StartsWith(qs,"P1")) return outgoing ? OP1 : IP1;
    else if(ModelParsing::StartsWith(qs,"P2")) return outgoing ? OP2 : IP2;  
    else if(ModelParsing::StartsWith(qs,"IM")) return IM;
    return END_DIR;
}


//returns the reverse direction of DIR
SwitchDir::DIR SwitchDir::reverse(DIR myDir, bool reverseIO) {
  switch(myDir) {
    case N:   return S;
    case NE:  return SW;
    case E:   return W;
    case SE:  return NW;
    case S:   return N;
    case SW:  return NE;
    case W:   return E;
    case NW:  return SE;
    default: {
      if(reverseIO) {
        switch(myDir) {
          case IP0: return OP0;
          case IP1: return OP1;
          case IP2: return OP2;
          case OP0: return IP0;
          case OP1: return IP1;
          case OP2: return IP2;
          default: assert(0); return myDir;
        }
      } 
      //assert(isInputDir(myDir) && !isOutputDir(myDir));      
      return myDir; //don't reverse
    }
  }
}

const char*  SwitchDir::dirNameDBG(SwitchDir::DIR myDir, bool reverse) {
  if(isInputDir(myDir) || isOutputDir(myDir)) {
  switch(reverse ? SwitchDir::reverse(myDir) : myDir) {
    case SwitchDir::IP0:
        return "IP0";
        break;
    case SwitchDir::IP1:
        return "IP1";
        break;
    case SwitchDir::IP2:
        return "IP2";
        break;
    case SwitchDir::OP0:
        return "IP0";
        break;
    case SwitchDir::OP1:
        return "IP1";
        break;
    case SwitchDir::OP2:
        return "IP2";
        break;
    case SwitchDir::IM:
        return "IM";
        break;
    default:
        assert(0);
        break;
    }
  } else {
    return SwitchDir::dirName(myDir,reverse);
  }
}


const char* SwitchDir::dirName(SwitchDir::DIR myDir, bool reverse) {

  switch(reverse ? SwitchDir::reverse(myDir) : myDir) {
    case SwitchDir::N:
        return "N";
        break;
    case SwitchDir::NE:
        return "NE";
        break;
    case SwitchDir::E:
        return "E";
        break;
    case SwitchDir::SE:
        return "SE";
        break;
    case SwitchDir::S:
        return "S";
        break;
    case SwitchDir::SW:
        return "SW";
        break;
    case SwitchDir::W:
        return "W";
        break;
    case SwitchDir::NW:
        return "NW";
        break;
    case SwitchDir::IP0:
        return "P0";
        break;
    case SwitchDir::IP1:
        return "P1";
        break;
    case SwitchDir::IP2:
        return "P2";
        break;
    case SwitchDir::OP0:
        return "P0";
        break;
    case SwitchDir::OP1:
        return "P1";
        break;
    case SwitchDir::OP2:
        return "P2";
        break;
    case SwitchDir::IM:
        return "IM";
        break;
    case SwitchDir::END_DIR:
        return "xxx";
        break;
    }
    return "???";
}
