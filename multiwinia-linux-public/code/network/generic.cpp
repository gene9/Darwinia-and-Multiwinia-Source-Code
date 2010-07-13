#include "lib/universal_include.h"

#include "lib/netlib/net_lib.h"

#include "network/generic.h"


void IpToString(struct in_addr in, char *newip)
{
#if defined(TARGET_MSVC) 
        sprintf ( newip, "%u.%u.%u.%u", in.S_un.S_un_b.s_b1,
                                        in.S_un.S_un_b.s_b2,
                                        in.S_un.S_un_b.s_b3,
                                        in.S_un.S_un_b.s_b4 );
#elif defined(HAVE_INET_NTOA)
strcpy( newip, inet_ntoa( in ) );
#else
#error Need inet_ntoa or similar
#endif 
}
