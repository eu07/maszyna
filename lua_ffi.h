const char lua_ffi[] = R"STRING(
local ffi = require("ffi")
ffi.cdef[[
struct memcell_values { const char *str; double num1; double num2; };

typedef struct TEvent TEvent;
typedef struct TTrack TTrack;
typedef struct TIsolated TIsolated;
typedef struct TDynamicObject TDynamicObject;
typedef struct TMemCell TMemCell;
typedef struct memcell_values memcell_values;

TEvent* scriptapi_event_create(const char* name, double delay, double randomdelay, void (*handler)(TEvent*, TDynamicObject*));
TEvent* scriptapi_event_find(const char* name);
const char* scriptapi_event_getname(TEvent *e);
void scriptapi_event_dispatch(TEvent *e, TDynamicObject *activator, double delay);

TTrack* scriptapi_track_find(const char* name);
bool scriptapi_track_isoccupied(TTrack *track);

TIsolated* scriptapi_isolated_find(const char* name);
bool scriptapi_isolated_isoccupied(TIsolated *isolated);

const char* scriptapi_train_getname(TDynamicObject *dyn);
void scriptapi_dynobj_putvalues(TDynamicObject *dyn, const char *str, double num1, double num2);

TMemCell* scriptapi_memcell_find(const char *name);
memcell_values scriptapi_memcell_read(TMemCell *mc);
void scriptapi_memcell_update(TMemCell *mc, const char *str, double num1, double num2);

double scriptapi_random(double a, double b);
void scriptapi_writelog(const char* txt);
void scriptapi_writeerrorlog(const char* txt);
]]

local ns = ffi.C

local module = {}

module.event_create = ns.scriptapi_event_create
function module.event_preparefunc(f)
    return ffi.cast("void (*)(TEvent*, TDynamicObject*)", f)
end
module.event_find = ns.scriptapi_event_find
function module.event_getname(a)
    return ffi.string(ns.scriptapi_event_getname(a))
end
module.event_dispatch = ns.scriptapi_event_dispatch
function module.event_dispatch_n(a, b, c)
    ns.scriptapi_event_dispatch(ns.scriptapi_event_find(a), b, c)
end

module.track_find = ns.scriptapi_track_find
module.track_isoccupied = ns.scriptapi_track_isoccupied
function module.track_isoccupied_n(a)
    return ns.scriptapi_track_isoccupied(ns.scriptapi_track_find(a))
end

module.isolated_find = ns.scriptapi_isolated_find
module.isolated_isoccupied = ns.scriptapi_isolated_isoccupied
function module.isolated_isoccupied_n(a)
    return ns.scriptapi_isolated_isoccupied(ns.scriptapi_isolated_find(a))
end

function module.train_getname(a)
    return ffi.string(ns.scriptapi_train_getname(a))
end
function module.dynobj_putvalues(a, b)
    ns.scriptapi_dynobj_putvalues(a, b.str, b.num1, b.num2)
end

module.memcell_find = ns.scriptapi_memcell_find
function module.memcell_read(a)
    native = ns.scriptapi_memcell_read(a)
    mc = { }
    mc.str = ffi.string(native.str)
    mc.num1 = native.num1
    mc.num2 = native.num2
    return mc
end
function module.memcell_read_n(a)
    return module.memcell_read(ns.scriptapi_memcell_find(a))
end
function module.memcell_update(a, b)
    ns.scriptapi_memcell_update(a, b.str, b.num1, b.num2)
end
function module.memcell_update_n(a, b)
    ns.scriptapi_memcell_update(ns.scriptapi_memcell_find(a), b.str, b.num1, b.num2)
end

module.random = ns.scriptapi_random
module.writelog = ns.scriptapi_writelog
module.writeerrorlog = ns.scriptapi_writeerrorlog

return module;
)STRING";
