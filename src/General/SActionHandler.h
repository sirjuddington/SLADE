#pragma once

namespace slade
{
// Basic 'interface' class for classes that handle SActions (yay multiple inheritance)
class SActionHandler
{
public:
	SActionHandler();
	virtual ~SActionHandler();

	static int  wxIdOffset();
	static void setWxIdOffset(int offset);
	static bool doAction(string_view id);

protected:
	virtual bool handleAction(string_view id) { return false; }
};
} // namespace slade
