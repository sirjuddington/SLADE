#pragma once

#include "Statement.h"
#include <thread>

namespace SQLite
{
class Database;
}

namespace slade::database
{
class Transaction;

class Context
{
public:
	Context(string_view file_path = {});
	~Context();

	const string&     filePath() const { return file_path_; }
	SQLite::Database* connectionRO() const { return connection_ro_.get(); }
	SQLite::Database* connectionRW() const { return connection_rw_.get(); }

	bool isOpen() const { return connection_ro_.get(); }
	bool isForThisThread() const;

	bool open(string_view file_path);
	bool close();

	Statement* cachedQuery(string_view id);
	Statement* cacheQuery(string_view id, string_view sql, bool writes = false);

	int exec(const string& query) const;
	int exec(const char* query) const;

	bool rowIdExists(string_view table_name, int64_t id, string_view id_col = "id") const;

	Transaction beginTransaction(bool write = false) const;

	void vacuum() const;

private:
	string          file_path_;
	std::thread::id thread_id_;

	unique_ptr<SQLite::Database> connection_ro_;
	unique_ptr<SQLite::Database> connection_rw_;

	std::map<string, unique_ptr<Statement>, std::less<>> cached_queries_;
};

// Global (config) database context
Context& global();

// Contexts for threads
void registerThreadContext(Context& context);
void deregisterThreadContexts();
} // namespace slade::database

// Shortcut alias for database namespace
namespace db = slade::database;
