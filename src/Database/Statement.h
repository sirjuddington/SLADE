#pragma once

namespace SQLite
{
class Statement;
class Column;
} // namespace SQLite

namespace slade::database
{
// A wrapper around an SQLite::Statement that supports binding string_view,
// clears bindings when constructed and resets the statement when destroyed
class Statement
{
public:
	Statement(SQLite::Statement& statement);
	~Statement();

	SQLite::Statement& statement() const { return *statement_; }

	void bind(int index, int32_t value) const;
	void bind(int index, uint32_t value) const;
	void bind(int index, int64_t value) const;
	void bind(int index, double value) const;
	void bind(int index, const string& value) const;
	void bind(int index, const char* value) const;
	void bind(int index, string_view value) const;
	void bind(int index, const void* value, int size) const;
	void bind(int index) const;
	void bindDateTime(int index, time_t value) const;
	void bindNoCopy(int index, const string& value) const;
	void bindNoCopy(int index, const char* value) const;
	void bindNoCopy(int index, const void* value, int size) const;

	int  exec() const;
	bool executeStep() const;
	void reset() const;

	SQLite::Column getColumn(int index) const;
	SQLite::Column getColumn(const string& name) const;
	SQLite::Column getColumn(string_view name) const;

private:
	SQLite::Statement* statement_ = nullptr;
};
} // namespace slade::database
