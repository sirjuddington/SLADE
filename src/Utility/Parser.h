
#ifndef __PARSER_H__
#define __PARSER_H__

#include "Tree.h"
#include "PropertyList/Property.h"

class ArchiveTreeNode;
class Parser;
class Tokenizer;

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

	size_t			nValues() const { return values_.size(); }
	Property		value(unsigned index = 0);
	string			stringValue(unsigned index = 0);
	vector<string>	stringValues();
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

	void	logError(const Tokenizer& tz, const string& error) const;
	bool	parsePreprocessor(Tokenizer& tz);
	bool	parseAssignment(Tokenizer& tz, ParseTreeNode* child) const;
};

class Parser
{
public:
	Parser(ArchiveTreeNode* dir_root = nullptr);
	~Parser();

	ParseTreeNode*	parseTreeRoot() const { return pt_root_.get(); }

	void	setCaseSensitive(bool cs) { case_sensitive_ = cs; }

	bool	parseText(MemChunk& mc, string source = "memory chunk");
	bool	parseText(string& text, string source = "string");
	void	define(const string& def) { defines_.push_back(def.Lower()); }
	bool	defined(const string& def) { return VECTOR_EXISTS(defines_, def.Lower()); }

	// To simplify casts from STreeNode to ParseTreeNode
	static ParseTreeNode*	node(STreeNode* node) { return static_cast<ParseTreeNode*>(node); }

private:
	ParseTreeNode::UPtr	pt_root_;
	vector<string>		defines_;
	ArchiveTreeNode*	archive_dir_root_	= nullptr;
	bool				case_sensitive_		= false;
};

#endif//__PARSER_H__
