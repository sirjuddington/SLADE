
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DateTime.cpp
// Description: Date/Time utility functions
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "DateTime.h"
#include "thirdparty/fmt/include/fmt/chrono.h"
#include <chrono>
#include <ctime>

using namespace slade;
using namespace datetime;


// -----------------------------------------------------------------------------
//
// DateTime Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Returns the current time (local)
// -----------------------------------------------------------------------------
time_t datetime::now()
{
	using clock = std::chrono::system_clock;
	return clock::to_time_t(clock::now());
}

// -----------------------------------------------------------------------------
// Returns [time_utc] in the local timezone
// -----------------------------------------------------------------------------
time_t datetime::toLocalTime(time_t time_utc)
{
	return mktime(std::localtime(&time_utc));
}

// -----------------------------------------------------------------------------
// Returns [time_local] in the UTC timezone
// -----------------------------------------------------------------------------
time_t datetime::toUniversalTime(time_t time_local)
{
	return mktime(std::gmtime(&time_local));
}

// -----------------------------------------------------------------------------
// Returns [time] as a formatted string
// -----------------------------------------------------------------------------
string datetime::toString(time_t time, Format format, string_view custom_format)
{
	auto as_tm = *std::localtime(&time);

	switch (format)
	{
	case Format::ISO:    return fmt::format("{:%F %T}", as_tm);
	case Format::Local:  return fmt::format("{:%c}", as_tm);
	case Format::Custom: return fmt::format(fmt::runtime(fmt::format("{{:{}}}", custom_format)), as_tm);
	}

	return {};
}
