
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    TextStyle.cpp
 * Description: Classes which handle font and colour settings for
 *              the text editor. TextStyle contains the actual font
 *              and colour settings for a particular 'style' (eg.
 *              keywords, comments, etc). StyleSet is just a set
 *              of these styles that can be loaded to the scintilla
 *              'styles' in the text editor
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "App.h"
#include "TextEditor.h"
#include "TextStyle.h"
#include "Lexer.h"
#include "Archive/ArchiveManager.h"


/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(String, txed_override_font, "", CVAR_SAVE)
CVAR(Int, txed_override_font_size, 0, CVAR_SAVE)
vector<StyleSet*>	style_sets;
StyleSet*			ss_current = NULL;
vector<TextEditor*>	StyleSet::editors;


/*******************************************************************
 * TEXTSTYLE CLASS FUNCTIONS
 *******************************************************************/

/* TextStyle::TextStyle
 * TextStyle class constructor
 *******************************************************************/
TextStyle::TextStyle(string name, string description, int style_id)
{
	// Init variables
	this->name = name;
	this->description = description;
	if (style_id >= 0)
		wx_styles.push_back(style_id);

	// Default (undefined) values
	font = "";
	size = -1;
	fg_defined = false;
	bg_defined = false;
	bold = -1;
	italic = -1;
	underlined = -1;
}

/* TextStyle::~TextStyle
 * TextStyle class destructor
 *******************************************************************/
TextStyle::~TextStyle()
{
}

/* TextStyle::addWxStyleId
 * Adds a wxSTC style id to the list (used for applying style to the
 * wxStyledTextCtrl, in case this style replaces multiple)
 *******************************************************************/
void TextStyle::addWxStyleId(int style)
{
	wx_styles.push_back(style);
}

/* TextStyle::parse
 * Reads text style information from a parse tree
 *******************************************************************/
bool TextStyle::parse(ParseTreeNode* node)
{
	// Check any info was given
	if (!node)
		return false;

	// Go through info nodes
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->getChildPTN(a);
		string name = child->getName();

		// Font name
		if (S_CMPNOCASE(name, "font"))
			font = child->stringValue();

		// Font size
		if (S_CMPNOCASE(name, "size"))
			size = child->intValue();

		// Foreground colour
		if (S_CMPNOCASE(name, "foreground"))
		{
			foreground.set(child->intValue(0), child->intValue(1), child->intValue(2), 255);
			fg_defined = true;
		}

		// Background colour
		if (S_CMPNOCASE(name, "background"))
		{
			background.set(child->intValue(0), child->intValue(1), child->intValue(2), 255);
			bg_defined = true;
		}

		// Bold
		if (S_CMPNOCASE(name, "bold"))
			bold = (int)child->boolValue();

		// Italic
		if (S_CMPNOCASE(name, "italic"))
			italic = (int)child->boolValue();

		// Underlined
		if (S_CMPNOCASE(name, "underlined"))
			underlined = (int)child->boolValue();
	}

	return true;
}

/* TextStyle::applyTo
 * Applies the style settings to the scintilla text control [stc]
 *******************************************************************/
void TextStyle::applyTo(TextEditor* stc)
{
	for (unsigned a = 0; a < wx_styles.size(); a++)
	{
		// Set font face
		if (txed_override_font != "")
			stc->StyleSetFaceName(wx_styles[a], txed_override_font);
		else if (!font.IsEmpty())
			stc->StyleSetFaceName(wx_styles[a], font);

		// Set font size
		if (txed_override_font_size > 0)
			stc->StyleSetSize(wx_styles[a], txed_override_font_size);
		else if (size > 0)
			stc->StyleSetSize(wx_styles[a], size);

		// Set foreground
		if (fg_defined)
			stc->StyleSetForeground(wx_styles[a], WXCOL(foreground));

		// Set background
		if (bg_defined)
			stc->StyleSetBackground(wx_styles[a], WXCOL(background));

		// Set bold
		if (bold > 0)
			stc->StyleSetBold(wx_styles[a], true);
		else if (bold == 0)
			stc->StyleSetBold(wx_styles[a], false);

		// Set italic
		if (italic > 0)
			stc->StyleSetItalic(wx_styles[a], true);
		else if (italic == 0)
			stc->StyleSetItalic(wx_styles[a], false);

		// Set underlined
		if (underlined > 0)
			stc->StyleSetUnderline(wx_styles[a], true);
		else if (underlined == 0)
			stc->StyleSetUnderline(wx_styles[a], false);
	}
}

