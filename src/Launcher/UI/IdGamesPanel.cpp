
#include "Main.h"
#include "IdGamesPanel.h"
#include "UI/Lists/ListView.h"
#include "Launcher/IdGames.h"
#include "Utility/XmlHelpers.h"
#include <wx/xml/xml.h>
#include <wx/sstream.h>
#include <wx/gbsizer.h>


IdGamesDetailsPanel::IdGamesDetailsPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	text_textfile = new wxTextCtrl(
		this,
		-1,
		"",
		wxDefaultPosition,
		wxDefaultSize,
		wxTE_READONLY | wxTE_MULTILINE
	);
	sizer->Add(text_textfile, 1, wxEXPAND, 0);
}

IdGamesDetailsPanel::~IdGamesDetailsPanel()
{
}

void IdGamesDetailsPanel::loadDetails(idGames::File file)
{
	text_textfile->SetValue(file.text_file);
}



IdGamesPanel::IdGamesPanel(wxWindow* parent) : wxPanel(parent)
{
	auto sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	auto vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND | wxALL, 8);

	auto rb_box = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(rb_box, 0, wxEXPAND | wxBOTTOM, 8);

	rb_latest_ = new wxRadioButton(this, -1, "Latest Uploads");
	rb_box->Add(rb_latest_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	rb_search_ = new wxRadioButton(this, -1, "Search idGames");
	rb_box->Add(rb_search_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	rb_browse_ = new wxRadioButton(this, -1, "Browse idGames");
	rb_box->Add(rb_browse_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	btn_refresh_ = new wxButton(this, -1, "Refresh");
	rb_box->AddStretchSpacer(1);
	rb_box->Add(btn_refresh_, 0);

	// Search controls
	panel_search_ = setupSearchControlPanel();
	vbox->Add(panel_search_, 0, wxEXPAND | wxBOTTOM, 8);
	panel_search_->Hide();

	lv_files_ = new ListView(this, -1);
	lv_files_->enableSizeUpdate(false);
	vbox->Add(lv_files_, 1, wxEXPAND);

	lv_files_->AppendColumn("Title");
	lv_files_->AppendColumn("Author");
	lv_files_->AppendColumn("Rating");



	// File info
	panel_details_ = new IdGamesDetailsPanel(this);
	sizer->Add(panel_details_, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 8);


	// Bind events
	Bind(wxEVT_COMMAND_IDGAMES_APICALL_COMPLETED, &IdGamesPanel::onApiCallCompleted, this);
	btn_refresh_->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &IdGamesPanel::onBtnRefreshClicked, this);
	rb_search_->Bind(wxEVT_RADIOBUTTON, &IdGamesPanel::onRBSearchClicked, this);
	rb_browse_->Bind(wxEVT_RADIOBUTTON, &IdGamesPanel::onRBBrowseClicked, this);
	rb_latest_->Bind(wxEVT_RADIOBUTTON, &IdGamesPanel::onRBLatestClicked, this);
	lv_files_->Bind(wxEVT_LIST_ITEM_SELECTED, &IdGamesPanel::onListItemSelected, this);
}

IdGamesPanel::~IdGamesPanel()
{
}

wxPanel* IdGamesPanel::setupSearchControlPanel()
{
	auto panel = new wxPanel(this, -1);
	auto sizer = new wxGridBagSizer(8, 8);
	panel->SetSizer(sizer);

	// Search query
	sizer->Add(new wxStaticText(panel, -1, "Search for"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);
	text_search_ = new wxTextCtrl(panel, -1);
	hbox->Add(text_search_, 1, wxEXPAND | wxRIGHT, 8);

	// Search type
	string search_types[] = {
		"Filename",
		"Title",
		"Author",
		"Email",
		"Description",
		"Credits",
		"Editors Used",
		"Text File"
	};
	choice_search_by_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 8, search_types);
	choice_search_by_->SetSelection(0);
	hbox->Add(new wxStaticText(panel, -1, "in"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	hbox->Add(choice_search_by_, 1, wxEXPAND);

	// Search sort
	string sort_types[] = {
		"Date",
		"Filename",
		"Size",
		"Rating"
	};
	sizer->Add(new wxStaticText(panel, -1, "Sort by"), wxGBPosition(1, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxGBPosition(1, 1), wxDefaultSpan, wxEXPAND);
	choice_search_sort_ = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 4, sort_types);
	choice_search_sort_->SetSelection(0);
	hbox->Add(choice_search_sort_, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	// Sort dir
	btn_search_sort_dir_ = new wxButton(panel, -1, "Ascending");
	//btn_search_sort_dir->SetImageLabel(Icons::getIcon(Icons::GENERAL, "up"));
	hbox->Add(btn_search_sort_dir_, 0, wxEXPAND);

	sizer->AddGrowableCol(1);

	return panel;
}

void IdGamesPanel::loadList(vector<idGames::File>& list)
{
	lv_files_->Show(false);
	lv_files_->DeleteAllItems();
	for (auto& file : list)
	{
		wxArrayString item;
		item.Add(file.title);
		item.Add(file.author);
		item.Add(S_FMT("%f", file.rating));
		lv_files_->addItem(lv_files_->GetItemCount(), item);
	}
	lv_files_->Show();
}

void IdGamesPanel::getLatestFiles()
{
	vector<key_value_t> params;
	params.push_back(key_value_t("limit", "200"));
	auto call = new idGames::ApiCall(this, "latestfiles", params);
	call->Create();
	call->Run();
}

void IdGamesPanel::readLatestFiles(string& xml)
{
	wxXmlDocument doc;
	wxStringInputStream stream(xml);
	doc.Load(stream);

	// Check root node is <idgames-response>
	if (!doc.GetRoot() || doc.GetRoot()->GetName() != "idgames-response")
		return;

	// Get content node
	auto content = XmlHelpers::getFirstChild(doc.GetRoot(), "content");
	if (!content)
		return;

	files_latest_.clear();

	// Go through <file> nodes
	auto file = content->GetChildren();
	while (file)
	{
		// Check it's a <file> node
		if (file->GetName() != "file")
		{
			file = file->GetNext();
			continue;
		}

		//// Read into idGames file
		//idGames::File id_file;
		//idGames::readFileXml(id_file, file);

		//// Add to list
		//wxArrayString item;
		//item.Add(id_file.title);
		//item.Add(id_file.author);
		//item.Add(S_FMT("%f", id_file.rating));
		//lv_files->addItem(lv_files->GetItemCount(), item);

		// Add file to list
		files_latest_.push_back(idGames::File());
		idGames::readFileXml(files_latest_.back(), file);

		file = file->GetNext();
	}

	// Load latest files list
	loadList(files_latest_);
}

void IdGamesPanel::searchFiles()
{
	// Check query
	string query = text_search_->GetValue();
	if (query.Length() < 3)
	{
		wxMessageBox("Search query must contain at least 3 characters", "Search Query Too Short", wxOK | wxICON_EXCLAMATION);
		return;
	}

	// Query parameter
	vector<key_value_t> params;
	params.push_back(key_value_t("query", text_search_->GetValue()));

	// Type parameter
	switch (choice_search_by_->GetSelection())
	{
	case 0:
		params.push_back(key_value_t("type", "filename")); break;
	case 1:
		params.push_back(key_value_t("type", "title")); break;
	case 2:
		params.push_back(key_value_t("type", "author")); break;
	case 3:
		params.push_back(key_value_t("type", "email")); break;
	case 4:
		params.push_back(key_value_t("type", "description")); break;
	case 5:
		params.push_back(key_value_t("type", "credits")); break;
	case 6:
		params.push_back(key_value_t("type", "editors")); break;
	case 7:
		params.push_back(key_value_t("type", "textfile")); break;
	default:
		break;
	}
	
	// Sort parameter
	switch (choice_search_sort_->GetSelection())
	{
	case 0:
		params.push_back(key_value_t("sort", "date")); break;
	case 1:
		params.push_back(key_value_t("sort", "filename")); break;
	case 2:
		params.push_back(key_value_t("sort", "size")); break;
	case 3:
		params.push_back(key_value_t("sort", "rating")); break;
	default:
		break;
	}

	// Dir parameter
	if (btn_search_sort_dir_->GetLabelText().StartsWith("A"))
		params.push_back(key_value_t("dir", "asc"));
	else
		params.push_back(key_value_t("dir", "desc"));

	// Call API
	auto call = new idGames::ApiCall(this, "search", params);
	call->Create();
	call->Run();
}

void IdGamesPanel::readSearchResult(string& xml)
{
	wxXmlDocument doc;
	wxStringInputStream stream(xml);
	doc.Load(stream);

	// Check root node is <idgames-response>
	if (!doc.GetRoot() || doc.GetRoot()->GetName() != "idgames-response")
		return;

	// Get content node
	auto content = XmlHelpers::getFirstChild(doc.GetRoot(), "content");
	if (!content)
		return;

	files_search_.clear();

	// Go through <file> nodes
	auto file = content->GetChildren();
	while (file)
	{
		// Check it's a <file> node
		if (file->GetName() != "file")
		{
			file = file->GetNext();
			continue;
		}

		//// Read into idGames file
		//idGames::File id_file;
		//idGames::readFileXml(id_file, file);

		//// Add to list
		//wxArrayString item;
		//item.Add(id_file.title);
		//item.Add(id_file.author);
		//item.Add(S_FMT("%f", id_file.rating));
		//lv_files->addItem(lv_files->GetItemCount(), item);

		// Add file to list
		files_search_.push_back(idGames::File());
		idGames::readFileXml(files_search_.back(), file);

		file = file->GetNext();
	}

	// Load search result list
	loadList(files_search_);
}

void IdGamesPanel::onApiCallCompleted(wxThreadEvent& e)
{
	// Re-enable controls
	rb_search_->Enable(true);
	rb_browse_->Enable(true);
	rb_latest_->Enable(true);
	btn_refresh_->Enable(true);

	// Check call completed successfully
	if (e.GetString() == "connect_failed" || e.GetString().IsEmpty())
		return;

	// Get command and response
	string response;
	string command = e.GetString().BeforeFirst(':', &response);
	LOG_MESSAGE(3, "command: " + command);

	// Latest files
	if (command == "latestfiles")
		readLatestFiles(response);

	// Search
	else if (command == "search")
		readSearchResult(response);
}

void IdGamesPanel::onBtnRefreshClicked(wxCommandEvent& e)
{
	// Disable controls
	rb_search_->Enable(false);
	rb_browse_->Enable(false);
	rb_latest_->Enable(false);
	btn_refresh_->Enable(false);

	// Latest files
	if (rb_latest_->GetValue())
		getLatestFiles();

	// Search
	else if (rb_search_->GetValue())
		searchFiles();
}

void IdGamesPanel::onRBSearchClicked(wxCommandEvent& e)
{
	// Update UI
	panel_search_->Show(true);
	btn_refresh_->SetLabel("Search");
	Layout();
	Refresh();

	// Load search result list
	loadList(files_search_);
}

void IdGamesPanel::onRBBrowseClicked(wxCommandEvent& e)
{
	// Update UI
	panel_search_->Show(false);
	Layout();
	Refresh();

	// Load browse list
	loadList(files_browse_);
}

void IdGamesPanel::onRBLatestClicked(wxCommandEvent& e)
{
	// Update UI
	panel_search_->Show(false);
	btn_refresh_->SetLabel("Refresh");
	Layout();
	Refresh();

	// Load latest files list
	loadList(files_latest_);
}

void IdGamesPanel::onListItemSelected(wxListEvent& e)
{
	int selection = lv_files_->selectedItems()[0];

	if (rb_latest_->GetValue())
		panel_details_->loadDetails(files_latest_[selection]);
}
