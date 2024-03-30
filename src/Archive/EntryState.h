#pragma once

namespace slade
{
enum class EntryState
{
	Unmodified,
	Modified,
	New,    // Newly created (not saved on disk yet)
	Deleted // Deleted (in case we have a shared_ptr hanging around to a deleted entry)
};
}
