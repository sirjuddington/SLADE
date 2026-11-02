#pragma once

namespace slade
{
class SeekableData
{
public:
	virtual ~SeekableData() = default;

	virtual u64 currentPos() const = 0;
	virtual u64 size() const       = 0;

	virtual bool seek(u64 offset) const          = 0;
	virtual bool seekFromStart(u64 offset) const = 0;
	virtual bool seekFromEnd(u64 offset) const   = 0;

	virtual bool read(void* buffer, u64 count) const  = 0;
	virtual bool write(const void* buffer, u64 count) = 0;

	virtual u64 lastReadCount() const { return 0; }

	template<typename T> bool read(T& var) { return read(&var, sizeof(T)); }
	template<typename T> bool write(T& var) { return write(&var, sizeof(T)); }

	template<typename T> T get()
	{
		T var = T{};
		read<T>(var);
		return var;
	}
};
} // namespace slade
