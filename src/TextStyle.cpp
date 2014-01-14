
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
#include "WxStuff.h"
#include "TextStyle.h"
#include "ArchiveManager.h"
#include <wx/dir.h>


/*******************************************************************
 * VARIABLES
 *******************************************************************/
vector<StyleSet*>	style_sets;
StyleSet*			ss_current = NULL;


/*******************************************************************
 * TEXTSTYLE CLASS FUNCTIONS
 *******************************************************************/

/* TextStyle::TextStyle
 * TextStyle class constructor
 *******************************************************************/
TextStyle::TextStyle()
{
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
		ParseTreeNode* child = (ParseTreeNode*)node->getChild(a);
		string name = child->getName();

		// Font name
		if (S_CMPNOCASE(name, "font"))
			font = child->getStringValue();

		// Font size
		if (S_CMPNOCASE(name, "size"))
			size = child->getIntValue();

		// Foreground colour
		if (S_CMPNOCASE(name, "foreground"))
		{
			foreground.set(child->getIntValue(0), child->getIntValue(1), child->getIntValue(2), 255);
			fg_defined = true;
		}

		// Background colour
		if (S_CMPNOCASE(name, "background"))
		{
			background.set(child->getIntValue(0), child->getIntValue(1), child->getIntValue(2), 255);
			bg_defined = true;
		}

		// Bold
		if (S_CMPNOCASE(name, "bold"))
			bold = (int)child->getBoolValue();

		// Italic
		if (S_CMPNOCASE(name, "italic"))
			italic = (int)child->getBoolValue();

		// Underlined
		if (S_CMPNOCASE(name, "underlined"))
			underlined = (int)child->getBoolValue();
	}

	return true;
}

/* TextStyle::applyTo
 * Applies the style settings to [style] in the scintilla text
 * control [stc]
 *******************************************************************/
void TextStyle::applyTo(wxStyledTextCtrl* stc, int style)
{
	// Set font face
	if (!font.IsEmpty())
		stc->StyleSetFaceName(style, font);

	// Set font size
	if (size > 0)
		stc->StyleSetSize(style, size);

	// Set foreground
	if (fg_defined)
		stc->StyleSetForeground(style, WXCOL(foreground));

	// Set background
	if (bg_defined)
		stc->StyleSetBackground(style, WXCOL(background));

	// Set bold
	if (bold > 0)
		stc->StyleSetBold(style, true);
	else if (bold == 0)
		stc->StyleSetBold(style, false);

	// Set italic
	if (italic > 0)
		stc->StyleSetItalic(style, true);
	else if (italic == 0)
		stc->StyleSetItalic(style, false);

	// Set underlined
	if (underlined > 0)
		stc->StyleSetUnderline(style, true);
	else if (underlined == 0)
		stc->StyleSetUnderline(style, false);
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
		ret += S_FMT("font = \"%s\";\n", CHR(font));
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
StyleSet::StyleSet(string name)
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

	// Init name
	this->name = name;
}

/* StyleSet::~StyleSet
 * StyleSet class destructor
 *******************************************************************/
StyleSet::~StyleSet()
{
}

/* StyleSet::parseSet
 * Reads style set info from a parse tree
 *******************************************************************/
bool StyleSet::parseSet(ParseTreeNode* root)
{
	if (!root)
		return false;

	// Get name
	ParseTreeNode* node = (ParseTreeNode*)root->getChild("name");
	if (node)
		name = node->getStringValue();

	// Parse styles
	ts_default.parse((ParseTreeNode*)root->getChild("default"));			// Default style
	ts_preprocessor.parse((ParseTreeNode*)root->getChild("preprocessor"));	// Preprocessor style
	ts_comment.parse((ParseTreeNode*)root->getChild("comment"));			// Comment style
	ts_string.parse((ParseTreeNode*)root->getChild("string"));				// String style
	ts_character.parse((ParseTreeNode*)root->getChild("character"));		// Character style
	ts_keyword.parse((ParseTreeNode*)root->getChild("keyword"));			// Keyword style
	ts_constant.parse((ParseTreeNode*)root->getChild("constant"));			// Constant style
	ts_function.parse((ParseTreeNode*)root->getChild("function"));			// Function style
	ts_bracematch.parse((ParseTreeNode*)root->getChild("bracematch"));		// Brace-match style
	ts_bracebad.parse((ParseTreeNode*)root->getChild("bracebad"));			// Brace-mismatch style

	return true;
}

/* StyleSet::applyTo
 * Applies all the styles in this set to the text styles in scintilla
 * text control [stc]
 *******************************************************************/
