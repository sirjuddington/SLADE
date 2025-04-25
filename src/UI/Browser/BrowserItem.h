#pragma once

#include "Browser.h"

namespace slade
{
class BrowserWindow;
namespace gl::draw2d
{
	struct Context;
	class TextBox;
} // namespace gl::draw2d

class BrowserItem
{
	friend class BrowserWindow;

public:
	BrowserItem(const string& name, unsigned index = 0, const string& type = "item");
	virtual ~BrowserItem();

	string   name() const { return name_; }
	unsigned index() const { return index_; }

	virtual bool loadImage();
	void         draw(
				int                  size,
				gl::draw2d::Context& dc,
				browser::NameType    nametype = browser::NameType::Normal,
				browser::ItemView    viewtype = browser::ItemView::Normal);
	virtual void   clearImage() {}
	virtual string itemInfo() { return ""; }

protected:
	string         type_;
	string         name_;
	unsigned       index_     = 0;
	unsigned       image_tex_ = 0;
	BrowserWindow* parent_    = nullptr;
	bool           blank_     = false;

	unique_ptr<gl::draw2d::TextBox> text_box_;
};
} // namespace slade