/* TextStyle::copyStyle
 * Copies style info from [copy]
 *******************************************************************/
bool TextStyle::copyStyle(TextStyle* copy)
{
	if (!copy)
		return false;

	// Copy data straight
	font = copy->font;
	size = copy->size;
	foreground = copy->foreground;
	fg_defined = copy->fg_defined;
	background = copy->background;
	bg_defined = copy->bg_defined;
	bold = copy->bold;
	italic = copy->italic;
	underlined = copy->underlined;

	return true;
}

/* TextStyle::getDefinition
 * Returns a formatted string defining this style
 *******************************************************************/
string TextStyle::getDefinition(unsigned tabs)
{
	string ret = "";

	// Write font
	if (!font.IsEmpty())
	{
		for (unsigned t = 0; t < tabs; t++) ret += "\t";
		ret += S_FMT("font = \"%s\";\n", font);
	}

	// Write size
	if (size >= 0)
	{
		for (unsigned t = 0; t < tabs; t++) ret += "\t";
		ret += S_FMT("size = %d;\n", size);
	}

	// Write foreground
	if (fg_defined)
	{
		for (unsigned t = 0; t < tabs; t++) ret += "\t";
		ret += S_FMT("foreground = %d, %d, %d;\n", foreground.r, foreground.g, foreground.b);
	}

	// Write background
	if (bg_defined)
	{
		for (unsigned t = 0; t < tabs; t++) ret += "\t";
		ret += S_FMT("background = %d, %d, %d;\n", background.r, background.g, background.b);
	}

	// Write bold
	if (bold >= 0)
	{
		for (unsigned t = 0; t < tabs; t++) ret += "\t";
		ret += S_FMT("bold = %d;\n", bold);
	}

	// Write italic
	if (italic >= 0)
	{
		for (unsigned t = 0; t < tabs; t++) ret += "\t";
		ret += S_FMT("italic = %d;\n", italic);
	}

	// Write underlined
	if (underlined >= 0)
	{
		for (unsigned t = 0; t < tabs; t++) ret += "\t";
		ret += S_FMT("underlined = %d;\n", underlined);
	}

	return ret;
}


/*******************************************************************
 * STYLESET CLASS FUNCTIONS
 *******************************************************************/

/* StyleSet::StyleSet
 * StyleSet class constructor
 *******************************************************************/
