#include "rgb24_data.h"
#include <stdexcept>

using namespace imaging;

rgb24_data::rgb24_data():
    dims(0,0)
{

}

void rgb24_data::set_data(uint32_t w, uint32_t h, const uint8_t *mem, size_t size)
{
    if (size % 3 || w * h != size / 3)
        throw std::runtime_error("Expecting 24 bits per pixel.");

    dims.width  = w;
    dims.height = h;
    resize(size);
    size_t i = 0;
    for (auto& a : *this)
        a = *(mem + i++);
}
