#ifndef __SS_DIRECTION_H__
#define __SS_DIRECTION_H__

#include <string>
#include <map>
#include <unordered_map>
#include <utility>

namespace SS_CONFIG {

  typedef std::tuple<bool,bool,bool,bool> epos; 

  class SwitchDir {
  public:
     enum DIR { IP0, IP1, IP2, OP0, OP1, OP2, N, NE, E, SE, S, SW, W, NW, IM, END_DIR };

     static bool isInputDir(DIR d)  {return d==IP0 || d==IP1 || d==IP2;}
     static bool isOutputDir(DIR d) {return d==OP0 || d==OP1 || d==OP2;}

     static DIR reverse(DIR myDir, bool reverseIO=false);
     static DIR toDir(std::string qs, bool outgoing);
     static const char* dirName(SwitchDir::DIR dir, bool reverse=false);
     static const char* dirNameDBG(SwitchDir::DIR dir, bool reverse=false);

     std::map<std::pair<DIR,epos>,int> io_enc;
     std::map<std::pair<int,epos>,DIR> io_dec;

     void add_encode(DIR dir, epos e, int index) {
       io_enc[std::make_pair(dir,e)]=index;
       io_dec[std::make_pair(index,e)]=dir;
     }

     //map func to map each direction and its index with
     //all possible tuples
     void add_encode(DIR dir, int index) {
       for(int i = 0; i <= 1; ++i) {
         for(int j = 0; j <= 1; ++j) {
           for(int k = 0; k <= 1; ++k) {
             for(int l = 0; l <= 1; ++l) {
               add_encode(dir,std::make_tuple(i,j,k,l),index);
             }
           }
         }
       }
     }
     
     SwitchDir();


     int encode(DIR i);
     DIR decode(int i, bool top, bool bottom, bool left, bool right);
     DIR dir_for_slot(int i, bool top, bool bottom, bool left, bool right);
     int encode(DIR i, bool top, bool bottom, bool left, bool right);
     int slot_for_dir(DIR i, bool top, bool bottom, bool left, bool right);

     int encode_fu_dir(DIR myDir);
     DIR fu_dir_of(int i);

  };
}




#endif
