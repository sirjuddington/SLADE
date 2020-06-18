#pragma once

// Simple struct containing an array of scoped signal connections, so classes that connect to many different signals
// don't need to keep a separate scoped_connection member variable for each one.
// Also has a += operator for convenience:
// ScopedConnectionList += signal.connect(...);
struct ScopedConnectionList
{
	vector<sigslot::scoped_connection> connections;

	void operator+=(sigslot::connection c) { connections.emplace_back(c); }
};
