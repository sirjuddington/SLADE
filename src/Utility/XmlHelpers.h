#pragma once

class wxXmlNode;
namespace XmlHelpers
{
	wxXmlNode*	getFirstChild(wxXmlNode* node, string child_name);
	string		getContent(wxXmlNode* element_node);
}
