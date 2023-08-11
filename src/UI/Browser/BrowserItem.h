#pragma once

#include "BrowserCanvas.h"

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
	BrowserItem(const wxString& name, unsigned index = 0, const wxString& type = "item");
	virtual ~BrowserItem();

	wxString name() const { return name_; }
	unsigned index() const { return index_; }

	virtual bool loadImage();
	void         draw(
				int                     size,
				gl::draw2d::Context&    dc,
				BrowserCanvas::NameType nametype = BrowserCanvas::NameType::Normal,
				BrowserCanvas::ItemView viewtype = BrowserCanvas::ItemView::Normal);
	virtual void     clearImage() {}
	virtual wxString itemInfo() { return ""; }

protected:
	wxString       type_;
	wxString       name_;
	unsigned       index_     = 0;
	unsigned       image_tex_ = 0;
	BrowserWindow* parent_    = nullptr;
	bool           blank_     = false;

	unique_ptr<gl::draw2d::TextBox> text_box_;
};
} // namespace slade
