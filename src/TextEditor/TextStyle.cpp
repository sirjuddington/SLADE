
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    TextStyle.cpp
// Description: Classes which handle font and colour settings for the text
//              editor. TextStyle contains the actual font and colour settings
//              for a particular 'style' (eg. keywords, comments, etc).
//              StyleSet is just a set of these styles that can be loaded to
//              the scintilla 'styles' in the text editor
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "TextStyle.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Lexer.h"
#include "UI/TextEditorCtrl.h"
#include "Utility/FileUtils.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include "thirdparty/fmt/fmt/color.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, txed_override_font, "", CVar::Flag::Save)
CVAR(Int, txed_override_font_size, 0, CVar::Flag::Save)
vector<TextEditorCtrl*> StyleSet::editors_;
namespace
{
vector<unique_ptr<StyleSet>> style_sets;
StyleSet*                    ss_current = nullptr;
} // namespace


// -----------------------------------------------------------------------------
//
// TextStyle Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextStyle class constructor
// -----------------------------------------------------------------------------
TextStyle::TextStyle(string_view name, string_view description, int style_id) :
	name_{ name },
	description_{ description }
{
	// Init variables
	if (style_id >= 0)
		wx_styles_.push_back(style_id);
}

// -----------------------------------------------------------------------------
// Adds a wxSTC style id to the list (used for applying style to the
// wxStyledTextCtrl, in case this style replaces multiple)
// -----------------------------------------------------------------------------
void TextStyle::addWxStyleId(int style)
{
	wx_styles_.push_back(style);
}

// -----------------------------------------------------------------------------
// Reads text style information from a parse tree
// -----------------------------------------------------------------------------
bool TextStyle::parse(ParseTreeNode* node)
{
	// Check any info was given
	if (!node)
		return false;

	// Go through info nodes
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->childPTN(a);
		auto name  = child->name();

		// Font name
		if (StrUtil::equalCI(name, "font"))
			font_ = child->stringValue();

		// Font size
		else if (StrUtil::equalCI(name, "size"))
			size_ = child->intValue();

		// Foreground colour
		else if (StrUtil::equalCI(name, "foreground"))
		{
			foreground_.set(child->intValue(0), child->intValue(1), child->intValue(2), 255);
			fg_defined_ = true;
		}

		// Background colour
		else if (StrUtil::equalCI(name, "background"))
		{
			background_.set(child->intValue(0), child->intValue(1), child->intValue(2), 255);
			bg_defined_ = true;
		}

		// Bold
		else if (StrUtil::equalCI(name, "bold"))
			bold_ = (int)child->boolValue();

		// Italic
		else if (StrUtil::equalCI(name, "italic"))
			italic_ = (int)child->boolValue();

		// Underlined
		else if (StrUtil::equalCI(name, "underlined"))
			underlined_ = (int)child->boolValue();
	}

	return true;
}

// -----------------------------------------------------------------------------
// Applies the style settings to the scintilla text control [stc]
// -----------------------------------------------------------------------------
void TextStyle::applyTo(wxStyledTextCtrl* stc)
{
	for (int wx_style : wx_styles_)
	{
		// Set font face
		if (!txed_override_font.value.empty())
			stc->StyleSetFaceName(wx_style, txed_override_font);
		else if (!font_.empty())
			stc->StyleSetFaceName(wx_style, font_);

		// Set font size
		if (txed_override_font_size > 0)
			stc->StyleSetSize(wx_style, txed_override_font_size);
		else if (size_ > 0)
			stc->StyleSetSize(wx_style, size_);

		// Set foreground
		if (fg_defined_)
			stc->StyleSetForeground(wx_style, WXCOL(foreground_));

		// Set background
		if (bg_defined_)
			stc->StyleSetBackground(wx_style, WXCOL(background_));

		// Set bold
		if (bold_ > 0)
			stc->StyleSetBold(wx_style, true);
		else if (bold_ == 0)
			stc->StyleSetBold(wx_style, false);

		// Set italic
		if (italic_ > 0)
			stc->StyleSetItalic(wx_style, true);
		else if (italic_ == 0)
			stc->StyleSetItalic(wx_style, false);

		// Set underlined
		if (underlined_ > 0)
			stc->StyleSetUnderline(wx_style, true);
		else if (underlined_ == 0)
			stc->StyleSetUnderline(wx_style, false);
	}
}

