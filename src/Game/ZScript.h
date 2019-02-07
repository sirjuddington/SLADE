#pragma once

#include "ThingType.h"
#include "Utility/PropertyList/PropertyList.h"

class Archive;
class ArchiveEntry;
class Tokenizer;

namespace ZScript
{
struct ParsedStatement
{
	ArchiveEntry* entry = nullptr;
	unsigned      line;

	vector<wxString>        tokens;
	vector<ParsedStatement> block;

	bool parse(Tokenizer& tz);
	void dump(int indent = 0);
};

class Enumerator
{
public:
	Enumerator(const wxString& name = "") : name_{ name } {}

	struct Value
	{
		wxString name;
		int      value;
	};

	bool parse(ParsedStatement& statement);

private:
	wxString      name_;
	vector<Value> values_;
};

class Identifier // rename this
{
public:
	Identifier(const wxString& name = "") : name_{ name } {}
	virtual ~Identifier() = default;

	wxString name() const { return name_; }
	bool     native() const { return native_; }
	wxString deprecated() const { return deprecated_; }
	wxString version() const { return version_; }

protected:
	wxString name_;
	bool     native_ = false;
	wxString deprecated_;
	wxString version_;
};

class Variable : public Identifier
{
public:
	Variable(const wxString& name = "") : Identifier(name), type_{ "<unknown>" } {}
	virtual ~Variable() = default;

private:
	wxString type_;
};

class Function : public Identifier
{
public:
	Function(const wxString& name = "") : Identifier(name), return_type_{ "void" } {}

	virtual ~Function() = default;

	struct Parameter
	{
		wxString name;
		wxString type;
		wxString default_value;
		Parameter() : name{ "<unknown>" }, type{ "<unknown>" }, default_value{ "" } {}

		unsigned parse(const vector<wxString>& tokens, unsigned start_index);
	};

	const wxString&          returnType() const { return return_type_; }
	const vector<Parameter>& parameters() const { return parameters_; }
	bool                     isVirtual() const { return virtual_; }
	bool                     isStatic() const { return static_; }
	bool                     isAction() const { return action_; }
	bool                     isOverride() const { return override_; }

	bool     parse(ParsedStatement& statement);
	wxString asString();

	static bool isFunction(ParsedStatement& statement);

private:
	vector<Parameter> parameters_;
	wxString          return_type_;
	bool              virtual_  = false;
	bool              static_   = false;
	bool              action_   = false;
	bool              override_ = false;
};

struct State
{
	struct Frame
	{
		wxString sprite_base;
		wxString sprite_frame;
		int      duration;
	};

	wxString editorSprite();

	vector<Frame> frames;
};

class StateTable
{
public:
	StateTable() = default;

	const wxString& firstState() const { return state_first_; }

	bool     parse(ParsedStatement& states);
	wxString editorSprite();

private:
	std::map<wxString, State> states_;
	wxString                  state_first_;
};

class Class : public Identifier
{
public:
	enum class Type
	{
		Class,
		Struct
	};

	Class(Type type, const wxString& name = "") : Identifier{ name }, type_{ type } {}
	virtual ~Class() = default;

	const vector<Function>& functions() const { return functions_; }

	bool parse(ParsedStatement& class_statement);
	bool extend(ParsedStatement& block);
	void toThingType(std::map<int, Game::ThingType>& types, vector<Game::ThingType>& parsed);

private:
	Type               type_;
	wxString           inherits_class_;
	vector<Variable>   variables_;
	vector<Function>   functions_;
	vector<Enumerator> enumerators_;
	PropertyList       default_properties_;
	StateTable         states_;

	vector<std::pair<wxString, wxString>> db_properties_;

	bool parseClassBlock(vector<ParsedStatement>& block);
	bool parseDefaults(vector<ParsedStatement>& defaults);
};

class Definitions // rename this also
{
public:
	Definitions()  = default;
	~Definitions() = default;

	const vector<Class>& classes() const { return classes_; }

	void clear();
	bool parseZScript(ArchiveEntry* entry);
	bool parseZScript(Archive* archive);

	void exportThingTypes(std::map<int, Game::ThingType>& types, vector<Game::ThingType>& parsed);

private:
	vector<Class>      classes_;
	vector<Enumerator> enumerators_;
	vector<Variable>   variables_;
	vector<Function>   functions_; // needed? dunno if global functions are a thing
};
} // namespace ZScript
