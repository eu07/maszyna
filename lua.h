#pragma once
#include <lua.hpp>

class TEvent;
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

    typedef void (*eventhandler_t)(TEvent*, const TDynamicObject*);
};
