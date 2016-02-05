
#ifndef __TEXT_STYLE_H__
#define __TEXT_STYLE_H__

#include "Utility/Parser.h"
#include <wx/font.h>
#include <wx/stc/stc.h>

class TextStyle
{
	friend class StyleSet;
private:
	string			name;
	string			description;
	vector<int>		wx_styles;

	string	font;
	int		size;
	rgba_t	foreground;
	bool	fg_defined;
	rgba_t	background;
	bool	bg_defined;
	int		bold;
	int		italic;
	int		underlined;

public:
	TextStyle(string name, string description, int style_id = -1);
	~TextStyle();

	void	addWxStyleId(int style);

	string	getDescription() { return description; }
	string	getFontFace() { return font; }
	int		getFontSize() { return size; }
	bool	hasForeground() { return fg_defined; }
	bool	hasBackground() { return bg_defined; }
	int		getBold() { return bold; }
	int		getItalic() { return italic; }
	int		getUnderlined() { return underlined; }

	void	setFontFace(string font) { this->font = font; }
	void	setFontSize(int size) { this->size = size; }
	void	setBold(int bold) { this->bold = bold; }
	void	setItalic(int italic) { this->italic = italic; }
	void	setUnderlined(int underlined) { this->underlined = underlined; }
	void	setForeground(rgba_t col) { foreground.set(col); fg_defined = true; }
	void	clearForeground() { fg_defined = false; }
	void	setBackground(rgba_t col) { background.set(col); bg_defined = true; }
	void	clearBackground() { bg_defined = false; }

	wxFont	getFont();
	rgba_t	getForeground() { return foreground; }
	rgba_t	getBackground() { return background; }

	bool	parse(ParseTreeNode* node);
	void	applyTo(wxStyledTextCtrl* stc);
	bool	copyStyle(TextStyle* copy);
	string	getDefinition(unsigned tabs = 0);
};

class StyleSet
{
private:
	string		name;
	TextStyle	ts_default;
	TextStyle	ts_selection;

	vector<TextStyle*>	styles;

public:
	StyleSet(string name = "Unnamed Style");
	~StyleSet();

	string		getName() { return name; }
	unsigned	nStyles() { return styles.size(); }

	bool		parseSet(ParseTreeNode* root);
	void		applyTo(wxStyledTextCtrl* stc);
	bool		copySet(StyleSet* copy);
	TextStyle*	getStyle(string name);
	TextStyle*	getStyle(unsigned index);
	bool		writeFile(string filename);

	// Static functions for styleset management
	static void			initCurrent();
	static void			saveCurrent();
	static StyleSet*	currentSet();
	static bool			loadSet(string name);
	static bool			loadSet(unsigned index);
	static void			applyCurrent(wxStyledTextCtrl* stc);
	static string		getName(unsigned index);
	static unsigned		numSets();
	static StyleSet*	getSet(unsigned index);

	static bool			loadResourceStyles();
	static bool			loadCustomStyles();
};

#endif//__TEXT_STYLE_H__
