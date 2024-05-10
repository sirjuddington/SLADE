#pragma once

namespace SQLite
{
class Database;
}

namespace slade::database
{
class Statement;
class Transaction;

// Helpers
bool              isTransactionActive(const SQLite::Database* connection);
int               exec(const string& query, SQLite::Database* connection = nullptr);
int               exec(const char* query, SQLite::Database* connection = nullptr);
bool              viewExists(string_view view_name, const SQLite::Database& connection);
bool              rowIdExists(string_view table_name, int64_t id, string_view id_col = "id");
Statement*        cacheQuery(string_view id, string_view sql, bool writes = false);
SQLite::Database* connectionRO();
SQLite::Database* connectionRW();

// General
bool    fileExists();
string  programDatabasePath();
int64_t sessionId();
bool    init();
void    close();
void    migrateConfigs();
} // namespace slade::database

// Shortcut alias for database namespace
namespace db = slade::database;
