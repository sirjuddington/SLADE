
#ifndef __PARSER_H__
#define __PARSER_H__

#include "Tree.h"
#include "Tokenizer.h"
#include "Property.h"

class Parser;
class ParseTreeNode : public STreeNode {
private:
	string				name;
	string				inherit;
	string				type;
	vector<Property>	values;
	Parser*				parser;

protected:
	STreeNode*	createChild(string name) {
		ParseTreeNode* ret = new ParseTreeNode();
		ret->setName(name);
		ret->parser = parser;
		return ret;
	}

public:
	ParseTreeNode(ParseTreeNode* parent = NULL, Parser* parser = NULL);
	~ParseTreeNode();

	string		getName() { return name; }
	void		setName(string name) { this->name = name; }
	string		getInherit() { return inherit; }
	string		getType() { return type; }
	unsigned	nValues() { return values.size(); }
	Property	getValue(unsigned index = 0);
	string		getStringValue(unsigned index = 0);
	int			getIntValue(unsigned index = 0);
	bool		getBoolValue(unsigned index = 0);
	double		getFloatValue(unsigned index = 0);

	bool	parse(Tokenizer& tz);
};

class Parser {
private:
	ParseTreeNode*	pt_root;
	vector<string>	defines;

public:
	Parser();
	~Parser();

	ParseTreeNode*	parseTreeRoot() { return pt_root; }

	bool	parseText(MemChunk& mc, string source = "memory chunk");
	bool	parseText(string& text, string source = "string");
	void	define(string def);
	bool	defined(string def);
};

#endif//__PARSER_H__
