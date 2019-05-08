#pragma once

#include "Utility/Colour.h"
#include "Utility/Parser.h"

class TextEditorCtrl;
class TextStyle
{
	friend class StyleSet;

public:
	TextStyle(string_view name, string_view description, int style_id = -1);
	~TextStyle() = default;

	void addWxStyleId(int style);

	const string& description() const { return description_; }
	const string& fontFace() const { return font_; }
	int           fontSize() const { return size_; }
	bool          hasForeground() const { return fg_defined_; }
	bool          hasBackground() const { return bg_defined_; }
	int           bold() const { return bold_; }
	int           italic() const { return italic_; }
	int           underlined() const { return underlined_; }

	void setFontFace(string_view font) { font_ = font; }
	void setFontSize(int size) { size_ = size; }
	void setBold(int bold) { bold_ = bold; }
	void setItalic(int italic) { italic_ = italic; }
	void setUnderlined(int underlined) { underlined_ = underlined; }
	void setForeground(ColRGBA col)
	{
		foreground_.set(col);
		fg_defined_ = true;
	}
	void clearForeground() { fg_defined_ = false; }
	void setBackground(ColRGBA col)
	{
		background_.set(col);
		bg_defined_ = true;
	}
	void clearBackground() { bg_defined_ = false; }

	ColRGBA foreground() const { return foreground_; }
	ColRGBA background() const { return background_; }

	bool   parse(ParseTreeNode* node);
	void   applyTo(wxStyledTextCtrl* stc);
	bool   copyStyle(TextStyle* copy);
	string textDefinition(unsigned tabs = 0) const;

private:
	string      name_;
	string      description_;
	vector<int> wx_styles_;

	string  font_;
	int     size_ = -1;
	ColRGBA foreground_;
	bool    fg_defined_ = false;
	ColRGBA background_;
	bool    bg_defined_ = false;
	int     bold_       = -1;
	int     italic_     = -1;
	int     underlined_ = -1;
};

class StyleSet
{
public:
	StyleSet(string_view name = "Unnamed Style");
	~StyleSet() = default;

	const string& getName() const { return name_; }
	unsigned      nStyles() const { return styles_.size(); }

	bool       parseSet(ParseTreeNode* root);
	void       applyTo(TextEditorCtrl* stc);
	void       applyToWx(wxStyledTextCtrl* stc);
	bool       copySet(StyleSet* copy);
	TextStyle* style(string_view name);
	TextStyle* style(unsigned index);
	bool       writeFile(string_view filename);

	ColRGBA styleForeground(string_view style_name);
	ColRGBA styleBackground(string_view style_name);
	string  defaultFontFace();
	int     defaultFontSize();

	// Static functions for styleset management
	static void      initCurrent();
	static void      saveCurrent();
	static StyleSet* currentSet();
	static bool      loadSet(string_view name);
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
	string            name_;
	TextStyle         ts_default_;
	TextStyle         ts_selection_;
	bool              built_in_ = false;
	vector<TextStyle> styles_;

	static vector<TextEditorCtrl*> editors_;
};
