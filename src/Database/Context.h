#pragma once

#include <thread>

namespace SQLite
{
class Database;
class Statement;
} // namespace SQLite

namespace slade::database
{
class Statement;
class Transaction;

class Context
{
public:
	Context(string_view file_path = {}, bool create = false);
	~Context();

	const string&     filePath() const { return file_path_; }
	SQLite::Database* connectionRO() const { return connection_ro_.get(); }
	SQLite::Database* connectionRW() const { return connection_rw_.get(); }

	bool isOpen() const { return connection_ro_.get(); }
	bool isForThisThread() const;

	void open(string_view file_path, bool create = false);
	void close();

	Statement preparedStatement(string_view id);
	Statement preparedStatement(string_view id, string_view sql, bool writes = false);

	int exec(const string& query) const;
	int exec(const char* query) const;

	bool rowIdExists(string_view table_name, int64_t id, string_view id_col = "id") const;
	bool tableExists(const string& table_name) const;
	bool viewExists(const string& view_name) const;

	Transaction beginTransaction(bool write = false) const;

	void vacuum() const;

private:
	string          file_path_;
	std::thread::id thread_id_;

	unique_ptr<SQLite::Database> connection_ro_;
	unique_ptr<SQLite::Database> connection_rw_;

	std::map<string, unique_ptr<SQLite::Statement>, std::less<>> prepared_statements_;
};

Context& context();
void     registerThreadContext(Context& context);
void     deregisterThreadContexts();
} // namespace slade::database
