
#include "Main.h"
#include "DateTime.h"
#include <chrono>
#include <ctime>
#include <fmt/chrono.h>

using namespace slade;
using namespace datetime;

time_t datetime::now()
{
	using clock = std::chrono::system_clock;
	return clock::to_time_t(clock::now());
}

time_t datetime::toLocalTime(time_t time_utc)
{
	return mktime(std::localtime(&time_utc));
}

time_t datetime::toUniversalTime(time_t time_local)
{
	return mktime(std::gmtime(&time_local));
}

string datetime::toString(time_t time, datetime::Format format, string_view custom_format)
{
	auto as_tm = *std::localtime(&time);

	switch (format)
	{
	case Format::ISO: return fmt::format("{:%F %T}", as_tm);
	case Format::Local: return fmt::format("{:%c}", as_tm);
	case Format::Custom: return fmt::format(fmt::format("{{:{}}}", custom_format), as_tm);
	}

	return {};
}
