#include "color_mapper.h"
#include <math.h>
#include <stdlib.h>
#include "ssdfg.h"

int getrgb(int r, int g, int b) {
  return ((r  << 16)|(g << 8)| b);
}

/*Luminance (standard, objective): (0.2126*R) + (0.7152*G) + (0.0722*B)
Luminance (perceived option 1): (0.299*R + 0.587*G + 0.114*B)
Luminance (perceived option 2, slower to calculate): sqrt( 0.241*R^2 + 0.691*G^2 + 0.068*B^2 )
*/
int ColorMapper::colorOf(SSDfgNode* item, bool reset) {
    if(item->num_inc()==1 && item->ops()[0].edges.size()==1) {
      return colorOf(item->ops()[0].get_first_edge()->def());
    }
    if(colorMap.count(item)==0 || reset) {
        int x=0,y=0,z=0;
        float lum=0;
        while(lum<0.22f || lum >0.95f) {  //prevent dark colors
            //  || abs(x-y) + abs(y-z) + abs(z-x) < 100
            x = rand()%256;
            y = rand()%256;
            z = rand()%256;
            lum = sqrt(x*x*0.241f+y*y*0.691f+z*z*0.068f)/255.0f;
            //lum = (x*0.299f+y*0.587f+z*0.114f)/255.0f;
        }
        colorMap[item] = getrgb(x,y,z);
    }
    return colorMap[item];
}

