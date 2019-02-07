#pragma once

namespace NodeBuilders
{
struct Builder
{
	wxString         id;
	wxString         name;
	wxString         path;
	wxString         command;
	wxString         exe;
	vector<wxString> options;
	vector<wxString> option_desc;
};

void     init();
void     addBuilderPath(wxString builder, wxString path);
void     saveBuilderPaths(wxFile& file);
unsigned nNodeBuilders();
Builder& builder(wxString id);
Builder& builder(unsigned index);
} // namespace NodeBuilders
