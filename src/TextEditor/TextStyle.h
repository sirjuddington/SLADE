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

	string getDescription() { return description_; }
	string getFontFace() { return font_; }
	int    getFontSize() { return size_; }
	bool   hasForeground() { return fg_defined_; }
	bool   hasBackground() { return bg_defined_; }
	int    getBold() { return bold_; }
	int    getItalic() { return italic_; }
	int    getUnderlined() { return underlined_; }

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

	wxFont getFont();
	rgba_t getForeground() { return foreground_; }
	rgba_t getBackground() { return background_; }

	bool   parse(ParseTreeNode* node);
	void   applyTo(wxStyledTextCtrl* stc);
	bool   copyStyle(TextStyle* copy);
	string getDefinition(unsigned tabs = 0);

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
	TextStyle* getStyle(string name);
	TextStyle* getStyle(unsigned index);
	bool       writeFile(string filename);

	rgba_t getStyleForeground(string style);
	rgba_t getStyleBackground(string style);
	string getDefaultFontFace();
	int    getDefaultFontSize();

	// Static functions for styleset management
	static void      initCurrent();
	static void      saveCurrent();
	static StyleSet* currentSet();
	static bool      loadSet(string name);
	static bool      loadSet(unsigned index);
	static void      applyCurrent(TextEditorCtrl* stc);
	static string    getName(unsigned index);
	static unsigned  numSets();
	static StyleSet* getSet(unsigned index);
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

	static vector<TextEditorCtrl*> editors;
};
