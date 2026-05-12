#pragma once

#include "General/Clipboard.h"
#include "SLADEMap/SLADEMapFwd.h"

namespace slade
{
class MapArchClipboardItem : public ClipboardItem
{
public:
	MapArchClipboardItem() : ClipboardItem(Type::MapArchitecture) {}
	~MapArchClipboardItem() override = default;

	void               addLines(const vector<MapLine*>& lines);
	string             info() const;
	vector<MapVertex*> pasteToMap(SLADEMap* map, const Vec2d& position) const;
	void               putLines(vector<MapLine*>& list) const;
	Vec2d              midpoint() const { return midpoint_; }

private:
	vector<unique_ptr<MapVertex>> vertices_;
	vector<unique_ptr<MapSide>>   sides_;
	vector<unique_ptr<MapLine>>   lines_;
	vector<unique_ptr<MapSector>> sectors_;
	Vec2d                         midpoint_;
};

class MapThingsClipboardItem : public ClipboardItem
{
public:
	MapThingsClipboardItem() : ClipboardItem(Type::MapThings) {}
	~MapThingsClipboardItem() override = default;

	void   addThings(const vector<MapThing*>& things);
	string info() const;
	void   pasteToMap(SLADEMap* map, const Vec2d& position) const;
	void   putThings(vector<MapThing*>& list) const;
	Vec2d  midpoint() const { return midpoint_; }

private:
	vector<unique_ptr<MapThing>> things_;
	Vec2d                        midpoint_;
};
} // namespace slade
