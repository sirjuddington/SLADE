
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Context.cpp
// Description: Database Context class - keeps connections open to a database,
//              since opening a new connection is expensive. It can also keep
//              prepared sql statements (for frequent reuse)
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Context.h"
#include "App.h"
#include "Statement.h"
#include "Transaction.h"
#include <SQLiteCpp/Database.h>
#include <shared_mutex>

using namespace slade;
using namespace database;


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace slade::database
{
vector<Context*>  thread_contexts;
std::shared_mutex mutex_thread_contexts;
} // namespace slade::database


// -----------------------------------------------------------------------------
//
// Context Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Context class constructor
// -----------------------------------------------------------------------------
Context::Context(string_view file_path, bool create)
{
	thread_id_ = std::this_thread::get_id();

	if (!file_path.empty())
		open(file_path, create);
}

// -----------------------------------------------------------------------------
// Context class destructor
// -----------------------------------------------------------------------------
Context::~Context()
{
	close();

	for (int i = static_cast<int>(thread_contexts.size()) - 1; i >= 0; i--)
		if (thread_contexts[i] == this)
			thread_contexts.erase(thread_contexts.begin() + i);
}

// -----------------------------------------------------------------------------
// Returns true if the context was created on the current thread
// -----------------------------------------------------------------------------
bool Context::isForThisThread() const
{
	return thread_id_ == std::this_thread::get_id();
}

// -----------------------------------------------------------------------------
// Opens connections to the database file at [file_path].
// Returns false if any existing connections couldn't be closed, true otherwise
// -----------------------------------------------------------------------------
void Context::open(string_view file_path, bool create)
{
	close();

	file_path_     = file_path;
	connection_rw_ = std::make_unique<SQLite::Database>(
		file_path_, create ? SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE : SQLite::OPEN_READWRITE, 1000);
	connection_ro_ = std::make_unique<SQLite::Database>(file_path_, SQLite::OPEN_READONLY, 1000);
}

// -----------------------------------------------------------------------------
// Closes the context's connections to its database
// -----------------------------------------------------------------------------
void Context::close()
{
	if (!connection_ro_)
		return;

	prepared_statements_.clear();
	file_path_.clear();
	connection_ro_ = nullptr;
	connection_rw_ = nullptr;
}

// -----------------------------------------------------------------------------
// Returns the prepared statement [id] or nullptr if not found.
// -----------------------------------------------------------------------------
Statement Context::preparedStatement(string_view id)
{
	auto i = prepared_statements_.find(id);
	if (i != prepared_statements_.end())
		return { *i->second };

	throw std::runtime_error(fmt::format("Prepared statement with id \"{}\" does not exist"));
}

// -----------------------------------------------------------------------------
// Returns the prepared statement [id] if it exists, otherwise creates prepared
// statement from the given [sql] string and returns it.
// If [writes] is true, the created query will use the read+write connection.
// -----------------------------------------------------------------------------
Statement Context::preparedStatement(string_view id, string_view sql, bool writes)
{
	// Check for existing prepared statement [id]
	auto i = prepared_statements_.find(id);
	if (i != prepared_statements_.end())
		return { *i->second };

	// Check connection
	if (!connection_ro_)
		throw std::runtime_error("Database Context is not open");

	// Create & add prepared statement
	auto& db        = writes ? *connection_rw_ : *connection_ro_;
	auto  statement = std::make_unique<SQLite::Statement>(db, string{ sql });
	if (auto [it, success] = prepared_statements_.emplace(id, std::move(statement)); success)
		return { *it->second };
	else
		throw std::runtime_error("Failed to insert prepared statement");
}

// -----------------------------------------------------------------------------
// Executes an sql [query] on the database.
// Returns the number of rows modified/created by the query, or 0 if the context
// is not connected
// -----------------------------------------------------------------------------
int Context::exec(const string& query) const
{
	return connection_rw_ ? connection_rw_->exec(query) : 0;
}
int Context::exec(const char* query) const
{
	return connection_rw_ ? connection_rw_->exec(query) : 0;
}

// -----------------------------------------------------------------------------
// Returns true if a row exists in [table_name] where [id_col] = [id].
// The column must be an integer column for this to work correctly
// -----------------------------------------------------------------------------
bool Context::rowIdExists(string_view table_name, int64_t id, string_view id_col) const
{
	auto query = fmt::format("SELECT EXISTS(SELECT 1 FROM {} WHERE {} = {})", table_name, id_col, id);
	return connection_ro_->execAndGet(query).getInt() > 0;
}

// -----------------------------------------------------------------------------
// Returns true if a table with the name [table_name] exists in the database
// -----------------------------------------------------------------------------
bool Context::tableExists(const string& table_name) const
{
	return connection_ro_->tableExists(table_name);
}

// -----------------------------------------------------------------------------
// Returns true if a view with [view_name] exists in the database
// -----------------------------------------------------------------------------
bool Context::viewExists(const string& view_name) const
{
	SQLite::Statement query(*connection_ro_, "SELECT count(*) FROM sqlite_master WHERE type='view' AND name=?");
	query.bind(1, view_name);
	query.executeStep(); // Cannot return false, as the above query always returns a result
	return query.getColumn(0).getInt() == 1;
}

// -----------------------------------------------------------------------------
// Begins a transaction and returns a Transaction object to encapsulate it
// -----------------------------------------------------------------------------
Transaction Context::beginTransaction(bool write) const
{
	return { write ? connection_rw_.get() : connection_ro_.get(), true };
}

// -----------------------------------------------------------------------------
// Cleans up the database file to reduce size on disk
// -----------------------------------------------------------------------------
void Context::vacuum() const
{
	exec("VACUUM;");
}


// -----------------------------------------------------------------------------
//
// Database Namespace Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Returns the database connection context for this thread.
//
// If this isn't being called from the main thread, it will first look for a
// context that has previously been registered for the current thread via
// registerThreadContext. If no context has been registered for the thread, the
// main thread's context will be returned and a warning logged
// -----------------------------------------------------------------------------
Context& database::context()
{
	// Check if we are not on the main thread
	if (std::this_thread::get_id() != app::mainThreadId())
	{
		std::shared_lock lock(mutex_thread_contexts);

		// Find context for this thread
		for (auto* thread_context : thread_contexts)
			if (thread_context->isForThisThread())
				return *thread_context;

		// No context available for this thread, warn and use main thread context
		// (should this throw an exception?)
		log::warning("A non-main thread is requesting the main thread's database connection context");
	}

	static Context main_thread_context;

	return main_thread_context;
}

// -----------------------------------------------------------------------------
// Sets [context] as the database connection context to use for the current
// thread when calling database::global()
// -----------------------------------------------------------------------------
void database::registerThreadContext(Context& context)
{
	std::unique_lock lock(mutex_thread_contexts);

	thread_contexts.push_back(&context);
}

// -----------------------------------------------------------------------------
// Clears all contexts registered for the current thread
// -----------------------------------------------------------------------------
void database::deregisterThreadContexts()
{
	std::unique_lock lock(mutex_thread_contexts);

	for (int i = static_cast<int>(thread_contexts.size()) - 1; i >= 0; i--)
		if (thread_contexts[i]->isForThisThread())
			thread_contexts.erase(thread_contexts.begin() + i);
}
