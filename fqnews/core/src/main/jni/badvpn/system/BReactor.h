#if defined(BADVPN_BREACTOR_BADVPN) + defined(BADVPN_BREACTOR_GLIB) + defined(BADVPN_BREACTOR_EMSCRIPTEN) != 1
#error No reactor backend or too many reactor backens
#endif

#if defined(BADVPN_BREACTOR_BADVPN)
#include "BReactor_badvpn.h"
#elif defined(BADVPN_BREACTOR_GLIB)
#include "BReactor_glib.h"
#elif defined(BADVPN_BREACTOR_EMSCRIPTEN)
#include "BReactor_emscripten.h"
#endif
