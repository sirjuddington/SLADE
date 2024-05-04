#pragma once

#include <SQLiteCpp/Statement.h>

namespace slade::database
{
// Wrapper of SQLiteCpp Statement to handle string_view
class Statement : public SQLite::Statement
{
public:
	Statement(const SQLite::Database& db, string_view query) :
		SQLite::Statement(db, string{ query.data(), query.size() })
	{
	}

	void bind(int index, int32_t value) { SQLite::Statement::bind(index, value); }
	void bind(int index, uint32_t value) { SQLite::Statement::bind(index, value); }
	void bind(int index, int64_t value) { SQLite::Statement::bind(index, value); }
	void bind(int index, double value) { SQLite::Statement::bind(index, value); }
	void bind(int index, const string& value) { SQLite::Statement::bind(index, value); }
	void bind(int index, const char* value) { SQLite::Statement::bind(index, value); }
	void bind(int index, string_view value) { SQLite::Statement::bind(index, string{ value.data(), value.size() }); }
	void bind(int index, const void* value, int size) { SQLite::Statement::bind(index, value, size); }
	void bind(int index) { SQLite::Statement::bind(index); }

	// Needed to avoid ambiguous call error on some systems for time_t
	void bindDateTime(int index, time_t value) { SQLite::Statement::bind(index, static_cast<int64_t>(value)); }

	void bindNoCopy(int index, const string& value) { SQLite::Statement::bindNoCopy(index, value); }
	void bindNoCopy(int index, const char* value) { SQLite::Statement::bindNoCopy(index, value); }
	void bindNoCopy(int index, string_view value)
	{
		SQLite::Statement::bindNoCopy(index, string{ value.data(), value.size() });
	}
	void bindNoCopy(int index, const void* value, int size) { SQLite::Statement::bindNoCopy(index, value, size); }
};
} // namespace slade::database
