#pragma once

#include "Context.h"
#include <SQLiteCpp/Statement.h>

namespace slade::database
{
template<typename T> bool rowExists(Context& db, string_view table_name, string_view col_name, T value)
{
	SQLite::Statement sql(*db.connectionRO(), fmt::format("SELECT * FROM {} WHERE {} = ?", table_name, col_name));
	sql.bind(1, value);
	return sql.executeStep();
}
} // namespace slade::database
