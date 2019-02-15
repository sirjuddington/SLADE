#pragma once

#include "PropertyList/Property.h"
#include "Tree.h"

class ArchiveTreeNode;
class Parser;
class Tokenizer;

class ParseTreeNode : public STreeNode
{
public:
	ParseTreeNode(
		ParseTreeNode*   parent      = nullptr,
		Parser*          parser      = nullptr,
		ArchiveTreeNode* archive_dir = nullptr,
		std::string_view type        = "");
	~ParseTreeNode() = default;

	const std::string& name() const override { return name_; }
	void               setName(std::string_view name) override { this->name_ = name; }

	const std::string&      inherit() const { return inherit_; }
	const std::string&      type() const { return type_; }
	const vector<Property>& values() const { return values_; }

	bool nameIs(std::string_view name) const { return name_ == name; }
	bool nameIsCI(std::string_view name) const;

	size_t              nValues() const { return values_.size(); }
	Property            value(unsigned index = 0);
	std::string         stringValue(unsigned index = 0);
	vector<std::string> stringValues();
	int                 intValue(unsigned index = 0);
	bool                boolValue(unsigned index = 0);
	double              floatValue(unsigned index = 0);

	// To avoid need for casts everywhere
	ParseTreeNode* childPTN(std::string_view name) { return dynamic_cast<ParseTreeNode*>(child(name)); }
	ParseTreeNode* childPTN(unsigned index) { return dynamic_cast<ParseTreeNode*>(child(index)); }

	ParseTreeNode* addChildPTN(std::string_view name, std::string_view type = "");
	void           addStringValue(std::string_view value) { values_.emplace_back(std::string{ value }); }
	void           addIntValue(int value) { values_.emplace_back(value); }
	void           addBoolValue(bool value) { values_.emplace_back(value); }
	void           addFloatValue(double value) { values_.emplace_back(value); }

	bool parse(Tokenizer& tz);
	void write(std::string& out, int indent = 0) const;

	typedef std::unique_ptr<ParseTreeNode> UPtr;

protected:
	STreeNode* createChild(std::string_view name) override
	{
		auto ret = new ParseTreeNode();
		ret->setName(name);
		ret->parser_ = parser_;
		return ret;
	}

private:
	std::string      name_;
	std::string      inherit_;
	std::string      type_;
	vector<Property> values_;
	Parser*          parser_      = nullptr;
	ArchiveTreeNode* archive_dir_ = nullptr;

	void logError(const Tokenizer& tz, std::string_view error) const;
	bool parsePreprocessor(Tokenizer& tz);
	bool parseAssignment(Tokenizer& tz, ParseTreeNode* child) const;
};

class Parser
{
public:
	Parser(ArchiveTreeNode* dir_root = nullptr);
	~Parser() = default;

	ParseTreeNode* parseTreeRoot() const { return pt_root_.get(); }

	void setCaseSensitive(bool cs) { case_sensitive_ = cs; }

	bool parseText(MemChunk& mc, std::string_view source = "memory chunk") const;
	bool parseText(std::string_view text, std::string_view source = "string") const;
	void define(std::string_view def);
	bool defined(std::string_view def);

	// To simplify casts from STreeNode to ParseTreeNode
	static ParseTreeNode* node(STreeNode* node) { return dynamic_cast<ParseTreeNode*>(node); }

private:
	ParseTreeNode::UPtr pt_root_;
	vector<std::string> defines_;
	ArchiveTreeNode*    archive_dir_root_ = nullptr;
	bool                case_sensitive_   = false;
};
