#pragma once

#include "BrowserCanvas.h"
#include "OpenGL/Drawing.h"

class BrowserWindow;

class BrowserItem
{
	friend class BrowserWindow;

public:
	BrowserItem(const wxString& name, unsigned index = 0, const wxString& type = "item");
	virtual ~BrowserItem() = default;

	wxString name() const { return name_; }
	unsigned index() const { return index_; }

	virtual bool loadImage();
	void         draw(
				int                     size,
				int                     x,
				int                     y,
				Drawing::Font           font,
				BrowserCanvas::NameType nametype    = BrowserCanvas::NameType::Normal,
				BrowserCanvas::ItemView viewtype    = BrowserCanvas::ItemView::Normal,
				const ColRGBA& colour      = ColRGBA::WHITE,
				bool                    text_shadow = true);
	virtual void     clearImage() {}
	virtual wxString itemInfo() { return ""; }

	typedef std::unique_ptr<BrowserItem> UPtr;

protected:
	wxString                 type_;
	wxString                 name_;
	unsigned                 index_     = 0;
	unsigned                 image_tex_ = 0;
	BrowserWindow*           parent_    = nullptr;
	bool                     blank_     = false;
	std::unique_ptr<TextBox> text_box_;
};
