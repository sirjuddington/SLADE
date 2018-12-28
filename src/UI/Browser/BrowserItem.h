#pragma once

#include "BrowserCanvas.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"

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
				Drawing::Font           font,
				BrowserCanvas::NameType nametype    = BrowserCanvas::NameType::Normal,
				BrowserCanvas::ItemView viewtype    = BrowserCanvas::ItemView::Normal,
				ColRGBA                 colour      = COL_WHITE,
				bool                    text_shadow = true);
	void           clearImage() const;
	virtual string itemInfo() { return ""; }

	typedef std::unique_ptr<BrowserItem> UPtr;

protected:
	string                   type_;
	string                   name_;
	unsigned                 index_  = 0;
	GLTexture*               image_  = nullptr;
	BrowserWindow*           parent_ = nullptr;
	bool                     blank_  = false;
	std::unique_ptr<TextBox> text_box_;
};