void StyleSet::applyTo(wxStyledTextCtrl* stc)
{
	// Set default style
	ts_default.applyTo(stc, wxSTC_STYLE_DEFAULT);

	// Apply default style to all
	stc->StyleClearAll();

	// Apply other styles
	ts_preprocessor.applyTo(stc, wxSTC_C_PREPROCESSOR);
	ts_comment.applyTo(stc, wxSTC_C_COMMENT);
	ts_comment.applyTo(stc, wxSTC_C_COMMENTLINE);
	ts_string.applyTo(stc, wxSTC_C_STRING);
	ts_character.applyTo(stc, wxSTC_C_CHARACTER);
	ts_keyword.applyTo(stc, wxSTC_C_WORD);
	ts_constant.applyTo(stc, wxSTC_C_GLOBALCLASS);
	ts_function.applyTo(stc, wxSTC_C_WORD2);
	ts_bracematch.applyTo(stc, wxSTC_STYLE_BRACELIGHT);
	ts_bracebad.applyTo(stc, wxSTC_STYLE_BRACEBAD);

	// Set caret colour to text foreground colour
	stc->SetCaretForeground(WXCOL(ts_default.foreground));
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
	ts_preprocessor.copyStyle(&(copy->ts_preprocessor));
	ts_comment.copyStyle(&(copy->ts_comment));
	ts_string.copyStyle(&(copy->ts_string));
	ts_character.copyStyle(&(copy->ts_character));
	ts_keyword.copyStyle(&(copy->ts_keyword));
	ts_constant.copyStyle(&(copy->ts_constant));
	ts_function.copyStyle(&(copy->ts_function));
	ts_bracematch.copyStyle(&(copy->ts_bracematch));
	ts_bracebad.copyStyle(&(copy->ts_bracebad));

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
	else if (S_CMPNOCASE(name, "preprocessor"))
		return &ts_preprocessor;
	else if (S_CMPNOCASE(name, "comment"))
		return &ts_comment;
	else if (S_CMPNOCASE(name, "string"))
		return &ts_string;
	else if (S_CMPNOCASE(name, "character"))
		return &ts_character;
	else if (S_CMPNOCASE(name, "keyword"))
		return &ts_keyword;
	else if (S_CMPNOCASE(name, "constant"))
		return &ts_constant;
	else if (S_CMPNOCASE(name, "function"))
		return &ts_function;
	else if (S_CMPNOCASE(name, "bracematch"))
		return &ts_bracematch;
	else if (S_CMPNOCASE(name, "bracebad"))
		return &ts_bracebad;

	// Not a valid style
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
	file.Write(S_FMT("\tname = \"%s\";\n\n", CHR(name)));

	// Default style
	file.Write("\tdefault {\n");
	file.Write(ts_default.getDefinition(2));
	file.Write("\t}\n\n");

	// Preprocessor style
	file.Write("\tpreprocessor {\n");
	file.Write(ts_preprocessor.getDefinition(2));
	file.Write("\t}\n\n");

	// Comment style
	file.Write("\tcomment {\n");
	file.Write(ts_comment.getDefinition(2));
	file.Write("\t}\n\n");

	// String style
	file.Write("\tstring {\n");
	file.Write(ts_string.getDefinition(2));
	file.Write("\t}\n\n");

	// Character style
	file.Write("\tcharacter {\n");
	file.Write(ts_character.getDefinition(2));
	file.Write("\t}\n\n");

	// Keyword style
	file.Write("\tkeyword {\n");
	file.Write(ts_keyword.getDefinition(2));
	file.Write("\t}\n\n");

	// Constant style
	file.Write("\tconstant {\n");
	file.Write(ts_constant.getDefinition(2));
	file.Write("\t}\n\n");

	// Function style
	file.Write("\tfunction {\n");
	file.Write(ts_function.getDefinition(2));
	file.Write("\t}\n\n");

	// BraceMatch style
	file.Write("\tbracematch {\n");
	file.Write(ts_bracematch.getDefinition(2));
	file.Write("\t}\n\n");

	// BraceBad style
	file.Write("\tbracebad {\n");
	file.Write(ts_bracebad.getDefinition(2));
	file.Write("\t}\n\n");

	// Write end
	file.Write("}\n");

	// Close file
	file.Close();

	return true;
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
	string path = appPath("current.sss", DIR_USER);
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
		ParseTreeNode* node = (ParseTreeNode*)root.getChild("styleset");
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

	ss_current->writeFile(appPath("current.sss", DIR_USER));
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
void StyleSet::applyCurrent(wxStyledTextCtrl* stc)
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
		wxLogMessage("Warning: No 'config/text_styles' directory exists in slade.pk3");
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
	if (!wxDirExists(appPath("text_styles", DIR_USER)))
		wxMkdir(appPath("text_styles", DIR_USER));

	// Open the custom stylesets directory
	wxDir res_dir;
	res_dir.Open(appPath("text_styles", DIR_USER));

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
