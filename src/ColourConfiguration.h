
#ifndef __COLOUR_CONFIGURATION_H__
#define __COLOUR_CONFIGURATION_H__

struct cc_col_t
{
	bool	exists;
	bool	custom;
	string	name;
	string	group;
	rgba_t	colour;
	cc_col_t() { exists = false; custom = false; }
};

namespace ColourConfiguration
{
	rgba_t		getColour(string name);
	cc_col_t	getColDef(string name);
	void		setColour(string name, int red = -1, int green = -1, int blue = -1, int alpha = -1, int blend = -1);


	bool	readConfiguration(MemChunk& mc);
	bool	writeConfiguration(MemChunk& mc);
	bool	init();
	void	loadDefaults();

	bool	readConfiguration(string name);
	void	getConfigurationNames(vector<string>& names);

	void	getColourNames(vector<string>& list);
}

#endif//__COLOUR_CONFIGURATION_H__
