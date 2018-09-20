#pragma once
#include <lua.hpp>

class basic_event;
class TDynamicObject;

class lua
{
    lua_State *state;

    static int atpanic(lua_State *s);
    static int openffi(lua_State *s);

public:
    lua();
    ~lua();

    void interpret(std::string file);

    typedef void (*eventhandler_t)(basic_event*, const TDynamicObject*);
};
