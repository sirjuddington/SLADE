
#ifndef __PARSER_H__
#define __PARSER_H__

#include "Tree.h"
#include "Tokenizer.h"
#include "PropertyList/Property.h"

class ArchiveTreeNode;
class Parser;

class ParseTreeNode : public STreeNode
{
public:
	ParseTreeNode(
		ParseTreeNode* parent = nullptr,
		Parser* parser = nullptr,
		ArchiveTreeNode* archive_dir = nullptr,
		string type = ""
	);
	~ParseTreeNode();

	string	getName() override { return name_; }
	void	setName(string name) override { this->name_ = name; }

	const string&			inherit() const { return inherit_; }
	const string&			type() const { return type_; }
	const vector<Property>&	values() const { return values_; }

	unsigned		nValues() const { return values_.size(); }
	Property		value(unsigned index = 0);
	string			stringValue(unsigned index = 0);
	int				intValue(unsigned index = 0);
	bool			boolValue(unsigned index = 0);
	double			floatValue(unsigned index = 0);

	// To avoid need for casts everywhere
	ParseTreeNode*	getChildPTN(const string& name)
					{ return static_cast<ParseTreeNode*>(getChild(name)); }
	ParseTreeNode*	getChildPTN(unsigned index)
					{ return static_cast<ParseTreeNode*>(getChild(index)); }

	ParseTreeNode*	addChildPTN(const string& name, const string& type = "");
	void			addStringValue(const string& value) { values_.push_back({value}); }
	void			addIntValue(int value) { values_.push_back({value}); }
	void			addBoolValue(bool value) { values_.push_back({value}); }
	void			addFloatValue(double value) { values_.push_back({value}); }

	bool	parse(Tokenizer& tz);
	void	write(string& out, int indent = 0) const;

	typedef std::unique_ptr<ParseTreeNode> UPtr;

protected:
	STreeNode* createChild(string name) override
	{
		ParseTreeNode* ret = new ParseTreeNode();
		ret->setName(name);
		ret->parser_ = parser_;
		return ret;
	}

private:
	string				name_;
	string				inherit_;
	string				type_;
	vector<Property>	values_;
	Parser*				parser_;
	ArchiveTreeNode*	archive_dir_;
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

	ParseTreeNode*	parseTreeRoot() const { return pt_root.get(); }

	bool	parseText(MemChunk& mc, string source = "memory chunk", bool debug = false);
	bool	parseText(string& text, string source = "string", bool debug = false);
	void	define(string def);
	bool	defined(string def);

	// To simplify casts from STreeNode to ParseTreeNode
	static ParseTreeNode*	node(STreeNode* node) { return static_cast<ParseTreeNode*>(node); }
};

#endif//__PARSER_H__
