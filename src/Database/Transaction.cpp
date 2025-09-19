
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2025 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Transaction.cpp
// Description: Encapsulates a single SQL transaction, ensuring it's closed off
//              properly etc. via RAII
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
#include "Transaction.h"
#include <SQLiteCpp/Database.h>
#include <sqlite3.h>

using namespace slade;
using namespace database;


// -----------------------------------------------------------------------------
//
// Transaction Class Functions
//
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Transaction class constructor
// -----------------------------------------------------------------------------
Transaction::Transaction(SQLite::Database* connection, bool begin) : connection_{ connection }, has_begun_{ begin }
{
	if (begin)
		connection->exec("BEGIN");
}

// -----------------------------------------------------------------------------
// Transaction class move constructor
// -----------------------------------------------------------------------------
Transaction::Transaction(Transaction&& other) noexcept :
	connection_{ other.connection_ },
	has_begun_{ other.has_begun_ },
	has_ended_{ other.has_ended_ }
{
	other.connection_ = nullptr;
	other.has_begun_  = true;
	other.has_ended_  = true;
}

// -----------------------------------------------------------------------------
// Transaction class destructor
// -----------------------------------------------------------------------------
Transaction::~Transaction()
{
	if (has_begun_ && !has_ended_)
		connection_->exec("ROLLBACK");
}

// -----------------------------------------------------------------------------
// Begins the transaction if it isn't active already
// -----------------------------------------------------------------------------
void Transaction::begin()
{
	if (has_begun_)
		return;

	connection_->exec("BEGIN");
	has_begun_ = true;
}

// -----------------------------------------------------------------------------
// Begins the transaction if there are no currently active transactions on the
// connection
// -----------------------------------------------------------------------------
void Transaction::beginIfNoActiveTransaction()
{
	if (sqlite3_get_autocommit(connection_->getHandle()) != 0)
		begin();
}

// -----------------------------------------------------------------------------
// Commits the transaction
// -----------------------------------------------------------------------------
void Transaction::commit()
{
	if (!has_begun_ || has_ended_)
		return;

	connection_->exec("COMMIT");
	has_ended_ = true;
}

// -----------------------------------------------------------------------------
// Rolls the transaction back
// -----------------------------------------------------------------------------
void Transaction::rollback()
{
	if (!has_begun_ || has_ended_)
		return;

	connection_->exec("ROLLBACK");
	has_ended_ = true;
}
