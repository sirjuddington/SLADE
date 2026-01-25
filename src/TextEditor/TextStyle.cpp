
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
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
#include "Archive/Archive.h"
#include "Archive/ArchiveDir.h"
#include "Archive/ArchiveEntry.h"
#include "Archive/ArchiveManager.h"
#include "Lexer.h"
#include "UI/TextEditorCtrl.h"
#include "Utility/Colour.h"
#include "Utility/FileUtils.h"
#include "Utility/JsonUtils.h"
#include "Utility/Parser.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"
#include <fmt/format.h>

using namespace slade;


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
bool TextStyle::parse(const ParseTreeNode* node)
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
		if (strutil::equalCI(name, "font"))
			font_ = child->stringValue();

		// Font size
		else if (strutil::equalCI(name, "size"))
			size_ = child->intValue();

		// Foreground colour
		else if (strutil::equalCI(name, "foreground"))
		{
			foreground_.set(child->intValue(0), child->intValue(1), child->intValue(2), 255);
			fg_defined_ = true;
		}

		// Background colour
		else if (strutil::equalCI(name, "background"))
		{
			background_.set(child->intValue(0), child->intValue(1), child->intValue(2), 255);
			bg_defined_ = true;
		}

		// Bold
		else if (strutil::equalCI(name, "bold"))
			bold_ = static_cast<int>(child->boolValue());

		// Italic
		else if (strutil::equalCI(name, "italic"))
			italic_ = static_cast<int>(child->boolValue());

		// Underlined
		else if (strutil::equalCI(name, "underlined"))
			underlined_ = static_cast<int>(child->boolValue());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Applies the style settings to the scintilla text control [stc]
