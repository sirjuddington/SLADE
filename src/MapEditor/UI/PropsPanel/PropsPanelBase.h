#pragma once

namespace slade
{
class MapObject;

class PropsPanelBase : public wxPanel
{
public:
	PropsPanelBase(wxWindow* parent) : wxPanel(parent, -1) {}
	~PropsPanelBase() override = default;

	virtual void openObjects(vector<MapObject*>& objects) {}
	virtual void applyChanges() {}

protected:
	vector<MapObject*> objects_;
};
} // namespace slade
