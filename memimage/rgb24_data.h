#ifndef RGB24_DATA_H
#define RGB24_DATA_H
#include <valarray>
#include <stdint.h>
namespace imaging
{


    struct dims_t
    {
        uint32_t width;
        uint32_t height;
        dims_t(uint32_t w, uint32_t h): width(w), height(h){}
    };

    struct rgb24_data : public std::valarray<uint8_t>
    {

        dims_t dims;
        rgb24_data();
        void set_data(uint32_t w, uint32_t h, const uint8_t* mem, size_t size); //expects R-G-B 24 bits total
    };
}
#endif // RGB24_DATA_H
