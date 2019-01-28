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
		const string&    type        = "");
	~ParseTreeNode() = default;

	string name() override { return name_; }
	void   setName(string name) override { this->name_ = name; }

	const string&           nameRef() const { return name_; }
	const string&           inherit() const { return inherit_; }
	const string&           type() const { return type_; }
	const vector<Property>& values() const { return values_; }

	bool nameIs(const string& name) const { return name_ == name; }
	bool nameIsCI(const string& name) const { return S_CMPNOCASE(name_, name); }

	size_t         nValues() const { return values_.size(); }
	Property       value(unsigned index = 0);
	string         stringValue(unsigned index = 0);
	vector<string> stringValues();
	int            intValue(unsigned index = 0);
	bool           boolValue(unsigned index = 0);
	double         floatValue(unsigned index = 0);

	// To avoid need for casts everywhere
	ParseTreeNode* childPTN(const string& name) { return dynamic_cast<ParseTreeNode*>(child(name)); }
	ParseTreeNode* childPTN(unsigned index) { return dynamic_cast<ParseTreeNode*>(child(index)); }

	ParseTreeNode* addChildPTN(const string& name, const string& type = "");
	void           addStringValue(const string& value) { values_.emplace_back(value); }
	void           addIntValue(int value) { values_.emplace_back(value); }
	void           addBoolValue(bool value) { values_.emplace_back(value); }
	void           addFloatValue(double value) { values_.emplace_back(value); }

	bool parse(Tokenizer& tz);
	void write(string& out, int indent = 0) const;

	typedef std::unique_ptr<ParseTreeNode> UPtr;

protected:
	STreeNode* createChild(string name) override
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
	ArchiveTreeNode* archive_dir_ = nullptr;

	void logError(const Tokenizer& tz, const string& error) const;
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

	bool parseText(MemChunk& mc, const string& source = "memory chunk") const;
	bool parseText(const string& text, const string& source = "string") const;
	void define(const string& def) { defines_.push_back(def.Lower()); }
	bool defined(const string& def) { return VECTOR_EXISTS(defines_, def.Lower()); }

	// To simplify casts from STreeNode to ParseTreeNode
	static ParseTreeNode* node(STreeNode* node) { return dynamic_cast<ParseTreeNode*>(node); }

private:
	ParseTreeNode::UPtr pt_root_;
	vector<string>      defines_;
	ArchiveTreeNode*    archive_dir_root_ = nullptr;
	bool                case_sensitive_   = false;
};
