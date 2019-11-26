#pragma once

class MapObject;

class PropsPanelBase : public wxPanel
{
public:
	PropsPanelBase(wxWindow* parent) : wxPanel(parent, -1) {}
	virtual ~PropsPanelBase() = default;

	virtual void openObjects(vector<MapObject*>& objects) {}
	virtual void applyChanges() {}

protected:
	vector<MapObject*> objects_;
};