// -----------------------------------------------------------------------------
// Copies style info from [copy]
// -----------------------------------------------------------------------------
bool TextStyle::copyStyle(TextStyle* copy)
{
	if (!copy)
		return false;

	// Copy data straight
	font_       = copy->font_;
	size_       = copy->size_;
	foreground_ = copy->foreground_;
	fg_defined_ = copy->fg_defined_;
	background_ = copy->background_;
	bg_defined_ = copy->bg_defined_;
	bold_       = copy->bold_;
	italic_     = copy->italic_;
	underlined_ = copy->underlined_;

	return true;
}

// -----------------------------------------------------------------------------
// Returns a formatted string defining this style
// -----------------------------------------------------------------------------
string TextStyle::textDefinition(unsigned tabs) const
{
	fmt::memory_buffer buf;
	string             indent(tabs, '\t');

	// Write font
	if (!font_.empty())
		fmt::format_to(buf, "{}font = \"{}\";\n", indent, font_);

	// Write size
	if (size_ >= 0)
		fmt::format_to(buf, "{}size = {};\n", indent, size_);

	// Write foreground
	if (fg_defined_)
		fmt::format_to(buf, "{}foreground = {}, {}, {};\n", indent, foreground_.r, foreground_.g, foreground_.b);

	// Write background
	if (bg_defined_)
		fmt::format_to(buf, "{}background = {}, {}, {};\n", indent, background_.r, background_.g, background_.b);

	// Write bold
	if (bold_ >= 0)
		fmt::format_to(buf, "{}bold = {};\n", indent, bold_);

	// Write italic
	if (italic_ >= 0)
		fmt::format_to(buf, "{}italic = {};\n", indent, italic_);

	// Write underlined
	if (underlined_ >= 0)
		fmt::format_to(buf, "{}underlined = {};\n", indent, underlined_);

	return fmt::to_string(buf);
}


