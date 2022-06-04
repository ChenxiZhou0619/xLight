#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#include <iostream>

int main() {
    std::string filename = "/mnt/renderer/xLight/data/image/studio_country_hall_1k.hdr";
    int _x, _y ,_channels;
    unsigned char *data = stbi_load(filename.c_str(), &_x, &_y, &_channels, 3);
    std::cout << "width = " << _x << std::endl 
              << "height = " << _y << std::endl
              << "channels = " << _channels << std::endl;
    
}