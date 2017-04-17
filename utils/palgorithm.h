#ifndef PALGORITHM_H
#define PALGORITHM_H

#include <algorithm>

#ifdef USING_OPENMP
#include <parallel/algorithm>

#ifndef ALG_NS
#define ALG_NS __gnu_parallel
#endif

#endif

//fallback to std
#ifndef ALG_NS
#define ALG_NS std
#endif



#endif // PALGORITHM_H
