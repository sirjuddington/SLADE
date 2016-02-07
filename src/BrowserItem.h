
#ifndef __BROWSER_ITEM_H__
#define __BROWSER_ITEM_H__

#include "GLTexture.h"

class BrowserWindow;
class TextBox;
class BrowserItem
{
	friend class BrowserWindow;
protected:
	string			type;
	string			name;
	unsigned		index;
	GLTexture*		image;
	BrowserWindow*	parent;
	bool			blank;
	TextBox*		text_box;

public:
	BrowserItem(string name, unsigned index = 0, string type = "item");
	virtual ~BrowserItem();

	string		getName() { return name; }
	unsigned	getIndex() { return index; }

	virtual bool	loadImage();
	void			draw(int size, int x, int y, int font, int nametype = 0, int viewtype = 0, rgba_t colour = COL_WHITE, bool text_shadow = true);
	void			clearImage();
	virtual string	itemInfo() { return ""; }
};

#endif//__BROWSER_ITEM_H__
