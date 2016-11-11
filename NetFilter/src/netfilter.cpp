#include <assert.h>
#include <stdlib.h>

#include "netfilter.h"
#include "tpktfilter.h"

extern "C"
{


    NetFilter* CreateNetFilter(NetFilterType enuType)
    {
        switch( enuType )
        {
        case NET_TPKT_FILTER:
            return new CTpktFilter();
        default:
            assert(0);
            break;
        };
        return NULL;
    }
}


