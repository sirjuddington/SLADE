#pragma once

namespace SQLite
{
class Database;
}

namespace slade::database
{
class Transaction
{
public:
	Transaction(SQLite::Database* connection, bool begin = true);
	Transaction(Transaction&& other) noexcept;
	~Transaction();

	Transaction(const Transaction&)            = delete;
	Transaction& operator=(const Transaction&) = delete;

	void begin();
	void beginIfNoActiveTransaction();
	void commit();
	void rollback();

private:
	SQLite::Database* connection_ = nullptr;
	bool              has_begun_  = false;
	bool              has_ended_  = false;
};
} // namespace slade::database
