
#ifndef __PROPS_PANEL_BASE_H__
#define __PROPS_PANEL_BASE_H__

#include "common.h"
#include "UI/WxBasicControls.h"

class MapObject;
class PropsPanelBase : public wxPanel
{
protected:
	vector<MapObject*>	objects;

public:
	PropsPanelBase(wxWindow* parent) : wxPanel(parent, -1) {}
	virtual ~PropsPanelBase() {}

	virtual void openObjects(vector<MapObject*>& objects) {}
	virtual void applyChanges() {}
};

#endif//__PROPS_PANEL_BASE_H__
