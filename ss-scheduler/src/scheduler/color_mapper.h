#ifndef __COLOR_MAPPER_H__
#define __COLOR_MAPPER_H__

#include <map>

class SSDfgNode;

//Maintain mapping between SSDfgNode's and colors
class ColorMapper {
public:
    int colorOf(SSDfgNode* item, bool reset =false);
    void setColor(int i, SSDfgNode* item) {colorMap[item]=i;}
    void clear() {colorMap.clear();}
private:
    std::map<SSDfgNode*,int> colorMap;
};
#endif
