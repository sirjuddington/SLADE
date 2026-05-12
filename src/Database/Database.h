#pragma once

// Forward declarations
namespace slade
{
class Archive;
namespace database
{
	class Context;
} // namespace database
} // namespace slade
namespace SQLite
{
class Database;
}

namespace slade::database
{
// General database functions
bool   fileExists();
string programDatabasePath();
bool   init();
void   close();
void   migrateConfigs();

// Signals
struct Signals
{
	// Table updates
	sigslot::signal<> archive_file_updated;
};
Signals& signals();

} // namespace slade::database
