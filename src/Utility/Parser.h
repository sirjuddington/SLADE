#pragma once

#include "Tree.h"

namespace slade
{
class ArchiveDir;
class Parser;
class Tokenizer;

using Property = std::variant<bool, int, unsigned int, double, string>;

class ParseTreeNode : public STreeNode
{
public:
	ParseTreeNode(
		ParseTreeNode* parent      = nullptr,
		Parser*        parser      = nullptr,
		ArchiveDir*    archive_dir = nullptr,
		string_view    type        = "");
	~ParseTreeNode() override = default;

	const string& name() const override { return name_; }
	void          setName(string_view name) override { name_ = name; }

	const string&           inherit() const { return inherit_; }
	const string&           type() const { return type_; }
	const vector<Property>& values() const { return values_; }

	bool nameIs(string_view name) const { return name_ == name; }
	bool nameIsCI(string_view name) const;
	bool typeIs(string_view type) const { return type_ == type; }
	bool typeIsCI(string_view type) const;

	size_t         nValues() const { return values_.size(); }
	Property       value(unsigned index = 0);
	string         stringValue(unsigned index = 0) const;
	vector<string> stringValues() const;
	int            intValue(unsigned index = 0) const;
	bool           boolValue(unsigned index = 0) const;
	double         floatValue(unsigned index = 0) const;

	// To avoid need for casts everywhere
	ParseTreeNode* childPTN(string_view name) const { return dynamic_cast<ParseTreeNode*>(child(name)); }
	ParseTreeNode* childPTN(unsigned index) const { return dynamic_cast<ParseTreeNode*>(child(index)); }

	ParseTreeNode* addChildPTN(string_view name, string_view type = "");
	void           addStringValue(string_view value) { values_.emplace_back(string{ value }); }
	void           addIntValue(int value) { values_.emplace_back(value); }
	void           addBoolValue(bool value) { values_.emplace_back(value); }
	void           addFloatValue(double value) { values_.emplace_back(value); }

	bool parse(Tokenizer& tz);
	void write(string& out, int indent = 0) const;

protected:
	STreeNode* createChild(string_view name) override
	{
		auto ret = new ParseTreeNode();
		ret->setName(name);
		ret->parser_ = parser_;
		return ret;
	}

private:
	string           name_;
	string           inherit_;
	string           type_;
	vector<Property> values_;
	Parser*          parser_      = nullptr;
	ArchiveDir*      archive_dir_ = nullptr;

	void logError(const Tokenizer& tz, string_view error) const;
	bool parsePreprocessor(Tokenizer& tz);
	bool parseAssignment(Tokenizer& tz, ParseTreeNode* child) const;
};

class Parser
{
public:
	Parser(ArchiveDir* dir_root = nullptr);
	~Parser() = default;

	ParseTreeNode* parseTreeRoot() const { return pt_root_.get(); }

	void setCaseSensitive(bool cs) { case_sensitive_ = cs; }

	bool parseText(const MemChunk& mc, string_view source = "memory chunk") const;
	bool parseText(string_view text, string_view source = "string") const;
	void define(string_view def);
	bool defined(string_view def) const;

	// To simplify casts from STreeNode to ParseTreeNode
	static ParseTreeNode* node(STreeNode* node) { return dynamic_cast<ParseTreeNode*>(node); }

private:
	unique_ptr<ParseTreeNode> pt_root_;
	vector<string>            defines_;
	ArchiveDir*               archive_dir_root_ = nullptr;
	bool                      case_sensitive_   = false;
};
} // namespace slade
