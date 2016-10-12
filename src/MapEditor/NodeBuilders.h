
#ifndef __NODE_BUILDERS_H__
#define __NODE_BUILDERS_H__

#include "common.h"

namespace NodeBuilders
{
	struct builder_t
	{
		string			id;
		string			name;
		string			path;
		string			command;
		string			exe;
		vector<string>	options;
		vector<string>	option_desc;
	};

	void		init();
	void		addBuilderPath(string builder, string path);
	void		saveBuilderPaths(wxFile& file);
	unsigned	nNodeBuilders();
	builder_t&	getBuilder(string id);
	builder_t&	getBuilder(unsigned index);
}

#endif//__NODE_BUILDERS_H__
