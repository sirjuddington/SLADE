#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <thread>

namespace slade::database
{
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

	SQLite::Statement* cachedQuery(string_view id);
	SQLite::Statement* cacheQuery(string_view id, const char* sql, bool writes = false);

	int exec(const string& query) const;
	int exec(const char* query) const;

	SQLite::Transaction beginTransaction(bool write = false) const;

	void vacuum() const { exec("VACUUM;"); } // Cleans up the database file to reduce size on disk

private:
	string          file_path_;
	std::thread::id thread_id_;

	unique_ptr<SQLite::Database> connection_ro_;
	unique_ptr<SQLite::Database> connection_rw_;

	std::map<string, unique_ptr<SQLite::Statement>, std::less<>> cached_queries_;
};

// Global (config) database context
Context&          global();
SQLite::Database* connectionRO();
SQLite::Database* connectionRW();
bool              fileExists();

// Contexts for threads
void registerThreadContext(Context& context);
void deregisterThreadContexts();

// Helpers
int                       exec(const string& query, SQLite::Database* connection = nullptr);
int                       exec(const char* query, SQLite::Database* connection = nullptr);
template<typename T> bool rowExists(SQLite::Database& connection, string_view table_name, string_view col_name, T value)
{
	SQLite::Statement sql(connection, fmt::format("SELECT * FROM {} WHERE {} = ?", table_name, col_name));
	sql.bind(1, value);
	return sql.executeStep();
}

// General
string programDatabasePath();
bool   init();
void   close();

} // namespace slade::database
