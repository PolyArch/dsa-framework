#include "model_parsing.h"

#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <string.h>

using namespace SS_CONFIG;
using namespace std;

bool ModelParsing::StartsWith(const std::string& text,const std::string& token) {
    if(text.length() < token.length()) return false;
    return (text.compare(0, token.length(), token) == 0);
}

bool ModelParsing::StartsWith(const std::string& text,const char* token) {
    if(text.length() < strlen(token)) return false;
    return (text.compare(0, strlen(token), token) == 0);
}

void ModelParsing::trim_comments(std::string& s) {
  s = s.substr(0, s.find("#"));
}


//This function reads line from an ifstream, and gets a param and value,
//seperated by a ":"
bool ModelParsing::ReadPair(istream& is, string& param, string& value)
{
    //char line[512];
    //is.getline(line,512);
    
    string line;
    getline(is, line);

    if(is.fail()) {
        param="";
        value="";
        return false;
    }

    trim_comments(line);

    std::stringstream ss(line);
    getline(ss, param, ':');
    getline(ss, value);
    return true;
}


// trim from start
void ModelParsing::ltrim(std::string &s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end
void ModelParsing::rtrim(std::string &s) {
        s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends
void ModelParsing::trim(std::string &s) {
        rtrim(s);
        ltrim(s);
}

bool ModelParsing::stricmp(const std::string& str1, const std::string& str2) {
    if (str1.size() != str2.size()) {
        return false;
    }
    for (std::string::const_iterator c1 = str1.begin(), c2 = str2.begin(); c1 != str1.end(); ++c1, ++c2) {
        if (tolower(*c1) != tolower(*c2)) {
            return false;
        }
    }
    return true;
}

void ModelParsing::split(const std::string &s, const char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}

