#pragma once

namespace slade
{
class SeekableData
{
public:
	virtual ~SeekableData() = default;

	virtual unsigned currentPos() const = 0;
	virtual unsigned size() const       = 0;

	virtual bool seek(unsigned offset) const          = 0;
	virtual bool seekFromStart(unsigned offset) const = 0;
	virtual bool seekFromEnd(unsigned offset) const   = 0;

	virtual bool read(void* buffer, unsigned count) const  = 0;
	virtual bool write(const void* buffer, unsigned count) = 0;

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
