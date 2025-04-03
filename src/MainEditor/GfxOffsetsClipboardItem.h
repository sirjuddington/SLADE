#pragma once

#include "General/Clipboard.h"

namespace slade
{
class GfxOffsetsClipboardItem : public ClipboardItem
{
public:
	GfxOffsetsClipboardItem(const Vec2i& offsets) : ClipboardItem{ Type::GfxOffsets }, offsets_{ offsets } {}

	const Vec2i& offsets() const { return offsets_; }
	void         setOffsets(const Vec2i& offsets) { offsets_ = offsets; }

private:
	Vec2i offsets_;
};
}
