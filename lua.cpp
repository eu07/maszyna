#include "stdafx.h"
#include "lua.h"
#include "Event.h"
#include "Logs.h"
#include "MemCell.h"
#include "World.h"
#include "Driver.h"
#include "lua_ffi.h"

extern TWorld World;

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
    ErrorLog(std::string(lua_tostring(s, -1)));
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

extern "C"
{
    TEvent* scriptapi_event_create(const char* name, lua::eventhandler_t handler, double delay)
    {
        TEvent *event = new TEvent();
        event->bEnabled = true;
        event->Type = tp_Lua;
        event->asName = std::string(name);
        event->fDelay = delay;
        event->Params[0].asPointer = (void*)handler;
        World.Ground.add_event(event);
        return event;
    }

    TEvent* scriptapi_event_find(const char* name)
    {
        std::string str(name);
        TEvent *e = World.Ground.FindEvent(str);
        if (e)
            return e;
        else
            WriteLog("missing event: " + str);
        return nullptr;
    }

    TTrack* scriptapi_track_find(const char* name)
    {
        std::string str(name);
        TGroundNode *n = World.Ground.FindGroundNode(str, TP_TRACK);
        if (n)
            return n->pTrack;
        else
            WriteLog("missing track: " + str);
        return nullptr;
    }

    bool scriptapi_track_isoccupied(TTrack* track)
    {
        if (track)
            return !track->IsEmpty();
        return false;
    }

    const char* scriptapi_event_getname(TEvent *e)
    {
        if (e)
            return e->asName.c_str();
        return nullptr;
    }

    const char* scriptapi_train_getname(TDynamicObject *dyn)
    {
        if (dyn && dyn->Mechanik)
            return dyn->Mechanik->TrainName().c_str();
        return nullptr;
    }

    void scriptapi_event_dispatch(TEvent *e, TDynamicObject *activator)
    {
        if (e)
            World.Ground.AddToQuery(e, activator);
    }

    double scriptapi_random(double a, double b)
    {
        return Random(a, b);
    }

    void scriptapi_writelog(const char* txt)
    {
        WriteLog("lua log: " + std::string(txt));
    }

    struct memcell_values { const char *str; double num1; double num2; };

    TMemCell* scriptapi_memcell_find(const char *name)
    {
        std::string str(name);
        TGroundNode *n = World.Ground.FindGroundNode(str, TP_MEMCELL);
        if (n)
            return n->MemCell;
        else
            WriteLog("missing memcell: " + str);
        return nullptr;
    }

    memcell_values scriptapi_memcell_read(TMemCell *mc)
    {
        if (!mc)
            return { nullptr, 0.0, 0.0 };
        return { mc->Text().c_str(), mc->Value1(), mc->Value2() };
    }

    void scriptapi_memcell_update(TMemCell *mc, const char *str, double num1, double num2)
    {
        if (!mc)
            return;
        mc->UpdateValues(std::string(str), num1, num2,
                         update_memstring | update_memval1 | update_memval2);
    }

    void scriptapi_dynobj_putvalues(TDynamicObject *dyn, const char *str, double num1, double num2)
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