// -----------------------------------------------------------------------------
//
// StyleSet Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// StyleSet class constructor
// -----------------------------------------------------------------------------
StyleSet::StyleSet(string_view name) :
	ts_default_("default", "Default", wxSTC_STYLE_DEFAULT),
	ts_selection_("selection", "Selected Text")
{
	// Init default style
	wxFont f(10, wxFONTFAMILY_MODERN, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
	ts_default_.font_ = f.GetFaceName();
	ts_default_.size_ = 10;
	ts_default_.foreground_.set(0, 0, 0, 255);
	ts_default_.fg_defined_ = true;
	ts_default_.background_.set(255, 255, 255, 255);
	ts_default_.bg_defined_   = true;
	ts_default_.bold_         = 0;
	ts_default_.italic_       = 0;
	ts_default_.underlined_   = 0;
	ts_selection_.bg_defined_ = true;
	ts_selection_.background_.set(150, 150, 150, 255);
	ts_selection_.fg_defined_ = false;

	// Init name
	name_ = name;

	// Init styles
	styles_.emplace_back("preprocessor", "Preprocessor", Lexer::Style::Preprocessor);
	styles_.emplace_back("comment", "Comment", Lexer::Style::Comment);
	styles_.emplace_back("comment_doc", "Comment (Doc)", Lexer::Style::CommentDoc);
	styles_.emplace_back("string", "String", Lexer::Style::String);
	styles_.emplace_back("character", "Character", Lexer::Style::Char);
	styles_.emplace_back("keyword", "Keyword", Lexer::Style::Keyword);
	styles_.emplace_back("constant", "Constant", Lexer::Style::Constant);
	styles_.emplace_back("type", "Type", Lexer::Style::Type);
	styles_.emplace_back("property", "Property", Lexer::Style::Property);
	styles_.emplace_back("function", "Function", Lexer::Style::Function);
	styles_.emplace_back("number", "Number", Lexer::Style::Number);
	styles_.emplace_back("operator", "Operator", Lexer::Style::Operator);
	styles_.emplace_back("bracematch", "Brace Match", wxSTC_STYLE_BRACELIGHT);
	styles_.emplace_back("bracebad", "Brace Mismatch", wxSTC_STYLE_BRACEBAD);
	styles_.emplace_back("linenum", "Line Numbers", wxSTC_STYLE_LINENUMBER);
	styles_.emplace_back("calltip", "Call Tip", wxSTC_STYLE_CALLTIP);
	styles_.emplace_back("calltip_hl", "Call Tip Highlight");
	styles_.emplace_back("foldmargin", "Code Folding Margin");
	styles_.emplace_back("guides", "Indent/Right Margin Guide");
	styles_.emplace_back("wordmatch", "Word Match");
	styles_.emplace_back("current_line", "Current Line");
}

// -----------------------------------------------------------------------------
// Reads style set info from a parse tree
// -----------------------------------------------------------------------------
bool StyleSet::parseSet(ParseTreeNode* root)
{
	if (!root)
		return false;

	// Get name
	auto node = root->childPTN("name");
	if (node)
		name_ = node->stringValue();

	// Parse styles
	ts_default_.parse(root->childPTN("default"));     // Default style
	ts_selection_.parse(root->childPTN("selection")); // Selection style
	for (auto& style : styles_)                       // Other styles
	{
		if (auto style_node = root->childPTN(style.name_))
			style.parse(style_node);
		else
		{
			if (style.name_ == "foldmargin")
			{
				// No 'foldmargin' style defined, copy it from line numbers style
				style.foreground_ = styleForeground("linenum");
				style.background_ = styleBackground("linenum");
				style.fg_defined_ = true;
				style.bg_defined_ = true;
			}
			else if (style.name_ == "guides")
			{
				// No 'guides' style defined, use the default foreground colour
				style.foreground_ = ts_default_.foreground();
				style.fg_defined_ = true;
			}
			else if (style.name_ == "type" || style.name_ == "property")
			{
				// No 'type' or 'property' style defined, copy it from keyword style
				style.copyStyle(this->style("keyword"));
			}
			else if (style.name_ == "comment_doc")
			{
				// No 'comment_doc' style defined, copy it from comment style
				style.copyStyle(this->style("comment"));
			}
			else if (style.name_ == "current_line")
			{
				// No 'currentline' style defined, use the default background and darken/lighten it a little
				int fgm = -20;
				int bgm = -10;
				if (ts_default_.background_.greyscale().r < 100)
				{
					fgm = 30;
					bgm = 15;
				}
				style.foreground_ = ts_default_.background().amp(fgm, fgm, fgm, 0);
				style.fg_defined_ = true;
				style.background_ = ts_default_.background().amp(bgm, bgm, bgm, 0);
				style.bg_defined_ = true;
			}
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Applies all the styles in this set to the text styles in scintilla text
// control [stc]
// -----------------------------------------------------------------------------
void StyleSet::applyTo(TextEditorCtrl* stc)
{
	applyToWx(stc);

	// Update code folding margin
	stc->setupFoldMargin(style("foldmargin"));
}

void StyleSet::applyToWx(wxStyledTextCtrl* stc)
{
	// Set default style
	ts_default_.applyTo(stc);

	// Apply default style to all
	stc->StyleClearAll();

	// Apply other styles
	for (auto& style : styles_)
		style.applyTo(stc);

	// Set selection background if customised
	if (ts_selection_.hasBackground())
		stc->SetSelBackground(true, WXCOL(ts_selection_.background_));
	else
		stc->SetSelBackground(false, wxColour("red"));

	// Set selection foreground if customised
	if (ts_selection_.hasForeground())
		stc->SetSelForeground(true, WXCOL(ts_selection_.foreground_));
	else
		stc->SetSelForeground(false, wxColour("red"));

	// Set caret colour to text foreground colour
	stc->SetCaretForeground(WXCOL(ts_default_.foreground_));

	// Set indent and right margin line colour
	stc->SetEdgeColour(WXCOL(style("guides")->foreground()));
	stc->StyleSetBackground(wxSTC_STYLE_INDENTGUIDE, WXCOL(styleBackground("guides")));
	stc->StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, WXCOL(styleForeground("guides")));

	// Set word match indicator colour
	stc->SetIndicatorCurrent(8);
	stc->IndicatorSetForeground(8, WXCOL(styleForeground("wordmatch")));

	// Set current line colour
	stc->SetCaretLineBackground(WXCOL(styleBackground("current_line")));
	stc->MarkerDefine(
		1, wxSTC_MARK_BACKGROUND, WXCOL(styleBackground("current_line")), WXCOL(styleBackground("current_line")));
	stc->MarkerDefine(
		2, wxSTC_MARK_UNDERLINE, WXCOL(styleForeground("current_line")), WXCOL(styleForeground("current_line")));
}

// -----------------------------------------------------------------------------
// Copies all styles in [copy] to this set
// -----------------------------------------------------------------------------
bool StyleSet::copySet(StyleSet* copy)
{
	if (!copy)
		return false;

	// Copy all styles
	ts_default_.copyStyle(&(copy->ts_default_));
	ts_selection_.copyStyle(&(copy->ts_selection_));
	for (unsigned a = 0; a < copy->styles_.size(); a++)
		styles_[a].copyStyle(&copy->styles_[a]);

	return true;
}

// -----------------------------------------------------------------------------
// Returns the text style associated with [name] (these are hard coded), or
// nullptr if [name] was invalid
// -----------------------------------------------------------------------------
TextStyle* StyleSet::style(string_view name)
{
	// Return style matching name given
	if (StrUtil::equalCI(name, "default"))
		return &ts_default_;
	else if (StrUtil::equalCI(name, "selection"))
		return &ts_selection_;
	else
	{
		for (auto& style : styles_)
		{
			if (style.name_ == name)
				return &style;
		}
	}

	// Not a valid style
	return nullptr;
}

// -----------------------------------------------------------------------------
// Returns the extra text style at [index]
// -----------------------------------------------------------------------------
TextStyle* StyleSet::style(unsigned index)
{
	if (index < styles_.size())
		return &styles_[index];
	else
		return nullptr;
}

// -----------------------------------------------------------------------------
// Writes this style set as a text definition to a file [filename]
// -----------------------------------------------------------------------------
bool StyleSet::writeFile(string_view filename)
{
	// Open file for writing
	wxFile file(wxString{ filename.data(), filename.size() }, wxFile::write);

	if (!file.IsOpened())
		return false;

	// Write opening
	file.Write("styleset {\n");

	// Name
	file.Write(wxString::Format("\tname = \"%s\";\n\n", name_));

	// Default style
	file.Write("\tdefault {\n");
	file.Write(ts_default_.textDefinition(2));
	file.Write("\t}\n\n");

	// Selection style
	file.Write("\tselection {\n");
	file.Write(ts_selection_.textDefinition(2));
	file.Write("\t}\n\n");

	// Other styles
	for (auto& style : styles_)
	{
		file.Write(wxString::Format("\t%s {\n", style.name_));
		file.Write(style.textDefinition(2));
		file.Write("\t}\n\n");
	}

	// Write end
	file.Write("}\n");

	// Close file
	file.Close();

	return true;
}

// -----------------------------------------------------------------------------
// Returns the foreground colour of [style], or the default style's foreground
// colour if it is not set
// -----------------------------------------------------------------------------
ColRGBA StyleSet::styleForeground(string_view style_name)
{
	auto s = style(style_name);
	return s && s->hasForeground() ? s->foreground() : ts_default_.foreground();
}

// -----------------------------------------------------------------------------
// Returns the background colour of [style], or the default style's background
// colour if it is not set
// -----------------------------------------------------------------------------
ColRGBA StyleSet::styleBackground(string_view style_name)
{
	auto s = style(style_name);
	return s && s->hasBackground() ? s->background() : ts_default_.background();
}

// -----------------------------------------------------------------------------
// Returns the default style font face
// -----------------------------------------------------------------------------
string StyleSet::defaultFontFace()
{
	return !txed_override_font.value.empty() ? txed_override_font : style("default")->fontFace();
}

// -----------------------------------------------------------------------------
// Returns the default style font size
// -----------------------------------------------------------------------------
int StyleSet::defaultFontSize()
{
	if (!txed_override_font.value.empty() && txed_override_font_size > 0)
		return txed_override_font_size;

	return style("default")->fontSize();
}


// -----------------------------------------------------------------------------
//
// StyleSet Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Initialises the 'current' style set from the previously saved 'current.sss'
// file, or uses the default set if the file does not exist
// -----------------------------------------------------------------------------
void StyleSet::initCurrent()
{
	// Create 'current' styleset
	ss_current        = new StyleSet();
	ss_current->name_ = "<current styleset>";

	// First up, check if "<userdir>/current.sss" exists
	auto path = App::path("current.sss", App::Dir::User);
	if (FileUtil::fileExists(path))
	{
		// Read it in
		Tokenizer tz;
		tz.openFile(path);

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Find definition
		auto node = root.childPTN("styleset");
		if (node)
		{
			// If found, load it into the current set
			ss_current->parseSet(node);
			return;
		}
	}

	// Unable to load from userdir, just load first styleset (should be default)
	if (!style_sets.empty())
		ss_current->copySet(style_sets[0].get());
}

// -----------------------------------------------------------------------------
// Writes the current style set to the 'current.sss' file
// -----------------------------------------------------------------------------
void StyleSet::saveCurrent()
{
	if (!ss_current)
		return;

	ss_current->writeFile(App::path("current.sss", App::Dir::User));
}

// -----------------------------------------------------------------------------
// Returns the current style set
// -----------------------------------------------------------------------------
StyleSet* StyleSet::currentSet()
{
	if (!ss_current)
		initCurrent();

	return ss_current;
}

// -----------------------------------------------------------------------------
// Loads the style set matching [name] to the current style set.
// Returns false if no match was found, true otherwise
// -----------------------------------------------------------------------------
bool StyleSet::loadSet(string_view name)
{
	// Search for set matching name
	for (auto& style_set : style_sets)
	{
		if (StrUtil::equalCI(style_set->name_, name))
		{
			ss_current->copySet(style_set.get());
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Loads the style set at [index] to the current style set.
// Returns false if [index] was out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool StyleSet::loadSet(unsigned index)
{
	// Check index
	if (index >= style_sets.size())
		return false;

	currentSet()->copySet(style_sets[index].get());

	return true;
}

// -----------------------------------------------------------------------------
// Applies the current style set to the scintilla text control [stc]
// -----------------------------------------------------------------------------
void StyleSet::applyCurrent(TextEditorCtrl* stc)
{
	currentSet()->applyTo(stc);
}

// -----------------------------------------------------------------------------
// Returns the name of the style set at [index], or an empty string if [index]
// is out of bounds
// -----------------------------------------------------------------------------
string StyleSet::styleName(unsigned index)
{
	// Check index
	if (index >= style_sets.size())
		return "";

	return style_sets[index]->name_;
}

// -----------------------------------------------------------------------------
// Returns the number of loaded style sets
// -----------------------------------------------------------------------------
unsigned StyleSet::numSets()
{
	return style_sets.size();
}

// -----------------------------------------------------------------------------
// Returns the style set at [index], or nullptr if [index] is out of bounds
// -----------------------------------------------------------------------------
StyleSet* StyleSet::set(unsigned index)
{
	// Check index
	if (index >= style_sets.size())
		return nullptr;

	return style_sets[index].get();
}

// -----------------------------------------------------------------------------
// Adds [stc] to the current list of text editors
// -----------------------------------------------------------------------------
void StyleSet::addEditor(TextEditorCtrl* stc)
{
	editors_.push_back(stc);
}

// -----------------------------------------------------------------------------
// Removes [stc] from the current list of text editors
// -----------------------------------------------------------------------------
void StyleSet::removeEditor(TextEditorCtrl* stc)
{
	VECTOR_REMOVE(editors_, stc);
}

// -----------------------------------------------------------------------------
// Applies the current style set to all text editors in the list
// -----------------------------------------------------------------------------
void StyleSet::applyCurrentToAll()
{
	for (auto& editor : editors_)
		applyCurrent(editor);
}

// -----------------------------------------------------------------------------
// Adds [set] to the list of text styles (makes a copy). If a custom set with
// [set]'s name already exists, copy [set] to it
// -----------------------------------------------------------------------------
void StyleSet::addSet(StyleSet* set)
{
	// Find existing custom set with same name
	for (const auto& s : style_sets)
		if (s->name_ == set->name_)
		{
			// Non-custom set exists, need to create a copy instead
			if (s->built_in_)
			{
				set->name_ += " (Copy)";
				break;
			}

			// Copy set
			s->copySet(set);
			return;
		}

	// Not found, add new set
	auto new_set = std::make_unique<StyleSet>(set->name_);
	new_set->copySet(set);
	style_sets.push_back(std::move(new_set));
}

// -----------------------------------------------------------------------------
// Loads all text styles from the slade resource archive (slade.pk3)
// -----------------------------------------------------------------------------
bool StyleSet::loadResourceStyles()
{
	// Get 'config/text_styles' directory in slade.pk3
	auto dir = App::archiveManager().programResourceArchive()->dir("config/text_styles");

	// Check it exists
	if (!dir)
	{
		Log::warning("No 'config/text_styles' directory exists in slade.pk3");
		return false;
	}

	// Read default style set first
	auto default_style = dir->entry("default.sss");
	if (default_style)
	{
		// Read entry data into tokenizer
		Tokenizer tz;
		tz.openMem(default_style->data(), default_style->name());

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Read any styleset definitions
		auto nodes = root.children("styleset");
		for (auto& node : nodes)
		{
			auto newset       = std::make_unique<StyleSet>();
			newset->built_in_ = true;
			if (newset->parseSet(dynamic_cast<ParseTreeNode*>(node)))
				style_sets.push_back(std::move(newset));
		}
	}

	// Go through all entries within it
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		auto entry = dir->entryAt(a);

		// Skip default
		if (entry->nameNoExt() == "default")
			continue;

		// Read entry data into tokenizer
		Tokenizer tz;
		tz.openMem(entry->data(), entry->name());

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Read any styleset definitions
		auto nodes = root.children("styleset");
		for (auto& node : nodes)
		{
			auto newset       = std::make_unique<StyleSet>();
			newset->built_in_ = true;
			if (newset->parseSet(dynamic_cast<ParseTreeNode*>(node)))
				style_sets.push_back(std::move(newset));
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Loads all text styles from the user text style directory
// -----------------------------------------------------------------------------
bool StyleSet::loadCustomStyles()
{
	// If the custom stylesets directory doesn't exist, create it
	auto custom_dir = App::path("text_styles", App::Dir::User);
	FileUtil::createDir(custom_dir);

	// Go through each file in the directory
	for (const auto& path : FileUtil::allFilesInDir(custom_dir))
	{
		// Read file into tokenizer
		Tokenizer tz;
		tz.openFile(path);

		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Read any styleset definitions
		auto nodes = root.children("styleset");
		for (auto& node : nodes)
		{
			auto newset = std::make_unique<StyleSet>();
			if (newset->parseSet(dynamic_cast<ParseTreeNode*>(node)))
				style_sets.push_back(std::move(newset));
		}
	}

	return true;
}