StyleSet::StyleSet(string name) : ts_default("default", "Default", wxSTC_STYLE_DEFAULT), ts_selection("selection", "Selected Text")
{
	// Init default style
	wxFont f(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	ts_default.font = f.GetFaceName();
	ts_default.size = 10;
	ts_default.foreground.set(0, 0, 0, 255);
	ts_default.fg_defined = true;
	ts_default.background.set(255, 255, 255, 255);
	ts_default.bg_defined = true;
	ts_default.bold = 0;
	ts_default.italic = 0;
	ts_default.underlined = 0;
	ts_selection.bg_defined = true;
	ts_selection.background.set(150, 150, 150, 255);
	ts_selection.fg_defined = false;

	// Init name
	this->name = name;

	// Init styles
	styles.push_back(new TextStyle("preprocessor",	"Preprocessor",		Lexer::Style::Preprocessor));
	styles.push_back(new TextStyle("comment",		"Comment",			Lexer::Style::Comment));
	styles.push_back(new TextStyle("comment_doc",	"Comment (Doc)",	Lexer::Style::CommentDoc));
	styles.push_back(new TextStyle("string",		"String",			Lexer::Style::String));
	styles.push_back(new TextStyle("character",		"Character",		Lexer::Style::Char));
	styles.push_back(new TextStyle("keyword",		"Keyword",			Lexer::Style::Keyword));
	styles.push_back(new TextStyle("constant",		"Constant",			Lexer::Style::Constant));
	styles.push_back(new TextStyle("type",			"Type",				Lexer::Style::Type));
	styles.push_back(new TextStyle("property",		"Property",			Lexer::Style::Property));
	styles.push_back(new TextStyle("function",		"Function",			Lexer::Style::Function));
	styles.push_back(new TextStyle("number",		"Number",			Lexer::Style::Number));
	styles.push_back(new TextStyle("operator",		"Operator",			Lexer::Style::Operator));
	styles.push_back(new TextStyle("bracematch",	"Brace Match",		wxSTC_STYLE_BRACELIGHT));
	styles.push_back(new TextStyle("bracebad",		"Brace Mismatch",	wxSTC_STYLE_BRACEBAD));
	styles.push_back(new TextStyle("linenum",		"Line Numbers",		wxSTC_STYLE_LINENUMBER));
	styles.push_back(new TextStyle("calltip",		"Call Tip",			wxSTC_STYLE_CALLTIP));
	styles.push_back(new TextStyle("calltip_hl",	"Call Tip Highlight"));
	styles.push_back(new TextStyle("foldmargin",	"Code Folding Margin"));
	styles.push_back(new TextStyle("guides",		"Indent/Right Margin Guide"));
	styles.push_back(new TextStyle("wordmatch",		"Word Match"));
	styles.push_back(new TextStyle("current_line",	"Current Line"));
}

/* StyleSet::~StyleSet
 * StyleSet class destructor
 *******************************************************************/
StyleSet::~StyleSet()
{
	for (unsigned a = 0; a < styles.size(); a++)
		delete styles[a];
}

/* StyleSet::parseSet
 * Reads style set info from a parse tree
 *******************************************************************/
bool StyleSet::parseSet(ParseTreeNode* root)
{
	if (!root)
		return false;

	// Get name
	auto node = root->getChildPTN("name");
	if (node)
		name = node->stringValue();

	// Parse styles
	ts_default.parse(root->getChildPTN("default"));			// Default style
	ts_selection.parse(root->getChildPTN("selection"));		// Selection style
	for (unsigned a = 0; a < styles.size(); a++)			// Other styles
	{
		if (ParseTreeNode* node = root->getChildPTN(styles[a]->name))
			styles[a]->parse(node);
		else
		{
			if (styles[a]->name == "foldmargin")
			{
				// No 'foldmargin' style defined, copy it from line numbers style
				styles[a]->foreground = getStyleForeground("linenum");
				styles[a]->background = getStyleBackground("linenum");
				styles[a]->fg_defined = true;
				styles[a]->bg_defined = true;
			}
			else if (styles[a]->name == "guides")
			{
				// No 'guides' style defined, use the default foreground colour
				styles[a]->foreground = ts_default.getForeground();
				styles[a]->fg_defined = true;
			}
			else if (styles[a]->name == "type" || styles[a]->name == "property")
			{
				// No 'type' or 'property' style defined, copy it from keyword style
				styles[a]->copyStyle(getStyle("keyword"));
			}
			else if (styles[a]->name == "comment_doc")
			{
				// No 'comment_doc' style defined, copy it from comment style
				styles[a]->copyStyle(getStyle("comment"));
			}
			else if (styles[a]->name == "current_line")
			{
				// No 'currentline' style defined, use the default background and darken/lighten it a little
				int fgm = -20;
				int bgm = -10;
				if (ts_default.background.greyscale().r < 100)
				{
					fgm = 30;
					bgm = 15;
				}
				styles[a]->foreground = ts_default.getBackground().amp(fgm, fgm, fgm, 0);
				styles[a]->fg_defined = true;
				styles[a]->background = ts_default.getBackground().amp(bgm, bgm, bgm, 0);
				styles[a]->bg_defined = true;
			}
		}
	}

	return true;
}

/* StyleSet::applyTo
 * Applies all the styles in this set to the text styles in scintilla
 * text control [stc]
 *******************************************************************/
void StyleSet::applyTo(TextEditor* stc)
{
	// Set default style
	ts_default.applyTo(stc);

	// Apply default style to all
	stc->StyleClearAll();

	// Apply other styles
	for (unsigned a = 0; a < styles.size(); a++)
		styles[a]->applyTo(stc);

	// Set selection background if customised
	if (ts_selection.hasBackground())
		stc->SetSelBackground(true, WXCOL(ts_selection.background));
	else
		stc->SetSelBackground(false, wxColour("red"));

	// Set selection foreground if customised
	if (ts_selection.hasForeground())
		stc->SetSelForeground(true, WXCOL(ts_selection.foreground));
	else
		stc->SetSelForeground(false, wxColour("red"));

	// Set caret colour to text foreground colour
	stc->SetCaretForeground(WXCOL(ts_default.foreground));

	// Update code folding margin
	stc->setupFoldMargin(getStyle("foldmargin"));

	// Set indent and right margin line colour
	stc->SetEdgeColour(WXCOL(getStyle("guides")->getForeground()));
	stc->StyleSetBackground(wxSTC_STYLE_INDENTGUIDE, WXCOL(getStyleBackground("guides")));
	stc->StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, WXCOL(getStyleForeground("guides")));

	// Set word match indicator colour
	stc->SetIndicatorCurrent(8);
	stc->IndicatorSetForeground(8, WXCOL(getStyleForeground("wordmatch")));

	// Set current line colour
	stc->MarkerDefine(
		1,
		wxSTC_MARK_BACKGROUND,
		WXCOL(getStyleBackground("current_line")),
		WXCOL(getStyleBackground("current_line"))
	);
	stc->MarkerDefine(
		2,
		wxSTC_MARK_UNDERLINE,
		WXCOL(getStyleForeground("current_line")),
		WXCOL(getStyleForeground("current_line"))
	);
}

