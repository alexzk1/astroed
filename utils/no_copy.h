#ifndef NO_COPY_H
#define NO_COPY_H

namespace utility
{
    struct NoCopy
    {
    public:
        NoCopy() = default;
        NoCopy(const NoCopy &) = delete;
    };

    struct NoAssign
    {
        NoAssign() = default;
        NoAssign &operator=(const NoAssign &) = delete;
    };

    struct NoCopyAssign : NoCopy, NoAssign
    {
         NoCopyAssign() = default;
    };
    typedef NoCopyAssign NoAssignCopy;

}

#endif // NO_COPY_H
