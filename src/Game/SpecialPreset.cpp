
#include "Main.h"
#include "SpecialPreset.h"
#include "Utility/Parser.h"

using namespace Game;

void SpecialPreset::parse(ParseTreeNode* node)
{
	name = node->getName();

	for (unsigned a = 0; a < node->nChildren(); a++)
	{
		auto child = node->getChildPTN(a);
		string name = child->getName();

		// Group
		if (S_CMPNOCASE(child->getName(), "group"))
			group = child->stringValue();

		// Special
		else if (S_CMPNOCASE(child->getName(), "special"))
			special = child->intValue();

		// Flags
		else if (S_CMPNOCASE(child->getName(), "flags"))
			for (auto& flag : child->values())
				flags.push_back(flag.getStringValue());

		// Args
		else if (S_CMPNOCASE(child->getName(), "arg1"))
			args[0] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg2"))
			args[1] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg3"))
			args[2] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg4"))
			args[3] = child->intValue();
		else if (S_CMPNOCASE(child->getName(), "arg5"))
			args[4] = child->intValue();
	}
}