/* StyleSet::copySet
 * Copies all styles in [copy] to this set
 *******************************************************************/
bool StyleSet::copySet(StyleSet* copy)
{
	if (!copy)
		return false;

	// Copy all styles
	ts_default.copyStyle(&(copy->ts_default));
	ts_selection.copyStyle(&(copy->ts_selection));
	for (unsigned a = 0; a < copy->styles.size(); a++)
		styles[a]->copyStyle(copy->styles[a]);

	return true;
}

/* StyleSet::getStyle
 * Returns the text style associated with [name] (these are hard
 * coded), or NULL if [name] was invalid
 *******************************************************************/
TextStyle* StyleSet::getStyle(string name)
{
	// Return style matching name given
	if (S_CMPNOCASE(name, "default"))
		return &ts_default;
	else if (S_CMPNOCASE(name, "selection"))
		return &ts_selection;
	else
	{
		for (unsigned a = 0; a < styles.size(); a++)
		{
			if (styles[a]->name == name)
				return styles[a];
		}
	}

	// Not a valid style
	return NULL;
}

/* StyleSet::getStyle
 * Returns the extra text style at [index]
 *******************************************************************/
TextStyle* StyleSet::getStyle(unsigned index)
{
	if (index < styles.size())
		return styles[index];
	else
		return NULL;
}

/* StyleSet::writeFile
 * Writes this style set as a text definition to a file [filename]
 *******************************************************************/
bool StyleSet::writeFile(string filename)
{
	// Open file for writing
	wxFile file(filename, wxFile::write);

	if (!file.IsOpened())
		return false;

	// Write opening
	file.Write("styleset {\n");

	// Name
	file.Write(S_FMT("\tname = \"%s\";\n\n", name));

	// Default style
	file.Write("\tdefault {\n");
	file.Write(ts_default.getDefinition(2));
	file.Write("\t}\n\n");

	// Selection style
	file.Write("\tselection {\n");
	file.Write(ts_selection.getDefinition(2));
	file.Write("\t}\n\n");

	// Other styles
	for (unsigned a = 0; a < styles.size(); a++)
	{
		file.Write(S_FMT("\t%s {\n", styles[a]->name));
		file.Write(styles[a]->getDefinition(2));
		file.Write("\t}\n\n");
	}

	// Write end
	file.Write("}\n");

	// Close file
	file.Close();

	return true;
}

