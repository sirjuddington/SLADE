#pragma once

#include "BrowserCanvas.h"
#include "OpenGL/Drawing.h"

namespace slade
{
class BrowserWindow;

class BrowserItem
{
	friend class BrowserWindow;

public:
	BrowserItem(const string& name, unsigned index = 0, const string& type = "item");
	virtual ~BrowserItem() = default;

	string   name() const { return name_; }
	unsigned index() const { return index_; }

	virtual bool loadImage();
	void         draw(
				int                     size,
				int                     x,
				int                     y,
				drawing::Font           font,
				BrowserCanvas::NameType nametype    = BrowserCanvas::NameType::Normal,
				BrowserCanvas::ItemView viewtype    = BrowserCanvas::ItemView::Normal,
				const ColRGBA&          colour      = ColRGBA::WHITE,
				bool                    text_shadow = true);
	virtual void   clearImage() {}
	virtual string itemInfo() { return ""; }

protected:
	string              type_;
	string              name_;
	unsigned            index_     = 0;
	unsigned            image_tex_ = 0;
	BrowserWindow*      parent_    = nullptr;
	bool                blank_     = false;
	unique_ptr<TextBox> text_box_;
};
} // namespace slade
