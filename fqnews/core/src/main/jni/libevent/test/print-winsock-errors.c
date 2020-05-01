#include <winsock2.h>
#include <windows.h>

#include <stdlib.h>
#include <stdio.h>

#include "event2/event.h"
#include "event2/util.h"
#include "event2/thread.h"

#define E(x) printf (#x " -> \"%s\"\n", evutil_socket_error_to_string (x));

int main (int argc, char **argv)
{
  int i, j;
  const char *s1, *s2;

#ifdef EVTHREAD_USE_WINDOWS_THREADS_IMPLEMENTED
  evthread_use_windows_threads ();
#endif

  s1 = evutil_socket_error_to_string (WSAEINTR);

  for (i = 0; i < 3; i++) {
    printf ("\niteration %d:\n\n", i);
    E(WSAEINTR);
    E(WSAEACCES);
    E(WSAEFAULT);
    E(WSAEINVAL);
    E(WSAEMFILE);
    E(WSAEWOULDBLOCK);
    E(WSAEINPROGRESS);
    E(WSAEALREADY);
    E(WSAENOTSOCK);
    E(WSAEDESTADDRREQ);
    E(WSAEMSGSIZE);
    E(WSAEPROTOTYPE);
    E(WSAENOPROTOOPT);
    E(WSAEPROTONOSUPPORT);
    E(WSAESOCKTNOSUPPORT);
    E(WSAEOPNOTSUPP);
    E(WSAEPFNOSUPPORT);
    E(WSAEAFNOSUPPORT);
    E(WSAEADDRINUSE);
    E(WSAEADDRNOTAVAIL);
    E(WSAENETDOWN);
    E(WSAENETUNREACH);
    E(WSAENETRESET);
    E(WSAECONNABORTED);
    E(WSAECONNRESET);
    E(WSAENOBUFS);
    E(WSAEISCONN);
    E(WSAENOTCONN);
    E(WSAESHUTDOWN);
    E(WSAETIMEDOUT);
    E(WSAECONNREFUSED);
    E(WSAEHOSTDOWN);
    E(WSAEHOSTUNREACH);
    E(WSAEPROCLIM);
    E(WSASYSNOTREADY);
    E(WSAVERNOTSUPPORTED);
    E(WSANOTINITIALISED);
    E(WSAEDISCON);
    E(WSATYPE_NOT_FOUND);
    E(WSAHOST_NOT_FOUND);
    E(WSATRY_AGAIN);
    E(WSANO_RECOVERY);
    E(WSANO_DATA);
    E(0xdeadbeef); /* test the case where no message is available */

    /* fill up the hash table a bit to make sure it grows properly */
    for (j = 0; j < 50; j++) {
      int err;
      evutil_secure_rng_get_bytes(&err, sizeof(err));
      evutil_socket_error_to_string(err);
    }
  }

  s2 = evutil_socket_error_to_string (WSAEINTR);
  if (s1 != s2)
    printf ("caching failed!\n");

  libevent_global_shutdown ();

  return EXIT_SUCCESS;
}