/* StyleSet::getStyleForeground
 * Returns the foreground colour of [style], or the default style's
 * foreground colour if it is not set
 *******************************************************************/
rgba_t StyleSet::getStyleForeground(string style)
{
	TextStyle* s = getStyle(style);
	if (s && s->hasForeground())
		return s->getForeground();
	else
		return ts_default.getForeground();
}

/* StyleSet::getStyleBackground
 * Returns the background colour of [style], or the default style's
 * background colour if it is not set
 *******************************************************************/
rgba_t StyleSet::getStyleBackground(string style)
{
	TextStyle* s = getStyle(style);
	if (s && s->hasBackground())
		return s->getBackground();
	else
		return ts_default.getBackground();
}

/* StyleSet::getDefaultFontFace
 * Returns the default style font face
 *******************************************************************/
string StyleSet::getDefaultFontFace()
{
	if (txed_override_font != "")
		return txed_override_font;
	else
		return getStyle("default")->getFontFace();
}

/* StyleSet::getDefaultFontSize
 * Returns the default style font size
 *******************************************************************/
int StyleSet::getDefaultFontSize()
{
	if (txed_override_font != "" && txed_override_font_size > 0)
		return txed_override_font_size;
	else
		return getStyle("default")->getFontSize();
}


/*******************************************************************
 * STYLESET STATIC FUNCTIONS
 *******************************************************************/

/* StyleSet::initCurrent
 * Initialises the 'current' style set from the previously saved
 * 'current.sss' file, or uses the default set if the file does not
 * exist
 *******************************************************************/
void StyleSet::initCurrent()
{
	// Create 'current' styleset
	ss_current = new StyleSet();
	ss_current->name = "<current styleset>";

	// First up, check if "<userdir>/current.sss" exists
	string path = App::path("current.sss", App::Dir::User);
	if (wxFileExists(path))
	{
		// Read it in
		Tokenizer tz;
		tz.openFile(path);

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Find definition
		auto node = root.getChildPTN("styleset");
		if (node)
		{
			// If found, load it into the current set
			ss_current->parseSet(node);
			return;
		}
	}

	// Unable to load from userdir, just load first styleset (should be default)
	if (style_sets.size() > 0)
		ss_current->copySet(style_sets[0]);
}

/* StyleSet::saveCurrent
 * Writes the current style set to the 'current.sss' file
 *******************************************************************/
void StyleSet::saveCurrent()
{
	if (!ss_current)
		return;

	ss_current->writeFile(App::path("current.sss", App::Dir::User));
}

/* StyleSet::currentSet
 * Returns the current style set
 *******************************************************************/
StyleSet* StyleSet::currentSet()
{
	if (!ss_current)
		initCurrent();

	return ss_current;
}

/* StyleSet::loadSet
 * Loads the style set matching [name] to the current style set.
 * Returns false if no match was found, true otherwise
 *******************************************************************/
bool StyleSet::loadSet(string name)
{
	// Search for set matching name
	for (unsigned a = 0; a < style_sets.size(); a++)
	{
		if (S_CMPNOCASE(style_sets[a]->name, name))
		{
			ss_current->copySet(style_sets[a]);
			return true;
		}
	}

	return false;
}

/* StyleSet::loadSet
 * Loads the style set at [index] to the current style set. Returns
 * false if [index] was out of bounds, true otherwise
 *******************************************************************/
bool StyleSet::loadSet(unsigned index)
{
	// Check index
	if (index >= style_sets.size())
		return false;

	currentSet()->copySet(style_sets[index]);

	return true;
}

/* StyleSet::applyCurrent
 * Applies the current style set to the scintilla text control [stc]
 *******************************************************************/
void StyleSet::applyCurrent(TextEditor* stc)
{
	currentSet()->applyTo(stc);
}

/* StyleSet::getName
 * Returns the name of the style set at [index], or an empty string
 * if [index] is out of bounds
 *******************************************************************/
string StyleSet::getName(unsigned index)
{
	// Check index
	if (index >= style_sets.size())
		return "";

	return style_sets[index]->name;
}

/* StyleSet::numSets
 * Returns the number of loaded style sets
 *******************************************************************/
