
#include "Main.h"
#include "IdGamesPanel.h"
#include "UI/Lists/ListView.h"
#include "Launcher/IdGames.h"
#include "Utility/XmlHelpers.h"
#include "UI/STabCtrl.h"
#include "Graphics/Icons.h"
#include <wx/xml/xml.h>
#include <wx/sstream.h>
#include <wx/gbsizer.h>


IdGamesDetailsPanel::IdGamesDetailsPanel(wxWindow* parent) : wxPanel(parent, -1)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	text_textfile = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, wxTE_READONLY|wxTE_MULTILINE);
	sizer->Add(text_textfile, 1, wxEXPAND, 0);
}

IdGamesDetailsPanel::~IdGamesDetailsPanel()
{
}

void IdGamesDetailsPanel::loadDetails(idGames::File file)
{
	text_textfile->SetValue(file.text_file);
}



IdGamesPanel::IdGamesPanel(wxWindow* parent)
	: wxPanel(parent)
{
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	sizer->Add(vbox, 1, wxEXPAND | wxALL, 8);

	wxBoxSizer* rb_box = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(rb_box, 0, wxEXPAND | wxBOTTOM, 8);

	rb_latest = new wxRadioButton(this, -1, "Latest Uploads");
	rb_box->Add(rb_latest, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	rb_search = new wxRadioButton(this, -1, "Search idGames");
	rb_box->Add(rb_search, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	rb_browse = new wxRadioButton(this, -1, "Browse idGames");
	rb_box->Add(rb_browse, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	btn_refresh = new wxButton(this, -1, "Refresh");
	rb_box->AddStretchSpacer(1);
	rb_box->Add(btn_refresh, 0);

	// Search controls
	panel_search = setupSearchControlPanel();
	vbox->Add(panel_search, 0, wxEXPAND | wxBOTTOM, 8);
	panel_search->Hide();

	lv_files = new ListView(this, -1);
	lv_files->enableSizeUpdate(false);
	vbox->Add(lv_files, 1, wxEXPAND);

	lv_files->AppendColumn("Title");
	lv_files->AppendColumn("Author");
	lv_files->AppendColumn("Rating");



	// File info
	panel_details = new IdGamesDetailsPanel(this);
	sizer->Add(panel_details, 1, wxEXPAND | wxTOP | wxRIGHT | wxBOTTOM, 8);


	// Bind events
	Bind(wxEVT_COMMAND_IDGAMES_APICALL_COMPLETED, &IdGamesPanel::onApiCallCompleted, this);
	btn_refresh->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &IdGamesPanel::onBtnRefreshClicked, this);
	rb_search->Bind(wxEVT_RADIOBUTTON, &IdGamesPanel::onRBSearchClicked, this);
	rb_browse->Bind(wxEVT_RADIOBUTTON, &IdGamesPanel::onRBBrowseClicked, this);
	rb_latest->Bind(wxEVT_RADIOBUTTON, &IdGamesPanel::onRBLatestClicked, this);
	lv_files->Bind(wxEVT_LIST_ITEM_SELECTED, &IdGamesPanel::onListItemSelected, this);
}

IdGamesPanel::~IdGamesPanel()
{
}

wxPanel* IdGamesPanel::setupSearchControlPanel()
{
	wxPanel* panel = new wxPanel(this, -1);
	wxGridBagSizer* sizer = new wxGridBagSizer(8, 8);
	panel->SetSizer(sizer);

	// Search query
	sizer->Add(new wxStaticText(panel, -1, "Search for"), wxGBPosition(0, 0), wxDefaultSpan, wxALIGN_CENTER_VERTICAL);
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	sizer->Add(hbox, wxGBPosition(0, 1), wxDefaultSpan, wxEXPAND);
	text_search = new wxTextCtrl(panel, -1);
	hbox->Add(text_search, 1, wxEXPAND | wxRIGHT, 8);

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
	choice_search_by = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 8, search_types);
	choice_search_by->SetSelection(0);
	hbox->Add(new wxStaticText(panel, -1, "in"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
	hbox->Add(choice_search_by, 1, wxEXPAND);

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
	choice_search_sort = new wxChoice(panel, -1, wxDefaultPosition, wxDefaultSize, 4, sort_types);
	choice_search_sort->SetSelection(0);
	hbox->Add(choice_search_sort, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

	// Sort dir
	btn_search_sort_dir = new wxButton(panel, -1, "Ascending");
	//btn_search_sort_dir->SetImageLabel(Icons::getIcon(Icons::GENERAL, "up"));
	hbox->Add(btn_search_sort_dir, 0, wxEXPAND);

	sizer->AddGrowableCol(1);

	return panel;
}

void IdGamesPanel::loadList(vector<idGames::File>& list)
{
	lv_files->Show(false);
	lv_files->DeleteAllItems();
	for (unsigned a = 0; a < list.size(); a++)
	{
		wxArrayString item;
		item.Add(list[a].title);
		item.Add(list[a].author);
		item.Add(S_FMT("%f", list[a].rating));
		lv_files->addItem(lv_files->GetItemCount(), item);
	}
	lv_files->Show();
}

void IdGamesPanel::getLatestFiles()
{
	vector<key_value_t> params;
	params.push_back(key_value_t("limit", "200"));
	idGames::ApiCall* call = new idGames::ApiCall(this, "latestfiles", params);
	call->Create();
	call->Run();
}

void IdGamesPanel::readLatestFiles(string& xml)
{
	wxXmlDocument doc;
	doc.Load(wxStringInputStream(xml));

	// Check root node is <idgames-response>
	if (!doc.GetRoot() || doc.GetRoot()->GetName() != "idgames-response")
		return;

	// Get content node
	wxXmlNode* content = XmlHelpers::getFirstChild(doc.GetRoot(), "content");
	if (!content)
		return;

	files_latest.clear();

	// Go through <file> nodes
	wxXmlNode* file = content->GetChildren();
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
		files_latest.push_back(idGames::File());
		idGames::readFileXml(files_latest.back(), file);

		file = file->GetNext();
	}

	// Load latest files list
	loadList(files_latest);
}

void IdGamesPanel::searchFiles()
{
	// Check query
	string query = text_search->GetValue();
	if (query.Length() < 3)
	{
		wxMessageBox("Search query must contain at least 3 characters", "Search Query Too Short", wxOK | wxICON_EXCLAMATION);
		return;
	}

	// Query parameter
	vector<key_value_t> params;
	params.push_back(key_value_t("query", text_search->GetValue()));

	// Type parameter
	switch (choice_search_by->GetSelection())
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
	switch (choice_search_sort->GetSelection())
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
	if (btn_search_sort_dir->GetLabelText().StartsWith("A"))
		params.push_back(key_value_t("dir", "asc"));
	else
		params.push_back(key_value_t("dir", "desc"));

	// Call API
	idGames::ApiCall* call = new idGames::ApiCall(this, "search", params);
	call->Create();
	call->Run();
}

void IdGamesPanel::readSearchResult(string& xml)
{
	wxXmlDocument doc;
	doc.Load(wxStringInputStream(xml));

	// Check root node is <idgames-response>
	if (!doc.GetRoot() || doc.GetRoot()->GetName() != "idgames-response")
		return;

	// Get content node
	wxXmlNode* content = XmlHelpers::getFirstChild(doc.GetRoot(), "content");
	if (!content)
		return;

	files_search.clear();

	// Go through <file> nodes
	wxXmlNode* file = content->GetChildren();
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
		files_search.push_back(idGames::File());
		idGames::readFileXml(files_search.back(), file);

		file = file->GetNext();
	}

	// Load search result list
	loadList(files_search);
}

void IdGamesPanel::onApiCallCompleted(wxThreadEvent& e)
{
	// Re-enable controls
	rb_search->Enable(true);
	rb_browse->Enable(true);
	rb_latest->Enable(true);
	btn_refresh->Enable(true);

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
	rb_search->Enable(false);
	rb_browse->Enable(false);
	rb_latest->Enable(false);
	btn_refresh->Enable(false);

	// Latest files
	if (rb_latest->GetValue())
		getLatestFiles();

	// Search
	else if (rb_search->GetValue())
		searchFiles();
}

void IdGamesPanel::onRBSearchClicked(wxCommandEvent& e)
{
	// Update UI
	panel_search->Show(true);
	btn_refresh->SetLabel("Search");
	Layout();
	Refresh();

	// Load search result list
	loadList(files_search);
}

void IdGamesPanel::onRBBrowseClicked(wxCommandEvent& e)
{
	// Update UI
	panel_search->Show(false);
	Layout();
	Refresh();

	// Load browse list
	loadList(files_browse);
}

void IdGamesPanel::onRBLatestClicked(wxCommandEvent& e)
{
	// Update UI
	panel_search->Show(false);
	btn_refresh->SetLabel("Refresh");
	Layout();
	Refresh();

	// Load latest files list
	loadList(files_latest);
}

void IdGamesPanel::onListItemSelected(wxListEvent& e)
{
	int selection = lv_files->selectedItems()[0];

	if (rb_latest->GetValue())
		panel_details->loadDetails(files_latest[selection]);
}
