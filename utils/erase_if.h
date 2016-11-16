#ifndef ERASE_IF_H
#define ERASE_IF_H

namespace utility
{
    //can erase from std::map
    template< typename ContainerT, typename PredicateT >
    void erase_if( ContainerT& items, const PredicateT& predicate )
    {
        for( auto it = items.begin(); it != items.end(); )
        {
              if( predicate(*it) ) it = items.erase(it);
              else ++it;
        }
    }
}

#endif // ERASE_IF_H
