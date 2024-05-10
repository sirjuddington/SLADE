#pragma once

#include "Statement.h"

namespace slade::database
{
template<typename T> bool rowExists(SQLite::Database& connection, string_view table_name, string_view col_name, T value)
{
	Statement sql(connection, fmt::format("SELECT * FROM {} WHERE {} = ?", table_name, col_name));
	sql.bind(1, value);
	return sql.executeStep();
}
} // namespace slade::database
