#ifndef CONT_UTILS_H
#define CONT_UTILS_H

#include <vector>
#include <set>
#ifdef USING_OPENMP
#include <parallel/algorithm>
#else
#include <algorithm>
#endif
#include <type_traits>
#include <stdint.h>
#include <iterator>

//containers utils

namespace utility
{
    template<typename T, typename L = std::vector<T>>
    size_t vector_unique_ordered(L& vec)
    {
        std::set<T> seen;

        auto newEnd = std::remove_if(vec.begin(), vec.end(), [&seen](const T& value)
        {
            if (seen.count(value))
                return true;

            seen.insert(value);
            return false;
        });

        vec.erase(newEnd, vec.end());

        return vec.size();
    }

    //swaps pointed values (so huge template, just to ensure it will swap values and not pointers itself and fail at compile time)
    template <typename PtrT,
              typename Pointed = typename std::enable_if<std::is_pointer<PtrT>::value,
                                                         typename std::remove_pointer<PtrT>::type>::type>
    void swapPointed(PtrT v1, PtrT v2)
    {
        std::swap<Pointed>(*v1, *v2);
    }

    //pixel iterator, which keeps counting in term of "pixel", which is tripplet of 3 colors
    template <typename ColorT, typename ColorType = ColorT*>
    class PixelIterator : public std::iterator<std::random_access_iterator_tag, ColorType>
    {
    private:
        ColorType buffer;
        size_t tripplets_amount;
        size_t current;

        PixelIterator(ColorT* buffer, size_t tripplets_amount): //expects size of buffer to be 3 * tripplets_amount of ColotT elements exactly
            buffer(buffer),
            tripplets_amount(tripplets_amount),
            current(0)
        {
        }

    public:
        PixelIterator():
            buffer(nullptr),
            tripplets_amount(0),
            current(0)
        {
        }
        static PixelIterator start(ColorT* buffer, size_t tripplets_amount)
        {
            return PixelIterator(buffer, tripplets_amount);
        }

        static PixelIterator end(ColorT* buffer, size_t tripplets_amount)
        {
            PixelIterator r(buffer, tripplets_amount);
            r.current = tripplets_amount;
            return r;
        }

        PixelIterator(const PixelIterator& ) = default;
        PixelIterator& operator = (const PixelIterator&) = default;
        ~PixelIterator() = default;

        ColorType operator*() const
        {

            if (current < tripplets_amount)
                return buffer + current * 3;
            return nullptr;
        }


        ColorType operator[](size_t n) const
        {
            if (current + n < tripplets_amount)
                return buffer + (current + n) * 3;
            return nullptr;
        }

        PixelIterator& operator++()
        {
            // actual increment takes place here
            ++current;
            return *this;
        }

        PixelIterator operator++(int)
        {
            PixelIterator tmp(*this); // copy
            operator++(); // pre-increment
            return tmp;   // return old value
        }

        PixelIterator& operator--()
        {
            if (current)
                --current;
            return *this;
        }

        PixelIterator operator--(int)
        {
            PixelIterator tmp(*this);
            operator--();
            return tmp;
        }


        PixelIterator& operator+=(size_t val)
        {
            current += val;
            return *this;
        }

        PixelIterator& operator-=(size_t val)
        {
            if (current - val >0)
                current -= val;
            else
                current = 0;
            return *this;
        }

        PixelIterator operator+(size_t val) const
        {
            PixelIterator tmp(*this);
            tmp.current = current + val;
            return tmp;
        }

        PixelIterator operator-(size_t val) const
        {
            PixelIterator tmp(*this);
            tmp.current = current - val;
            return tmp;
        }

        size_t operator+(const PixelIterator& val) const
        {
            return current + val.current;
        }

        size_t operator-(const PixelIterator& val) const
        {
            return current - val.current;
        }


        bool operator < (const PixelIterator& c) const
        {
            return current < c.current;
        }

        bool operator <= (const PixelIterator& c) const
        {
            return current <= c.current;
        }

        bool operator > (const PixelIterator& c) const
        {
            return current > c.current;
        }

        bool operator >= (const PixelIterator& c) const
        {
            return current >= c.current;
        }

        bool operator == (const PixelIterator& c) const
        {
            return buffer == c.buffer && tripplets_amount == c.tripplets_amount && current == c.current;
        }

        bool operator !=(const PixelIterator& c) const
        {
            return !(*this == c);
        }
    };

    //expecting, that buffer is 3 * pixels_amount, allows conversion in both directions (invariant)
    template <typename ColorT = uint8_t> //allows to have more than 8 bytes per color
    inline void bgr_rgb(ColorT *buffer, size_t pixels_amount)
    {
        using iter_t = PixelIterator<ColorT>;
        auto  istart = iter_t::start(buffer, pixels_amount);
        auto  iend   = iter_t::end  (buffer, pixels_amount);
#ifdef USING_OPENMP
        __gnu_parallel::
#else
        std::
#endif
                for_each(istart, iend, [](auto p)
        {
            utility::swapPointed(p + 0, p + 2);
        });
    }
}

#endif // CONT_UTILS_H
