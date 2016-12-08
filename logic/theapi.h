#ifndef THEAPI_H
#define THEAPI_H

#include "utils/inst_once.h"
#include <vector>
#include <QString>

//okey, I want all aspects of the program be controllable from the LUA scripts.
//to do that easiest way is to put all functions in single place, and make some API from that


class TheAPI : public utility::ItCanBeOnlyOne<TheAPI>
{
public:

    TheAPI();
    virtual ~TheAPI();
};

#define THEAPI utility::globalInstance<TheAPI>()

#endif // THEAPI_H
