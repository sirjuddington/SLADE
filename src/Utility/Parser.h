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
		const wxString&  type        = "");
	~ParseTreeNode() = default;

	wxString name() override { return name_; }
	void     setName(wxString name) override { this->name_ = name; }

	const wxString&         nameRef() const { return name_; }
	const wxString&         inherit() const { return inherit_; }
	const wxString&         type() const { return type_; }
	const vector<Property>& values() const { return values_; }

	bool nameIs(const wxString& name) const { return name_ == name; }
	bool nameIsCI(const wxString& name) const { return S_CMPNOCASE(name_, name); }

	size_t           nValues() const { return values_.size(); }
	Property         value(unsigned index = 0);
	wxString         stringValue(unsigned index = 0);
	vector<wxString> stringValues();
	int              intValue(unsigned index = 0);
	bool             boolValue(unsigned index = 0);
	double           floatValue(unsigned index = 0);

	// To avoid need for casts everywhere
	ParseTreeNode* childPTN(const wxString& name) { return dynamic_cast<ParseTreeNode*>(child(name)); }
	ParseTreeNode* childPTN(unsigned index) { return dynamic_cast<ParseTreeNode*>(child(index)); }

	ParseTreeNode* addChildPTN(const wxString& name, const wxString& type = "");
	void           addStringValue(const wxString& value) { values_.emplace_back(value); }
	void           addIntValue(int value) { values_.emplace_back(value); }
	void           addBoolValue(bool value) { values_.emplace_back(value); }
	void           addFloatValue(double value) { values_.emplace_back(value); }

	bool parse(Tokenizer& tz);
	void write(wxString& out, int indent = 0) const;

	typedef std::unique_ptr<ParseTreeNode> UPtr;

protected:
	STreeNode* createChild(wxString name) override
	{
		auto ret = new ParseTreeNode();
		ret->setName(name);
		ret->parser_ = parser_;
		return ret;
	}

private:
	wxString         name_;
	wxString         inherit_;
	wxString         type_;
	vector<Property> values_;
	Parser*          parser_      = nullptr;
	ArchiveTreeNode* archive_dir_ = nullptr;

	void logError(const Tokenizer& tz, const wxString& error) const;
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

	bool parseText(MemChunk& mc, const wxString& source = "memory chunk") const;
	bool parseText(const wxString& text, const wxString& source = "string") const;
	void define(const wxString& def) { defines_.push_back(def.Lower()); }
	bool defined(const wxString& def) { return VECTOR_EXISTS(defines_, def.Lower()); }

	// To simplify casts from STreeNode to ParseTreeNode
	static ParseTreeNode* node(STreeNode* node) { return dynamic_cast<ParseTreeNode*>(node); }

private:
	ParseTreeNode::UPtr pt_root_;
	vector<wxString>    defines_;
	ArchiveTreeNode*    archive_dir_root_ = nullptr;
	bool                case_sensitive_   = false;
};
