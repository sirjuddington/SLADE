
#include "Main.h"
#include "XmlHelpers.h"
#include <wx/xml/xml.h>

wxXmlNode* XmlHelpers::getFirstChild(wxXmlNode* node, string child_name)
{
	wxXmlNode* child = node->GetChildren();
	while (child)
		if (child->GetName() == child_name)
			return child;
		else
			child = child->GetNext();

	return NULL;
}

string XmlHelpers::getContent(wxXmlNode* node)
{
	// If it's a text node just return the content
	if (node->GetType() == wxXML_TEXT_NODE)
		return node->GetContent();

	// If it's an attribute node, return the content of the first child node that is text
	else
	{
		wxXmlNode* child = node->GetChildren();
		while (child)
			if (child->GetType() == wxXML_TEXT_NODE)
				return child->GetContent();
			else
				child = child->GetNext();
	}

	return "";
}
