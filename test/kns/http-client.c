#include "../../libs/kns/http-client.c"

const struct KEndPointArgsIterator * KNSManagerMakeKEndPointArgsIterator
    ( const KNSManager * self, const String * hostname, uint32_t port )
{
    static struct KEndPointArgsIterator i;
    KEndPointArgsIteratorMake ( & i, self, hostname, port );
    return & i;
}

bool KEndPointArgsIterator_Next ( KEndPointArgsIterator * self,
        const String ** hostname, uint16_t * port,
        bool * proxy_default_port, bool * proxy_ep )
{
    return KEndPointArgsIteratorNext
        ( self, hostname, port, proxy_default_port, proxy_ep);
}
