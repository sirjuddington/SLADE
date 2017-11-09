#pragma once

#include "OpenGL/GLTexture.h"

class BrowserWindow;
class TextBox;
class BrowserItem
{
	friend class BrowserWindow;
public:
	BrowserItem(string name, unsigned index = 0, string type = "item");
	virtual ~BrowserItem();

	string		name() const { return name_; }
	unsigned	index() const { return index_; }

	virtual bool	loadImage();
	void			draw(
						int size,
						int x,
						int y,
						int font,
						int nametype = 0,
						int viewtype = 0,
						rgba_t colour = COL_WHITE,
						bool text_shadow = true
					);
	void			clearImage();
	virtual string	itemInfo() { return ""; }

protected:
	string			type_;
	string			name_;
	unsigned		index_		= 0;
	GLTexture*		image_		= nullptr;
	BrowserWindow*	parent_		= nullptr;
	bool			blank_		= false;
	TextBox*		text_box_	= nullptr;
};
