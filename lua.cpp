#include "stdafx.h"
#include "lua.h"
#include "Event.h"
#include "Logs.h"
#include "MemCell.h"
#include "Driver.h"
#include "lua_ffi.h"
#include "simulation.h"

lua::lua()
{
    state = luaL_newstate();
    if (!state)
        throw std::runtime_error("cannot create lua state");
    lua_atpanic(state, atpanic);
    luaL_openlibs(state);

    lua_getglobal(state, "package");
    lua_pushstring(state, "preload");
    lua_gettable(state, -2);
    lua_pushcclosure(state, openffi, 0);
    lua_setfield(state, -2, "eu07.events");
    lua_settop(state, 0);
}

lua::~lua()
{
    lua_close(state);
    state = nullptr;
}

void lua::interpret(std::string file)
{
    if (luaL_dofile(state, file.c_str()))
        throw std::runtime_error(lua_tostring(state, -1));
}

// NOTE: we cannot throw exceptions in callbacks
// because it is not supported by LuaJIT on x86 windows

int lua::atpanic(lua_State *s)
{
	std::string err(lua_tostring(s, -1));
    ErrorLog(err);
#ifdef _WIN32
        MessageBox(NULL, err.c_str(), "MaSzyna", MB_OK);
#endif
    return 0;
}

int lua::openffi(lua_State *s)
{
    if (luaL_dostring(s, lua_ffi))
    {
        ErrorLog(std::string(lua_tostring(s, -1)));
        return 0;
    }
    return 1;
}

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

extern "C"
{
    EXPORT TEvent* scriptapi_event_create(const char* name, double delay, double randomdelay, lua::eventhandler_t handler)
    {
        TEvent *event = new TEvent();
        event->bEnabled = true;
        event->Type = tp_Lua;
        event->asName = std::string(name);
        event->fDelay = delay;
        event->fRandomDelay = randomdelay;
        event->Params[0].asPointer = (void*)handler;
		if (simulation::Events.insert(event))
			return event;
		else
			return nullptr;
    }

    EXPORT TEvent* scriptapi_event_find(const char* name)
    {
        std::string str(name);
        TEvent *e = simulation::Events.FindEvent(str);
        if (e)
            return e;
        else
            WriteLog("lua: missing event: " + str);
        return nullptr;
    }

    EXPORT TTrack* scriptapi_track_find(const char* name)
    {
        std::string str(name);
		TTrack *track = simulation::Paths.find(str);
        if (track)
            return track;
        else
            WriteLog("lua: missing track: " + str);
        return nullptr;
    }

    EXPORT bool scriptapi_track_isoccupied(TTrack* track)
    {
        if (track)
            return !track->IsEmpty();
        return false;
    }

	EXPORT TIsolated* scriptapi_isolated_find(const char* name)
	{
		std::string str(name);
        TIsolated *isolated = TIsolated::Find(name);
		if (isolated)
			return isolated;
		else
			WriteLog("lua: missing isolated: " + str);
		return nullptr;
	}

	EXPORT bool scriptapi_isolated_isoccupied(TIsolated* isolated)
	{
		if (isolated)
			return isolated->Busy();
		return false;
	}

    EXPORT const char* scriptapi_event_getname(TEvent *e)
    {
        if (e)
            return e->asName.c_str();
        return nullptr;
    }

    EXPORT const char* scriptapi_train_getname(TDynamicObject *dyn)
    {
        if (dyn && dyn->Mechanik)
            return dyn->Mechanik->TrainName().c_str();
        return nullptr;
    }

    EXPORT void scriptapi_event_dispatch(TEvent *e, TDynamicObject *activator, double delay)
    {
        if (e)
            simulation::Events.AddToQuery(e, activator, delay);
    }

    EXPORT double scriptapi_random(double a, double b)
    {
        return Random(a, b);
    }

    EXPORT void scriptapi_writelog(const char* txt)
    {
        WriteLog("lua: log: " + std::string(txt), logtype::lua);
    }

    EXPORT void scriptapi_writeerrorlog(const char* txt)
    {
        ErrorLog("lua: log: " + std::string(txt), logtype::lua);
    }

    struct memcell_values { const char *str; double num1; double num2; };

    EXPORT TMemCell* scriptapi_memcell_find(const char *name)
    {
        std::string str(name);
        TMemCell *mc = simulation::Memory.find(str);
        if (mc)
            return mc;
        else
            WriteLog("lua: missing memcell: " + str);
        return nullptr;
    }

    EXPORT memcell_values scriptapi_memcell_read(TMemCell *mc)
    {
        if (!mc)
            return { nullptr, 0.0, 0.0 };
        return { mc->Text().c_str(), mc->Value1(), mc->Value2() };
    }

    EXPORT void scriptapi_memcell_update(TMemCell *mc, const char *str, double num1, double num2)
    {
        if (!mc)
            return;
        mc->UpdateValues(std::string(str), num1, num2,
                         update_memstring | update_memval1 | update_memval2);
    }

    EXPORT void scriptapi_dynobj_putvalues(TDynamicObject *dyn, const char *str, double num1, double num2)
    {
        if (!dyn)
            return;
        TLocation loc;
        if (dyn->Mechanik)
            dyn->Mechanik->PutCommand(std::string(str), num1, num2, nullptr);
        else
            dyn->MoverParameters->PutCommand(std::string(str), num1, num2, loc);
    }
}