// -----------------------------------------------------------------------------
void TextStyle::applyTo(wxStyledTextCtrl* stc) const
{
	for (int wx_style : wx_styles_)
	{
		// Set font face
		if (!txed_override_font.value.empty())
			stc->StyleSetFaceName(wx_style, txed_override_font);
		else if (!font_.empty())
			stc->StyleSetFaceName(wx_style, wxString::FromUTF8(font_));

		// Set font size
		if (txed_override_font_size > 0)
			stc->StyleSetSize(wx_style, txed_override_font_size);
		else if (size_ > 0)
			stc->StyleSetSize(wx_style, size_);

		// Set foreground
		if (fg_defined_)
			stc->StyleSetForeground(wx_style, foreground_);

		// Set background
		if (bg_defined_)
			stc->StyleSetBackground(wx_style, background_);

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
bool TextStyle::copyStyle(const TextStyle* copy)
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
// Reads text style information from a JSON object [j]
// -----------------------------------------------------------------------------
void TextStyle::fromJson(const Json& j)
{
	if (j.contains("font"))
		font_ = j["font"].get<string>();
	if (j.contains("size"))
		size_ = j["size"].get<int>();
	if (j.contains("bold"))
		bold_ = j["bold"].get<bool>() ? 1 : 0;
	if (j.contains("italic"))
		italic_ = j["italic"].get<bool>() ? 1 : 0;
	if (j.contains("underlined"))
		underlined_ = j["underlined"].get<bool>() ? 1 : 0;
	if (j.contains("foreground"))
	{
		foreground_ = colour::fromString(j["foreground"].get<string>());
		fg_defined_ = true;
	}
	if (j.contains("background"))
	{
		background_ = colour::fromString(j["background"].get<string>());
		bg_defined_ = true;
	}
}

// -----------------------------------------------------------------------------
// Converts the text style to a JSON object
// -----------------------------------------------------------------------------
Json TextStyle::toJson() const
{
	json j;
	if (!font_.empty())
		j["font"] = font_;
	if (size_ >= 0)
		j["size"] = size_;
	if (bold_ >= 0)
		j["bold"] = bold_ != 0;
	if (italic_ >= 0)
		j["italic"] = italic_ != 0;
	if (underlined_ >= 0)
		j["underlined"] = underlined_ != 0;
	if (fg_defined_)
		j["foreground"] = colour::toString(foreground_, colour::StringFormat::RGB);
	if (bg_defined_)
		j["background"] = colour::toString(background_, colour::StringFormat::RGB);
	return j;
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
	ts_default_.font_ = f.GetFaceName().utf8_string();
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
bool StyleSet::parseOldSet(const ParseTreeNode* root)
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
				if (colour::greyscale(ts_default_.background_).r < 100)
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
// Reads style set info from a JSON object [j]
// -----------------------------------------------------------------------------
void StyleSet::readSet(const Json& j)
{
	// Get name
	j["name"].get_to(name_);

	// Parse styles
	ts_default_.fromJson(j["default"]);     // Default style
	ts_selection_.fromJson(j["selection"]); // Selection style
	for (auto& style : styles_)             // Other styles
	{
		// Check if style is defined in JSON
		if (j.contains(style.name_))
			style.fromJson(j[style.name_]);

		// Not defined - do special handling for certain styles
		else if (style.name_ == "foldmargin")
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
			int fgm = -20;
			int bgm = -10;
			if (colour::greyscale(ts_default_.background_).r < 100)
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
		stc->SetSelBackground(true, ts_selection_.background_);
	else
		stc->SetSelBackground(false, wxColour(wxS("red")));

	// Set selection foreground if customised
	if (ts_selection_.hasForeground())
		stc->SetSelForeground(true, ts_selection_.foreground_);
	else
		stc->SetSelForeground(false, wxColour(wxS("red")));

	// Set caret colour to text foreground colour
	stc->SetCaretForeground(ts_default_.foreground_);

	// Set indent and right margin line colour
	stc->SetEdgeColour(style("guides")->foreground());
	stc->StyleSetBackground(wxSTC_STYLE_INDENTGUIDE, styleBackground("guides"));
	stc->StyleSetForeground(wxSTC_STYLE_INDENTGUIDE, styleForeground("guides"));

	// Set word match indicator colour
	stc->SetIndicatorCurrent(8);
	stc->IndicatorSetForeground(8, styleForeground("wordmatch"));

	// Set current line colour
	stc->SetCaretLineBackground(styleBackground("current_line"));
	stc->MarkerDefine(1, wxSTC_MARK_BACKGROUND, styleBackground("current_line"), styleBackground("current_line"));
	stc->MarkerDefine(2, wxSTC_MARK_UNDERLINE, styleForeground("current_line"), styleForeground("current_line"));
}

// -----------------------------------------------------------------------------
// Copies all styles in [copy] to this set
// -----------------------------------------------------------------------------
bool StyleSet::copySet(const StyleSet* copy)
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
	if (strutil::equalCI(name, "default"))
		return &ts_default_;
	else if (strutil::equalCI(name, "selection"))
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
// Writes this style set as JSON to a file [filename]
// -----------------------------------------------------------------------------
bool StyleSet::writeFile(string_view filename) const
{
	// Build JSON
	ordered_json j;
	j["name"]      = name_;                  // Name
	j["default"]   = ts_default_.toJson();   // Default style
	j["selection"] = ts_selection_.toJson(); // Selection style
	for (const auto& style : styles_)        // Other styles
		if (auto js = style.toJson(); !js.is_null())
			j[style.name_] = js;

	// Write to file
	return jsonutil::writeFile(j, filename);
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
// Initialises the 'current' style set from the previously saved
// text_style_current file, or uses the default set if the file does not exist
// -----------------------------------------------------------------------------
void StyleSet::initCurrent()
{
	// Create 'current' styleset
	ss_current        = new StyleSet();
	ss_current->name_ = "<current styleset>";

	// First up, check if "<userdir>/text_style_current.json" exists
	auto path = app::path("text_style_current.json", app::Dir::User);
	if (fileutil::fileExists(path))
	{
		if (auto j = jsonutil::parseFile(path); !j.is_discarded())
		{
			ss_current->readSet(j);
			return;
		}
	}

	// Try pre-3.3.0 'current.sss'
	if (Tokenizer tz; tz.openFile(app::path("current.sss", app::Dir::User)))
	{
		// Parse it
		ParseTreeNode root;
		root.allowDup(true);
		root.parse(tz);

		// Find definition
		auto node = root.childPTN("styleset");
		if (node)
			ss_current->parseOldSet(node);
	}

	// Unable to load from userdir, just load default styleset depending on
	// overall app theme
	if (app::isDarkTheme())
		ss_current->copySet(style_sets[0].get());
	else
		ss_current->copySet(style_sets[1].get());
}

// -----------------------------------------------------------------------------
// Writes the current style set to the text_style_current file
// -----------------------------------------------------------------------------
void StyleSet::saveCurrent()
{
	if (!ss_current)
		return;

	ss_current->writeFile(app::path("text_style_current.json", app::Dir::User));
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
		if (strutil::equalCI(style_set->name_, name))
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
// Returns the style set matching [name], or nullptr if no match was found
// -----------------------------------------------------------------------------
StyleSet* StyleSet::set(string_view name)
{
	// Search for set matching name
	for (auto& style_set : style_sets)
	{
		if (strutil::equalCI(style_set->name_, name))
			return style_set.get();
	}

	// Not found
	return nullptr;
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
void StyleSet::removeEditor(const TextEditorCtrl* stc)
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
	auto dir = app::archiveManager().programResourceArchive()->dirAtPath("config/text_styles");

	// Check it exists
	if (!dir)
	{
		log::warning("No 'config/text_styles' directory exists in slade.pk3");
		return false;
	}

	// Go through all entries within it
	for (unsigned a = 0; a < dir->numEntries(); a++)
	{
		auto entry = dir->entryAt(a);

		// Parse JSON styleset and add it
		if (auto j = jsonutil::parse(entry->data()); !j.is_discarded())
		{
			auto newset       = std::make_unique<StyleSet>();
			newset->built_in_ = true;
			newset->readSet(j);
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
	auto custom_dir = app::path("text_styles", app::Dir::User);
	if (!fileutil::dirExists(custom_dir))
		fileutil::createDir(custom_dir);

	// Go through each file in the directory, get list of json and sss files
	vector<string> json_files;
	vector<string> sss_files;
	for (const auto& path : fileutil::allFilesInDir(custom_dir, true, true))
	{
		if (strutil::Path::extensionOf(path) == "json")
			json_files.push_back(path);
		else if (strutil::Path::extensionOf(path) == "sss")
			sss_files.push_back(path);
	}

	// Load JSON stylesets
	for (const auto& path : json_files)
	{
		if (auto j = jsonutil::parseFile(path); !j.is_discarded())
		{
			auto newset = std::make_unique<StyleSet>();
			newset->readSet(j);
			style_sets.push_back(std::move(newset));
		}
	}

	// Load any old sss stylesets (if not already loaded) and convert to json
	for (const auto& path : sss_files)
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
			if (newset->parseOldSet(dynamic_cast<ParseTreeNode*>(node)))
			{
				// Check the set isn't already loaded
				if (set(newset->name_))
					continue;

				// Write to JSON styleset
				auto filename = fmt::format("text_styles/{}.json", strutil::replace(newset->name_, " ", "_"));
				newset->writeFile(app::path(filename, app::Dir::User));

				style_sets.push_back(std::move(newset));
			}
		}
	}

	return true;
}
