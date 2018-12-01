#pragma once

#include "BrowserCanvas.h"
#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"

class BrowserWindow;
class TextBox;
class BrowserCanvas;

class BrowserItem
{
	friend class BrowserWindow;

public:
	BrowserItem(string name, unsigned index = 0, string type = "item");
	virtual ~BrowserItem();

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
				ColRGBA                  colour      = COL_WHITE,
				bool                    text_shadow = true);
	void           clearImage();
	virtual string itemInfo() { return ""; }

protected:
	string         type_;
	string         name_;
	unsigned       index_    = 0;
	GLTexture*     image_    = nullptr;
	BrowserWindow* parent_   = nullptr;
	bool           blank_    = false;
	TextBox*       text_box_ = nullptr;
};
