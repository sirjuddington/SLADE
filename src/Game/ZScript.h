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
		vector<string>			tokens;
		vector<ParsedStatement>	block;

		bool	parse(Tokenizer& tz, bool first_expression = false);
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
		Identifier(string name = "") : name_{ name }, native_{ false }, deprecated_{ false } {}
		virtual ~Identifier() {}

		string	name() const { return name_; }
		bool	native() const { return native_; }
		bool	deprecated() const { return deprecated_; }

	protected:
		string	name_;
		bool	native_		= false;
		bool	deprecated_	= false;
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
		};

		bool	parse(ParsedStatement& statement);

		static bool isFunction(ParsedStatement& block);

	private:
		vector<Parameter>	parameters_;
		string				return_type_;
		bool				virtual_	= false;
		bool				static_		= false;
		bool				action_		= false;
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

		bool	parse(ParsedStatement& block);
		void	toThingType(std::map<int, Game::ThingType>& types, vector<Game::ThingType>& parsed);

	private:
		Type				type_;
		string				inherits_class_;
		vector<Variable>	variables_;
		vector<Function>	functions_;
		vector<Enumerator>	enumerators_;
		PropertyList		default_properties_;

		vector<std::pair<string, string>>	db_properties_;

		bool parseDefaults(vector<ParsedStatement>& defaults);
	};

	class Definitions	// rename this also
	{
	public:
		Definitions() {}
		~Definitions() {}

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
