#pragma once

#include "Browser.h"
#include "Utility/ColRGBA.h"

namespace slade
{
class TextBox;
class BrowserWindow;
namespace drawing
{
	enum class Font;
}

class BrowserItem
{
	friend class BrowserWindow;

public:
	BrowserItem(const wxString& name, unsigned index = 0, const wxString& type = "item");
	virtual ~BrowserItem();

	wxString name() const { return name_; }
	unsigned index() const { return index_; }

	virtual bool loadImage();
	void         draw(
				int               size,
				int               x,
				int               y,
				drawing::Font     font,
				browser::NameType nametype    = browser::NameType::Normal,
				browser::ItemView viewtype    = browser::ItemView::Normal,
				const ColRGBA&    colour      = ColRGBA::WHITE,
				bool              text_shadow = true);
	virtual void     clearImage() {}
	virtual wxString itemInfo() { return ""; }

protected:
	wxString            type_;
	wxString            name_;
	unsigned            index_     = 0;
	unsigned            image_tex_ = 0;
	BrowserWindow*      parent_    = nullptr;
	bool                blank_     = false;
	unique_ptr<TextBox> text_box_;
};
} // namespace slade
