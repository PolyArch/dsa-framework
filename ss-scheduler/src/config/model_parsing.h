#ifndef __SS_MODEL_PARSING_H__
#define __SS_MODEL_PARSING_H__

#include <string>
#include <vector>
#include <iostream>


namespace SS_CONFIG {
  class ModelParsing {
  public:
      static bool StartsWith(const std::string& text,const std::string& token);
      static bool StartsWith(const std::string& text,const char* );
      static bool ReadPair(std::istream& ifs, std::string& param, std::string& value);
      static void split(const std::string &s, const char delim, std::vector<std::string> &elems);
        
      static void ltrim(std::string &s);
      static void rtrim(std::string &s);
      static void trim(std::string &s);
      static bool stricmp(const std::string& str1, const std::string& str2);
      static void trim_comments(std::string &s);
  };
  
  
}




#endif
