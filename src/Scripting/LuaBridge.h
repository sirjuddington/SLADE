#pragma once

// clang-format off

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

#include "thirdparty/luabridge/LuaBridge.h"
#include "thirdparty/luabridge/Vector.h"

// clang-format on

#define ADD_CLASS_CONSTANT(type, name, value) type.addStaticProperty(name, [] { return static_cast<int>(value); })
#define ADD_NS_CONSTANT(namespace, name, value) namespace.addProperty(name, [] { return static_cast<int>(value); })
