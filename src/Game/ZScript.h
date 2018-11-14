#pragma once

#include "Utility/PropertyList/PropertyList.h"
#include "ThingType.h"

class Archive;
class ArchiveEntry;
class Tokenizer;

namespace ZScript
{
	struct ParsedStatement
	{
		ArchiveEntry*	entry = nullptr;
		unsigned		line;

		vector<string>			tokens;
		vector<ParsedStatement>	block;

		bool	parse(Tokenizer& tz);
		void	dump(int indent = 0);
	};

	class Enumerator
	{
	public:
		Enumerator(string name = "") : name_{ name } {}

		struct Value
		{
			string	name;
			int		value;
			//Value() : name{name}, value{0} {}
		};

		bool parse(ParsedStatement& statement);

	private:
		string			name_;
		vector<Value>	values_;
	};

	class Identifier	// rename this
	{
	public:
		Identifier(string name = "") : name_{ name }, native_{ false } {}
		virtual ~Identifier() {}

		string	name() const { return name_; }
		bool	native() const { return native_; }
		string	deprecated() const { return deprecated_; }
		string	version() const { return version_; }

	protected:
		string	name_;
		bool	native_		= false;
		string	deprecated_;
		string	version_;
	};

	class Variable : public Identifier
	{
	public:
		Variable(string name = "") : Identifier(name), type_{ "<unknown>" } {}
		virtual ~Variable() {}

	private:
		string	type_;
	};

	class Function : public Identifier
	{
	public:
		Function(string name = "") : Identifier(name), return_type_{ "void" } {}
		
		virtual ~Function() {}

		struct Parameter
		{
			string name;
			string type;
			string default_value;
			Parameter() : name{ "<unknown>" }, type{ "<unknown>" }, default_value{ "" } {}

			unsigned parse(const vector<string>& tokens, unsigned start_index);
		};

		const string&				returnType() const { return return_type_; }
		const vector<Parameter>&	parameters() const { return parameters_; }
		bool						isVirtual() const { return virtual_; }
		bool						isStatic() const { return static_; }
		bool						isAction() const { return action_; }
		bool						isOverride() const { return override_; }

		bool	parse(ParsedStatement& statement);
		string	asString();

		static bool isFunction(ParsedStatement& block);

	private:
		vector<Parameter>	parameters_;
		string				return_type_;
		bool				virtual_	= false;
		bool				static_		= false;
		bool				action_		= false;
		bool				override_	= false;
	};

	struct State
	{
		struct Frame
		{
			string	sprite_base;
			string	sprite_frame;
			int		duration;
		};

		string editorSprite();

		vector<Frame>	frames;
	};

	class StateTable
	{
	public:
		StateTable() {}

		const string&	firstState() const { return state_first_; }

		bool	parse(ParsedStatement& states);
		string	editorSprite();

	private:
		std::map<string, State>	states_;
		string					state_first_;
	};

	class Class : public Identifier
	{
	public:
		enum class Type
		{
			Class,
			Struct
		};

		Class(Type type, string name = "") : Identifier{ name }, type_{ type } {}
		virtual ~Class() {}

		const vector<Function>&	functions() const { return functions_; }

		bool	parse(ParsedStatement& block);
		bool	extend(ParsedStatement& block);
		void	toThingType(std::map<int, Game::ThingType>& types, vector<Game::ThingType>& parsed);

	private:
		Type				type_;
		string				inherits_class_;
		vector<Variable>	variables_;
		vector<Function>	functions_;
		vector<Enumerator>	enumerators_;
		PropertyList		default_properties_;
		StateTable			states_;

		vector<std::pair<string, string>>	db_properties_;

		bool	parseClassBlock(vector<ParsedStatement>& block);
		bool	parseDefaults(vector<ParsedStatement>& defaults);
	};

	class Definitions	// rename this also
	{
	public:
		Definitions() {}
		~Definitions() {}

		const vector<Class>&	classes() const { return classes_; }

		void	clear();
		bool	parseZScript(ArchiveEntry* entry);
		bool	parseZScript(Archive* archive);

		void	exportThingTypes(std::map<int, Game::ThingType>& types, vector<Game::ThingType>& parsed);

	private:
		vector<Class>		classes_;
		vector<Enumerator>	enumerators_;
		vector<Variable>	variables_;
		vector<Function>	functions_;	// needed? dunno if global functions are a thing
	};
}
