#ifndef DRIVER_HINT_INCL
#define DRIVER_HINT_INCL
#define DRIVER_HINT_DEF(a, b) a,

enum class driver_hint {
    #include "driverhints_def.h"
};

#undef DRIVER_HINT_DEF
#endif

#ifdef DRIVER_HINT_CONTENT
#define DRIVER_HINT_DEF(a, b) b,

const char *driver_hints_texts[] =
{
    #include "driverhints_def.h"
};

#undef DRIVER_HINT_DEF
#endif
