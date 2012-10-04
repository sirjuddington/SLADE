
#include "Main.h"
#include "ColourConfiguration.h"
#include "Parser.h"
#include "ArchiveManager.h"
#include "Console.h"
#include <wx/hashmap.h>

WX_DECLARE_STRING_HASH_MAP(cc_col_t, ColourHashMap);
ColourHashMap	cc_colours;

rgba_t ColourConfiguration::getColour(string name) {
	cc_col_t& col = cc_colours[name];
	if (col.exists)
		return col.colour;
	else
		return COL_WHITE;
}

cc_col_t ColourConfiguration::getColDef(string name) {
	return cc_colours[name];
}

void ColourConfiguration::setColour(string name, int red, int green, int blue, int alpha, int blend) {
	cc_col_t& col = cc_colours[name];
	if (red >= 0)
		col.colour.r = red;
	if (green >= 0)
		col.colour.g = green;
	if (blue >= 0)
		col.colour.b = blue;
	if (alpha >= 0)
		col.colour.a = alpha;
	if (blend >= 0)
		col.colour.blend = blend;
	col.exists = true;
}

bool ColourConfiguration::readConfiguration(MemChunk& mc) {
	// Parse text
	Parser parser;
	parser.parseText(mc);

	// Get 'colours' block
	ParseTreeNode* colours = (ParseTreeNode*)parser.parseTreeRoot()->getChild("colours");
	if (!colours)
		return false;

	// Read all colour definitions
	for (unsigned a = 0; a < colours->nChildren(); a++) {
		ParseTreeNode* def = (ParseTreeNode*)colours->getChild(a);

		// Read properties
		for (unsigned b = 0; b < def->nChildren(); b++) {
			ParseTreeNode* prop = (ParseTreeNode*)def->getChild(b);
			cc_col_t& col = cc_colours[def->getName()];
			col.exists = true;

			// Colour name
			if (prop->getName() == "name")
				col.name = prop->getStringValue();

			// Colour group (for config ui)
			else if (prop->getName() == "group")
				col.group = prop->getStringValue();

			// Colour
			else if (prop->getName() == "rgb")
				col.colour.set(prop->getIntValue(0), prop->getIntValue(1), prop->getIntValue(2));

			// Alpha
			else if (prop->getName() == "alpha")
				col.colour.a = prop->getIntValue();

			// Additive
			else if (prop->getName() == "additive") {
				if (prop->getBoolValue())
					col.colour.blend = 1;
				else
					col.colour.blend = 0;
			}

			else
				wxLogMessage("Warning: unknown colour definition property \"%s\"", CHR(prop->getName()));
		}
	}

	return true;
}

bool ColourConfiguration::writeConfiguration(MemChunk& mc) {
	string cfgstring = "colours\n{\n";

	ColourHashMap::iterator i = cc_colours.begin();

	// Go through all properties
	while (i != cc_colours.end()) {
		// Skip if it doesn't 'exist'
		cc_col_t cc = i->second;
		if (!cc.exists) {
			i++;
			continue;
		}

		// Colour definition name
		cfgstring += S_FMT("\t%s\n\t{\n", CHR(i->first));

		// Full name
		cfgstring += S_FMT("\t\tname = \"%s\";\n", CHR(cc.name));

		// Group
		cfgstring += S_FMT("\t\tgroup = \"%s\";\n", CHR(cc.group));

		// Colour values
		cfgstring += S_FMT("\t\trgb = %d, %d, %d;\n", cc.colour.r, cc.colour.g, cc.colour.b);

		// Alpha
		if (cc.colour.a < 255)
			cfgstring += S_FMT("\t\talpha = %d;\n", cc.colour.a);

		// Additive
		if (cc.colour.blend == 1)
			cfgstring += "\t\tadditive = true;\n";

		cfgstring += "\t}\n\n";

		// Next colour
		i++;
	}

	cfgstring += "}\n";

	mc.write(cfgstring.ToAscii(), cfgstring.size());

	return true;
}

bool ColourConfiguration::init() {
	// Load default configuration
	loadDefaults();

	// Check for saved colour configuration
	if (wxFileExists(appPath("colours.cfg", DIR_USER))) {
		MemChunk ccfg;
		ccfg.importFile(appPath("colours.cfg", DIR_USER));
		readConfiguration(ccfg);
	}

	return true;
}

void ColourConfiguration::loadDefaults() {
	// Read default colours
	Archive* pres = theArchiveManager->programResourceArchive();
	ArchiveEntry* entry_default_cc = pres->entryAtPath("config/colours/default.txt");
	if (entry_default_cc)
		readConfiguration(entry_default_cc->getMCData());
}

bool ColourConfiguration::readConfiguration(string name) {
	// TODO: search custom folder

	// Search resource pk3
	Archive* res = theArchiveManager->programResourceArchive();
	ArchiveTreeNode* dir = res->getDir("config/colours");
	for (unsigned a = 0; a < dir->numEntries(); a++) {
		if (S_CMPNOCASE(dir->getEntry(a)->getName(true), name))
			return readConfiguration(dir->getEntry(a)->getMCData());
	}

	return false;
}

void ColourConfiguration::getConfigurationNames(vector<string>& names) {
	// TODO: search custom folder

	// Search resource pk3
	Archive* res = theArchiveManager->programResourceArchive();
	ArchiveTreeNode* dir = res->getDir("config/colours");
	for (unsigned a = 0; a < dir->numEntries(); a++)
		names.push_back(dir->getEntry(a)->getName(true));
}

void ColourConfiguration::getColourNames(vector<string>& list) {
	ColourHashMap::iterator i = cc_colours.begin();
	while (i != cc_colours.end()) {
		list.push_back(i->first);
		i++;
	}
}




CONSOLE_COMMAND(ccfg, 1) {
	// Check for 'list'
	if (S_CMPNOCASE(args[0], "list")) {
		// Get (sorted) list of colour names
		vector<string> list;
		ColourConfiguration::getColourNames(list);
		sort(list.begin(), list.end());

		// Dump list to console
		theConsole->logMessage(S_FMT("%d Colours:", list.size()));
		for (unsigned a = 0; a < list.size(); a++)
			theConsole->logMessage(list[a]);
	}
	else {
		// Check for enough args to set the colour
		if (args.size() >= 4) {
			// Read RGB
			long red, green, blue;
			args[1].ToLong(&red);
			args[2].ToLong(&green);
			args[3].ToLong(&blue);

			// Read alpha (if specified)
			long alpha = -1;
			if (args.size() >= 5)
				args[4].ToLong(&alpha);

			// Read blend (if specified)
			long blend = -1;
			if (args.size() >= 6)
				args[5].ToLong(&blend);

			// Set colour
			ColourConfiguration::setColour(args[0], red, green, blue, alpha, blend);
		}

		// Print colour
		rgba_t col = ColourConfiguration::getColour(args[0]);
		theConsole->logMessage(S_FMT("Colour \"%s\" = %d %d %d %d %d", CHR(args[0]), col.r, col.g, col.b, col.a, col.blend));
	}
}

CONSOLE_COMMAND(load_ccfg, 1) {
	ColourConfiguration::readConfiguration(args[0]);
}
