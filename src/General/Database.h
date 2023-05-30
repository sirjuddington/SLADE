#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <thread>

namespace slade::database
{
class Transaction
{
public:
	Transaction(SQLite::Database* connection, bool begin = true) : connection_{ connection }, has_begun_{ begin }
	{
		if (begin)
			connection->exec("BEGIN");
	}

	Transaction(Transaction&& other) noexcept :
		connection_{ other.connection_ }, has_begun_{ other.has_begun_ }, has_ended_{ other.has_ended_ }
	{
		other.connection_ = nullptr;
		other.has_begun_  = true;
		other.has_ended_  = true;
	}

	~Transaction()
	{
		if (has_begun_ && !has_ended_)
			connection_->exec("ROLLBACK");
	}

	Transaction(const Transaction&)            = delete;
	Transaction& operator=(const Transaction&) = delete;

	void begin()
	{
		if (has_begun_)
			return;

		connection_->exec("BEGIN");
		has_begun_ = true;
	}

	void beginIfNoActiveTransaction();

	void commit()
	{
		if (!has_begun_ || has_ended_)
			return;

		connection_->exec("COMMIT");
		has_ended_ = true;
	}

	void rollback()
	{
		if (!has_begun_ || has_ended_)
			return;

		connection_->exec("ROLLBACK");
		has_ended_ = true;
	}

private:
	SQLite::Database* connection_ = nullptr;
	bool              has_begun_  = false;
	bool              has_ended_  = false;
};

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

	void bindNoCopy(int index, const string& value) { SQLite::Statement::bindNoCopy(index, value); }
	void bindNoCopy(int index, const char* value) { SQLite::Statement::bindNoCopy(index, value); }
	void bindNoCopy(int index, string_view value)
	{
		SQLite::Statement::bindNoCopy(index, string{ value.data(), value.size() });
	}
	void bindNoCopy(int index, const void* value, int size) { SQLite::Statement::bindNoCopy(index, value, size); }
};

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

	void vacuum() const { exec("VACUUM;"); } // Cleans up the database file to reduce size on disk

private:
	string          file_path_;
	std::thread::id thread_id_;

	unique_ptr<SQLite::Database> connection_ro_;
	unique_ptr<SQLite::Database> connection_rw_;

	std::map<string, unique_ptr<Statement>, std::less<>> cached_queries_;
};

// Global (config) database context
Context& global();
bool     fileExists();

// Contexts for threads
void registerThreadContext(Context& context);
void deregisterThreadContexts();

// Helpers
bool        isTransactionActive(const SQLite::Database* connection);
int         exec(const string& query, SQLite::Database* connection = nullptr);
int         exec(const char* query, SQLite::Database* connection = nullptr);
bool        viewExists(string_view view_name, const SQLite::Database& connection);
inline bool rowIdExists(string_view table_name, int64_t id, string_view id_col = "id")
{
	return global().rowIdExists(table_name, id, id_col);
}
template<typename T> bool rowExists(SQLite::Database& connection, string_view table_name, string_view col_name, T value)
{
	Statement sql(connection, fmt::format("SELECT * FROM {} WHERE {} = ?", table_name, col_name));
	sql.bind(1, value);
	return sql.executeStep();
}
inline Statement* cacheQuery(string_view id, string_view sql, bool writes = false)
{
	return global().cacheQuery(id, sql, writes);
}
inline SQLite::Database* connectionRO()
{
	return global().connectionRO();
}
inline SQLite::Database* connectionRW()
{
	return global().connectionRW();
}

// General
string programDatabasePath();
bool   init();
void   close();
void   migrateConfigs();

} // namespace slade::database

// Shortcut alias for database namespace
namespace db = slade::database;