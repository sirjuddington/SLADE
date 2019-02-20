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

	vector<std::string>     tokens;
	vector<ParsedStatement> block;

	bool parse(Tokenizer& tz);
	void dump(int indent = 0);
};

class Enumerator
{
public:
	Enumerator(std::string_view name = "") : name_{ name } {}

	struct Value
	{
		std::string name;
		int      value;
	};

	bool parse(ParsedStatement& statement);

private:
	std::string   name_;
	vector<Value> values_;
};

class Identifier // rename this
{
public:
	Identifier(std::string_view name = "") : name_{ name } {}
	virtual ~Identifier() = default;

	const std::string& name() const { return name_; }
	bool     native() const { return native_; }
	const std::string& deprecated() const { return deprecated_; }
	const std::string& version() const { return version_; }

protected:
	std::string name_;
	bool     native_ = false;
	std::string deprecated_;
	std::string version_;
};

class Variable : public Identifier
{
public:
	Variable(std::string_view name = "") : Identifier(name), type_{ "<unknown>" } {}
	virtual ~Variable() = default;

private:
	std::string type_;
};

class Function : public Identifier
{
public:
	Function(std::string_view name = "") : Identifier(name), return_type_{ "void" } {}

	virtual ~Function() = default;

	struct Parameter
	{
		std::string name;
		std::string type;
		std::string default_value;
		Parameter() : name{ "<unknown>" }, type{ "<unknown>" }, default_value{ "" } {}

		unsigned parse(const vector<std::string>& tokens, unsigned start_index);
	};

	const std::string&       returnType() const { return return_type_; }
	const vector<Parameter>& parameters() const { return parameters_; }
	bool                     isVirtual() const { return virtual_; }
	bool                     isStatic() const { return static_; }
	bool                     isAction() const { return action_; }
	bool                     isOverride() const { return override_; }

	bool     parse(ParsedStatement& statement);
	std::string asString();

	static bool isFunction(ParsedStatement& statement);

private:
	vector<Parameter> parameters_;
	std::string       return_type_;
	bool              virtual_  = false;
	bool              static_   = false;
	bool              action_   = false;
	bool              override_ = false;
};

struct State
{
	struct Frame
	{
		std::string sprite_base;
		std::string sprite_frame;
		int      duration;
	};

	std::string editorSprite();

	vector<Frame> frames;
};

class StateTable
{
public:
	StateTable() = default;

	const std::string& firstState() const { return state_first_; }

	bool     parse(ParsedStatement& states);
	std::string editorSprite();

private:
	std::map<std::string, State> states_;
	std::string                  state_first_;
};

class Class : public Identifier
{
public:
	enum class Type
	{
		Class,
		Struct
	};

	Class(Type type, std::string_view name = "") : Identifier{ name }, type_{ type } {}
	virtual ~Class() = default;

	const vector<Function>& functions() const { return functions_; }

	bool parse(ParsedStatement& class_statement);
	bool extend(ParsedStatement& block);
	void toThingType(std::map<int, Game::ThingType>& types, vector<Game::ThingType>& parsed);

private:
	Type               type_;
	std::string        inherits_class_;
	vector<Variable>   variables_;
	vector<Function>   functions_;
	vector<Enumerator> enumerators_;
	PropertyList       default_properties_;
	StateTable         states_;

	vector<std::pair<std::string, std::string>> db_properties_;

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
