
#include "Main.h"
#include "WxStuff.h"
#include "ThingTypeBrowser.h"
#include "MapEditorWindow.h"
#include "GameConfiguration.h"
#include "Drawing.h"


CVAR(Bool, browser_thing_tiles, true, CVAR_SAVE)

ThingBrowserItem::ThingBrowserItem(string name, ThingType* type, unsigned index) : BrowserItem(name, index) {
	// Init variables
	this->type = type;
}

ThingBrowserItem::~ThingBrowserItem() {
}

bool ThingBrowserItem::loadImage() {
	// Get sprite
	GLTexture* tex = theMapEditor->textureManager().getSprite(type->getSprite(), type->getTranslation(), type->getPalette());
	if (!tex) {
		// Sprite not found, try an icon
		tex = theMapEditor->textureManager().getEditorImage(S_FMT("thing/%s", CHR(type->getIcon())));
	}
	if (!tex) {
		// Icon not found either, use unknown icon
		tex = theMapEditor->textureManager().getEditorImage("thing/unknown");
	}

	if (tex) {
		image = tex;
		return true;
	}
	else
		return false;
}



ThingTypeBrowser::ThingTypeBrowser(wxWindow* parent, int type) : BrowserWindow(parent) {
	// Set window title
	SetTitle("Browse Thing Types");

	// Add 'Details view' checkbox
	cb_view_tiles = new wxCheckBox(this, -1, "Details view");
	cb_view_tiles->SetValue(browser_thing_tiles);
	sizer_bottom->Add(cb_view_tiles, 0, wxEXPAND|wxRIGHT, 4);

	// Populate tree
	vector<tt_t> types = theGameConfiguration->allThingTypes();
	for (unsigned a = 0; a < types.size(); a++)
		addItem(new ThingBrowserItem(types[a].type->getName(), types[a].type, types[a].number), types[a].type->getGroup());
	populateItemTree();

	// Set browser options
	canvas->setItemNameType(BrowserCanvas::NAMES_INDEX);
	setupViewOptions();

	// Select initial item if any
	if (type >= 0)
		selectItem(theGameConfiguration->thingType(type)->getName());
	else
		openTree(items_root);	// Otherwise open 'all' category


	// Bind events
	cb_view_tiles->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &ThingTypeBrowser::onViewTilesClicked, this);

	Layout();
}

ThingTypeBrowser::~ThingTypeBrowser() {
}

void ThingTypeBrowser::setupViewOptions() {
	if (browser_thing_tiles) {
		setFont(Drawing::FONT_CONDENSED);
		setItemSize(48);
		setItemViewType(BrowserCanvas::ITEMS_TILES);
	}
	else {
		setFont(Drawing::FONT_BOLD);
		setItemSize(80);
		setItemViewType(BrowserCanvas::ITEMS_NORMAL);
	}

	canvas->updateScrollBar();
	canvas->updateLayout();
	canvas->showSelectedItem();
}

int ThingTypeBrowser::getSelectedType() {
	BrowserItem* selected = getSelectedItem();
	if (selected) {
		wxLogMessage("Selected item %d", selected->getIndex());
		return selected->getIndex();
	}
	else
		return -1;
}


void ThingTypeBrowser::onViewTilesClicked(wxCommandEvent& e) {
	browser_thing_tiles = cb_view_tiles->GetValue();
	setupViewOptions();
	Refresh();
}
