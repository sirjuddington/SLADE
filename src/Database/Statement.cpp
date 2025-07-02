
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Statement.cpp
// Description: A wrapper around an SQLite::Statement that supports binding
//              string_view, clears bindings when constructed and resets the
//              statement when destroyed
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
#include "Statement.h"
#include "SQLiteCpp/Column.h"
#include <SQLiteCpp/Statement.h>

using namespace slade;
using namespace database;


// -----------------------------------------------------------------------------
//
// Statement Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Statement class constructor
// -----------------------------------------------------------------------------
Statement::Statement(SQLite::Statement& statement) : statement_{ &statement }
{
	statement_->clearBindings();
}

// -----------------------------------------------------------------------------
// Statement class destructor
// -----------------------------------------------------------------------------
Statement::~Statement()
{
	statement_->reset();
}

// -----------------------------------------------------------------------------
// Binds an int [value] to the statement at the given [index]
// -----------------------------------------------------------------------------
void Statement::bind(int index, int32_t value) const
{
	statement_->bind(index, value);
}

// -----------------------------------------------------------------------------
// Binds an unsigned int [value] to the statement at the given [index]
// -----------------------------------------------------------------------------
void Statement::bind(int index, uint32_t value) const
{
	statement_->bind(index, value);
}

// -----------------------------------------------------------------------------
// Binds a 64-bit int [value] to the statement at the given [index]
// -----------------------------------------------------------------------------
void Statement::bind(int index, int64_t value) const
{
	statement_->bind(index, value);
}

// -----------------------------------------------------------------------------
// Binds a double [value] to the statement at the given [index]
// -----------------------------------------------------------------------------
void Statement::bind(int index, double value) const
{
	statement_->bind(index, value);
}

// -----------------------------------------------------------------------------
// Binds a string [value] to the statement at the given [index]
// -----------------------------------------------------------------------------
void Statement::bind(int index, const string& value) const
{
	statement_->bind(index, value);
}

// -----------------------------------------------------------------------------
// Binds a string literal [value] to the statement at the given [index]
// -----------------------------------------------------------------------------
void Statement::bind(int index, const char* value) const
{
	statement_->bind(index, value);
}

// -----------------------------------------------------------------------------
// Binds a string_view [value] to the statement at the given [index]
// -----------------------------------------------------------------------------
void Statement::bind(int index, string_view value) const
{
	statement_->bind(index, string{ value.data(), value.size() });
}

// -----------------------------------------------------------------------------
// Binds a binary blob [value] of [size] to the statement at the given [index]
// -----------------------------------------------------------------------------
void Statement::bind(int index, const void* value, int size) const
{
	statement_->bind(index, value, size);
}

// -----------------------------------------------------------------------------
// Binds a NULL value to the statement at the given [index]
// -----------------------------------------------------------------------------
void Statement::bind(int index) const
{
	statement_->bind(index);
}

// ---------------------------------------------------------------------------
// Binds a time_t [value] to the statement at the given [index]
// Different name needed to avoid an ambiguous call error on some systems for
// time_t
// ---------------------------------------------------------------------------
void Statement::bindDateTime(int index, time_t value) const
{
	statement_->bind(index, static_cast<int64_t>(value));
}

// -----------------------------------------------------------------------------
// Binds a string [value] to the statement at the given [index]
// Does not copy the value when executing the statement
// -----------------------------------------------------------------------------
void Statement::bindNoCopy(int index, const string& value) const
{
	statement_->bindNoCopy(index, value);
}

// -----------------------------------------------------------------------------
// Binds a string literal [value] to the statement at the given [index]
// Does not copy the value when executing the statement
// -----------------------------------------------------------------------------
void Statement::bindNoCopy(int index, const char* value) const
{
	statement_->bindNoCopy(index, value);
}

// -----------------------------------------------------------------------------
// Binds a binary blob [value] of [size] to the statement at the given [index]
// Does not copy the value when executing the statement
// -----------------------------------------------------------------------------
void Statement::bindNoCopy(int index, const void* value, int size) const
{
	statement_->bindNoCopy(index, value, size);
}

// -----------------------------------------------------------------------------
// Executes the statement without fetching results (eg. UPDATE).
// Returns the number of rows affected by the statement
// -----------------------------------------------------------------------------
int Statement::exec() const
{
	return statement_->exec();
}

// -----------------------------------------------------------------------------
// Executes the statement to fetch one row of results.
// Returns false if there are no more rows to fetch, true otherwise
// -----------------------------------------------------------------------------
bool Statement::executeStep() const
{
	return statement_->executeStep();
}

// -----------------------------------------------------------------------------
// Resets the statement, preparing it for a new execution
// -----------------------------------------------------------------------------
void Statement::reset() const
{
	statement_->reset();
}

// -----------------------------------------------------------------------------
// Returns the column at the given [index] in the current row of results
// -----------------------------------------------------------------------------
SQLite::Column Statement::getColumn(int index) const
{
	return statement_->getColumn(index);
}

// -----------------------------------------------------------------------------
// Returns the column with the given [name] in the current row of results
// -----------------------------------------------------------------------------
SQLite::Column Statement::getColumn(const string& name) const
{
	return statement_->getColumn(name.c_str());
}

// -----------------------------------------------------------------------------
// Returns the column with the given [name] in the current row of results
// -----------------------------------------------------------------------------
SQLite::Column Statement::getColumn(string_view name) const
{
	return statement_->getColumn(string{ name }.c_str());
}
