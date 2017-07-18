#pragma once

#include "common.h"

// ----------------------------------------------------------------------------
// wxString support for Sol
//
// See: https://github.com/ThePhD/sol2/issues/140
// ----------------------------------------------------------------------------
namespace sol
{
namespace stack
{

template<>
struct pusher<wxString>
{
	static int push(lua_State* L, const wxString& str)
	{
		//return stack::push(L, CHR(str));
		return stack::push(L, static_cast<const char*>(str.ToAscii()));
	}
};

template<>
struct getter<wxString>
{
	static wxString get(lua_State *L, int index, record &tracking)
	{
		tracking.use(1);
		const char* luastr = stack::get<const char *>(L, index);
		return wxString::FromUTF8(luastr);
	}
};

} // namespace stack

template<>
struct lua_type_of<wxString> : std::integral_constant<type, type::string> {};

} // namespace sol