unsigned StyleSet::numSets()
{
	return style_sets.size();
}

/* StyleSet::getSet
 * Returns the style set at [index], or NULL if [index] is out of bounds
 *******************************************************************/
StyleSet* StyleSet::getSet(unsigned index)
{
	// Check index
	if (index >= style_sets.size())
		return NULL;

	return style_sets[index];
}

/* StyleSet::addEditor
 * Adds [stc] to the current list of text editors
 *******************************************************************/
void StyleSet::addEditor(TextEditor* stc)
{
	editors.push_back(stc);
}

/* StyleSet::removeEditor
 * Removes [stc] from the current list of text editors
 *******************************************************************/
void StyleSet::removeEditor(TextEditor* stc)
{
	VECTOR_REMOVE(editors, stc);
}

/* StyleSet::applyCurrentToAll
 * Applies the current style set to all text editors in the list
 *******************************************************************/
void StyleSet::applyCurrentToAll()
{
	for (unsigned a = 0; a < editors.size(); a++)
		applyCurrent(editors[a]);
}

/* StyleSet::loadResourceStyles
 * Loads all text styles from the slade resource archive (slade.pk3)
 *******************************************************************/
bool StyleSet::loadResourceStyles()
{
	// Get 'config/text_styles' directory in slade.pk3
	ArchiveTreeNode* dir = theArchiveManager->programResourceArchive()->getDir("config/text_styles");

	// Check it exists
	if (!dir)
	{
		LOG_MESSAGE(1, "Warning: No 'config/text_styles' directory exists in slade.pk3");
		return false;
	}

	// Read default style set first
	ArchiveEntry* default_style = dir->getEntry("default.sss");
	if (default_style)
	{
		// Read entry data into tokenizer
		Tokenizer tz;
		tz.openMem(&default_style->getMCData(), default_style->getName());

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Read any styleset definitions
		vector<STreeNode*> nodes = root.getChildren("styleset");
		for (unsigned b = 0; b  < nodes.size(); b++)
		{
			StyleSet* newset = new StyleSet();
			if (newset->parseSet((ParseTreeNode*)nodes[b]))
				style_sets.push_back(newset);
			else
				delete newset;
		}
	}

	// Go through all entries within it
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		ArchiveEntry* entry = dir->getEntry(a);

		// Skip default
		if (entry->getName(true) == "default")
			continue;

		// Read entry data into tokenizer
		Tokenizer tz;
		tz.openMem(&entry->getMCData(), entry->getName());

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Read any styleset definitions
		vector<STreeNode*> nodes = root.getChildren("styleset");
		for (unsigned b = 0; b  < nodes.size(); b++)
		{
			StyleSet* newset = new StyleSet();
			if (newset->parseSet((ParseTreeNode*)nodes[b]))
				style_sets.push_back(newset);
			else
				delete newset;
		}
	}

	return true;
}

/* StyleSet::loadCustomStyles
 * Loads all text styles from the user text style directory
 *******************************************************************/
bool StyleSet::loadCustomStyles()
{
	// If the custom stylesets directory doesn't exist, create it
	if (!wxDirExists(App::path("text_styles", App::Dir::User)))
		wxMkdir(App::path("text_styles", App::Dir::User));

	// Open the custom stylesets directory
	wxDir res_dir;
	res_dir.Open(App::path("text_styles", App::Dir::User));

	// Go through each file in the directory
	string filename = wxEmptyString;
	bool files = res_dir.GetFirst(&filename, wxEmptyString, wxDIR_FILES);
	while (files)
	{
		// Read file into tokenizer
		Tokenizer tz;
		tz.openFile(res_dir.GetName() + "/" + filename);

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Read any styleset definitions
		vector<STreeNode*> nodes = root.getChildren("styleset");
		for (unsigned b = 0; b  < nodes.size(); b++)
		{
			StyleSet* newset = new StyleSet();
			if (newset->parseSet((ParseTreeNode*)nodes[b]))
				style_sets.push_back(newset);
			else
				delete newset;
		}

		// Next file
		files = res_dir.GetNext(&filename);
	}

	return true;
}
