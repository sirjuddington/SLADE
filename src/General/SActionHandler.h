#pragma once

namespace slade
{
// Basic 'interface' class for classes that handle SActions (yay multiple inheritance)
class SActionHandler
{
public:
	SActionHandler();
	virtual ~SActionHandler();

	static void setWxIdOffset(int offset) { wx_id_offset_ = offset; }
	static bool doAction(string_view id);

protected:
	static int wx_id_offset_;

	virtual bool handleAction(string_view id) { return false; }

private:
	static vector<SActionHandler*> action_handlers_;
};
} // namespace slade
