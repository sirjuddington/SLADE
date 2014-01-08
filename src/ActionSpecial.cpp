
/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "ActionSpecial.h"
#include "Parser.h"
#include "GameConfiguration.h"

/*******************************************************************
 * ACTIONSPECIAL CLASS FUNCTIONS
 *******************************************************************/

ActionSpecial::ActionSpecial(string name, string group)
{
	// Init variables
	this->name = name;
	this->group = group;
	this->tagged = 0;

	// Init args
	args[0].name = "Arg1";
	args[1].name = "Arg2";
	args[2].name = "Arg3";
	args[3].name = "Arg4";
	args[4].name = "Arg5";
}

void ActionSpecial::copy(ActionSpecial* copy)
{
	// Check AS to copy was given
	if (!copy) return;

	// Copy properties
	this->name = copy->name;
	this->group = copy->group;
	this->tagged = copy->tagged;

	// Copy args
	for (unsigned a = 0; a < 5; a++)
		this->args[a] = copy->args[a];
}

void ActionSpecial::reset()
{
	// Reset variables
	name = "Unknown";
	group = "";
	tagged = 0;

	// Reset args
	for (unsigned a = 0; a < 5; a++)
	{
		args[a].name = S_FMT("Arg%d", a+1);
		args[a].type = ARGT_NUMBER;
		args[a].custom_flags.clear();
		args[a].custom_values.clear();
	}
}

void ActionSpecial::parse(ParseTreeNode* node)
{
	// Go through all child nodes/values
	ParseTreeNode* child = NULL;
	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		child = (ParseTreeNode*)node->getChild(a);
		string name = child->getName();
		int arg = -1;

		// Name
		if (S_CMPNOCASE(name, "name"))
			this->name = child->getStringValue();

		// Args
		else if (S_CMPNOCASE(name, "arg1"))
			arg = 0;
		else if (S_CMPNOCASE(name, "arg2"))
			arg = 1;
		else if (S_CMPNOCASE(name, "arg3"))
			arg = 2;
		else if (S_CMPNOCASE(name, "arg4"))
			arg = 3;
		else if (S_CMPNOCASE(name, "arg5"))
			arg = 4;

		// Tagged
		else if (S_CMPNOCASE(name, "tagged"))
		{
			string str = child->getStringValue();
			if (S_CMPNOCASE(str, "no")) this->tagged = AS_TT_NO;
			else if (S_CMPNOCASE(str, "sector")) this->tagged = AS_TT_SECTOR;
			else if (S_CMPNOCASE(str, "line")) this->tagged = AS_TT_LINE;
			else if (S_CMPNOCASE(str, "lineid")) this->tagged = AS_TT_LINEID;
			else if (S_CMPNOCASE(str, "lineid_hi5")) this->tagged = AS_TT_LINEID_HI5;
			else if (S_CMPNOCASE(str, "thing")) this->tagged = AS_TT_THING;
			else if (S_CMPNOCASE(str, "sector_back")) this->tagged = AS_TT_SECTOR_BACK;
			else if (S_CMPNOCASE(str, "sector_or_back")) this->tagged = AS_TT_SECTOR_OR_BACK;
			else if (S_CMPNOCASE(str, "sector_and_back")) this->tagged = AS_TT_SECTOR_AND_BACK;
			else if (S_CMPNOCASE(str, "line_negative")) this->tagged = AS_TT_LINE_NEGATIVE;
			else if (S_CMPNOCASE(str, "ex_1thing_2sector")) this->tagged = AS_TT_1THING_2SECTOR;
			else if (S_CMPNOCASE(str, "ex_1thing_3sector")) this->tagged = AS_TT_1THING_3SECTOR;
			else if (S_CMPNOCASE(str, "ex_1thing_2thing")) this->tagged = AS_TT_1THING_2THING;
			else if (S_CMPNOCASE(str, "ex_1thing_4thing")) this->tagged = AS_TT_1THING_4THING;
			else if (S_CMPNOCASE(str, "ex_1thing_2thing_3thing")) this->tagged = AS_TT_1THING_2THING_3THING;
			else if (S_CMPNOCASE(str, "ex_1sector_2thing_3thing_5thing")) this->tagged = AS_TT_1SECTOR_2THING_3THING_5THING;
			else if (S_CMPNOCASE(str, "ex_1lineid_2line")) this->tagged = AS_TT_1LINEID_2LINE;
			else if (S_CMPNOCASE(str, "ex_4thing")) this->tagged = AS_TT_4THING;
			else if (S_CMPNOCASE(str, "ex_5thing")) this->tagged = AS_TT_5THING;
			else if (S_CMPNOCASE(str, "ex_1line_2sector")) this->tagged = AS_TT_1LINE_2SECTOR;
			else if (S_CMPNOCASE(str, "ex_1sector_2sector")) this->tagged = AS_TT_1SECTOR_2SECTOR;
			else if (S_CMPNOCASE(str, "ex_1sector_2sector_3sector_4_sector")) this->tagged = AS_TT_1SECTOR_2SECTOR_3SECTOR_4SECTOR;
			else if (S_CMPNOCASE(str, "ex_sector_2is3_line")) this->tagged = AS_TT_SECTOR_2IS3_LINE;
			else if (S_CMPNOCASE(str, "ex_1sector_2thing")) this->tagged = AS_TT_1SECTOR_2THING;
			else
				this->tagged = child->getIntValue();
		}

		// Parse arg definition if it was one
		if (arg >= 0)
		{
			// Check for simple definition
			if (child->isLeaf())
			{
				// Set name
				args[arg].name = child->getStringValue();

				// Set description (if specified)
				if (child->nValues() > 1) args[arg].desc = child->getStringValue(1);
			}
			else
			{
				// Extended arg definition

				// Name
				ParseTreeNode* val = (ParseTreeNode*)child->getChild("name");
				if (val) args[arg].name = val->getStringValue();

				// Description
				val = (ParseTreeNode*)child->getChild("desc");
				if (val) args[arg].desc = val->getStringValue();

				// Type
				val = (ParseTreeNode*)child->getChild("type");
				string atype;
				if (val) atype = val->getStringValue();
				if (S_CMPNOCASE(atype, "yesno"))
					args[arg].type = ARGT_YESNO;
				else if (S_CMPNOCASE(atype, "noyes"))
					args[arg].type = ARGT_NOYES;
				else if (S_CMPNOCASE(atype, "angle"))
					args[arg].type = ARGT_ANGLE;
				else
					args[arg].type = ARGT_NUMBER;
			}
		}
	}
}

