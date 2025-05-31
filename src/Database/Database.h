#pragma once

namespace SQLite
{
class Database;
}

namespace slade::database
{
bool   fileExists();
string programDatabasePath();
bool   init();
void   close();
void   migrateConfigs();
} // namespace slade::database
