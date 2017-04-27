
#ifndef __PARSER_H__
#define __PARSER_H__

#include "Tree.h"
#include "Tokenizer.h"
#include "PropertyList/Property.h"

class ArchiveTreeNode;
class Parser;

class ParseTreeNode : public STreeNode
{
private:
	string				name;
	string				inherit;
	string				type;
	vector<Property>	values;
	Parser*				parser;
	ArchiveTreeNode*	archive_dir;

protected:
	STreeNode*	createChild(string name) override
	{
		ParseTreeNode* ret = new ParseTreeNode();
		ret->setName(name);
		ret->parser = parser;
		return ret;
	}

public:
	ParseTreeNode(
		ParseTreeNode* parent = nullptr,
		Parser* parser = nullptr,
		ArchiveTreeNode* archive_dir = nullptr
	);
	~ParseTreeNode();

	string		getName() override { return name; }
	void		setName(string name) override { this->name = name; }
	string		getInherit() { return inherit; }
	string		getType() { return type; }
	unsigned	nValues() { return values.size(); }
	Property	getValue(unsigned index = 0);
	string		getStringValue(unsigned index = 0);
	int			getIntValue(unsigned index = 0);
	bool		getBoolValue(unsigned index = 0);
	double		getFloatValue(unsigned index = 0);

	bool	parse(Tokenizer& tz);

	typedef std::unique_ptr<ParseTreeNode> UPtr;
};

class Parser
{
private:
	ParseTreeNode::UPtr	pt_root;
	vector<string>		defines;
	ArchiveTreeNode*	archive_dir_root;

public:
	Parser(ArchiveTreeNode* dir_root = nullptr);
	~Parser();

	ParseTreeNode*	parseTreeRoot() { return pt_root.get(); }

	bool	parseText(MemChunk& mc, string source = "memory chunk", bool debug = false);
	bool	parseText(string& text, string source = "string", bool debug = false);
	void	define(string def);
	bool	defined(string def);
};

#endif//__PARSER_H__