string ActionSpecial::getArgsString(int args[5])
{
	string ret;

	// Add each arg to the string
	for (unsigned a = 0; a < 5; a++)
	{
		// Skip if the arg name is undefined and the arg value is 0
		if (args[a] == 0 && this->args[a].name.StartsWith("Arg"))
			continue;

		ret += this->args[a].name;
		ret += ": ";
		ret += this->args[a].valueString(args[a]);
		ret += ", ";
	}

	// Cut ending ", "
	if (!ret.IsEmpty())
		ret.RemoveLast(2);

	return ret;
}

string ActionSpecial::stringDesc()
{
	// Init string
	string ret = S_FMT("\"%s\" in group \"%s\"", CHR(name), CHR(group));

	// Add tagged info
	if (tagged)
		ret += " (tagged)";
	else
		ret += " (not tagged)";

	// Add args
	ret += "\nArgs: ";
	for (unsigned a = 0; a < 5; a++)
	{
		ret += args[a].name + ": ";

		if (args[a].type == ARGT_NUMBER)
			ret += "Number";
		else if (args[a].type == ARGT_YESNO)
			ret += "Yes/No";
		else if (args[a].type == ARGT_NOYES)
			ret += "No/Yes";
		else if (args[a].type == ARGT_ANGLE)
			ret += "Angle";
		else
			ret += "Unknown Type";

		ret += ", ";
	}

	return ret;
}
