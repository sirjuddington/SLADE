#pragma once

class wxChoice;
class wxTextCtrl;

namespace slade
{
namespace map
{
	enum class ObjectType : u8;
}

class ShowItemDialog : public wxDialog
{
public:
	ShowItemDialog(wxWindow* parent);
	~ShowItemDialog() override = default;

	map::ObjectType type() const;
	int             index() const;
	void            setType(map::ObjectType type) const;

private:
	wxChoice*   choice_type_ = nullptr;
	wxTextCtrl* text_index_  = nullptr;
};
} // namespace slade
