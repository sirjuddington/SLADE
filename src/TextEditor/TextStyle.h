#pragma once

#include "Utility/Parser.h"

class TextEditorCtrl;
class TextStyle
{
	friend class StyleSet;

public:
	TextStyle(string name, string description, int style_id = -1);
	~TextStyle();

	void addWxStyleId(int style);

	string description() { return description_; }
	string fontFace() { return font_; }
	int    fontSize() { return size_; }
	bool   hasForeground() { return fg_defined_; }
	bool   hasBackground() { return bg_defined_; }
	int    bold() { return bold_; }
	int    italic() { return italic_; }
	int    underlined() { return underlined_; }

	void setFontFace(string font) { this->font_ = font; }
	void setFontSize(int size) { this->size_ = size; }
	void setBold(int bold) { this->bold_ = bold; }
	void setItalic(int italic) { this->italic_ = italic; }
	void setUnderlined(int underlined) { this->underlined_ = underlined; }
	void setForeground(rgba_t col)
	{
		foreground_.set(col);
		fg_defined_ = true;
	}
	void clearForeground() { fg_defined_ = false; }
	void setBackground(rgba_t col)
	{
		background_.set(col);
		bg_defined_ = true;
	}
	void clearBackground() { bg_defined_ = false; }

	wxFont font();
	rgba_t foreground() { return foreground_; }
	rgba_t background() { return background_; }

	bool   parse(ParseTreeNode* node);
	void   applyTo(wxStyledTextCtrl* stc);
	bool   copyStyle(TextStyle* copy);
	string textDefinition(unsigned tabs = 0);

private:
	string      name_;
	string      description_;
	vector<int> wx_styles_;

	string font_;
	int    size_;
	rgba_t foreground_;
	bool   fg_defined_;
	rgba_t background_;
	bool   bg_defined_;
	int    bold_;
	int    italic_;
	int    underlined_;
};

class StyleSet
{
public:
	StyleSet(string name = "Unnamed Style");
	~StyleSet();

	string   getName() { return name_; }
	unsigned nStyles() { return styles_.size(); }

	bool       parseSet(ParseTreeNode* root);
	void       applyTo(TextEditorCtrl* stc);
	void       applyToWx(wxStyledTextCtrl* stc);
	bool       copySet(StyleSet* copy);
	TextStyle* style(string name);
	TextStyle* style(unsigned index);
	bool       writeFile(string filename);

	rgba_t styleForeground(string style);
	rgba_t styleBackground(string style);
	string defaultFontFace();
	int    defaultFontSize();

	// Static functions for styleset management
	static void      initCurrent();
	static void      saveCurrent();
	static StyleSet* currentSet();
	static bool      loadSet(string name);
	static bool      loadSet(unsigned index);
	static void      applyCurrent(TextEditorCtrl* stc);
	static string    styleName(unsigned index);
	static unsigned  numSets();
	static StyleSet* set(unsigned index);
	static void      addEditor(TextEditorCtrl* stc);
	static void      removeEditor(TextEditorCtrl* stc);
	static void      applyCurrentToAll();
	static void      addSet(StyleSet* set);

	static bool loadResourceStyles();
	static bool loadCustomStyles();

private:
	string             name_;
	TextStyle          ts_default_;
	TextStyle          ts_selection_;
	bool               built_in_ = false;
	vector<TextStyle*> styles_;

	static vector<TextEditorCtrl*> editors_;
};
