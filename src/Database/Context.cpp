
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2024 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Context.cpp
// Description: Database Context class - keeps connections open to a database,
//              since opening a new connection is expensive. It can also keep
//              cached sql queries (for frequent reuse)
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
Context           db_global;
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
Context::Context(string_view file_path)
{
	thread_id_ = std::this_thread::get_id();

	if (!file_path.empty())
		open(file_path);
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
bool Context::open(string_view file_path)
{
	if (!close())
		return false;

	file_path_     = file_path;
	connection_ro_ = std::make_unique<SQLite::Database>(file_path_, SQLite::OPEN_READONLY, 100);
	connection_rw_ = std::make_unique<SQLite::Database>(file_path_, SQLite::OPEN_READWRITE, 100);

	return true;
}

// -----------------------------------------------------------------------------
// Closes the context's connections to its database
// -----------------------------------------------------------------------------
bool Context::close()
{
	if (!connection_ro_)
		return true;

	try
	{
		cached_queries_.clear();
		connection_ro_ = nullptr;
		connection_rw_ = nullptr;
	}
	catch (std::exception& ex)
	{
		log::error("Error closing connections for database {}: {}", file_path_, ex.what());
		return false;
	}

	file_path_.clear();
	return true;
}

// -----------------------------------------------------------------------------
// Returns the cached query [id] or nullptr if not found
// -----------------------------------------------------------------------------
Statement* Context::cachedQuery(string_view id)
{
	auto i = cached_queries_.find(id);
	if (i != cached_queries_.end())
	{
		i->second->tryReset();
		return i->second.get();
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the cached query [id] if it exists, otherwise creates a new cached
// query from the given [sql] string and returns it.
// If [writes] is true, the created query will use the read+write connection.
// -----------------------------------------------------------------------------
Statement* Context::cacheQuery(string_view id, string_view sql, bool writes)
{
	// Check for existing cached query [id]
	auto i = cached_queries_.find(id);
	if (i != cached_queries_.end())
	{
		i->second->tryReset();
		return i->second.get();
	}

	// Check connection
	if (!connection_ro_)
		return nullptr;

	// Create & add cached query
	auto& db        = writes ? *connection_rw_ : *connection_ro_;
	auto  statement = std::make_unique<Statement>(db, sql);
	auto  ptr       = statement.get();
	cached_queries_.emplace(id, std::move(statement));

	return ptr;
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
// Returns the 'global' database connection context for this thread.
//
// If this isn't being called from the main thread, it will first look for a
// context that has previously been registered for the current thread via
// registerThreadContext. If no context has been registered for the thread, the
// main thread's context will be returned and a warning logged
// -----------------------------------------------------------------------------
Context& database::global()
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
		log::warning("A non-main thread is requesting the global database connection context");
	}

	return db_global;
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
