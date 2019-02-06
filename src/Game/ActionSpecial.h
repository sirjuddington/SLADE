#pragma once

#include "Args.h"

class ParseTreeNode;

namespace Game
{
enum class TagType;

class ActionSpecial
{
public:
	ActionSpecial(const wxString& name = "Unknown", const wxString& group = "");

	const wxString& name() const { return name_; }
	const wxString& group() const { return group_; }
	TagType         needsTag() const { return tagged_; }
	const ArgSpec&  argSpec() const { return args_; }
	int             number() const { return number_; }
	bool            defined() const { return number_ >= 0; }

	void setName(wxString name) { name_ = name; }
	void setGroup(wxString group) { group_ = group; }
	void setTagged(TagType tagged) { tagged_ = tagged; }
	void setNumber(int number) { number_ = number; }

	void     reset();
	void     parse(ParseTreeNode* node, Arg::SpecialMap* shared_args);
	wxString stringDesc() const;

	static const ActionSpecial& unknown() { return unknown_; }
	static const ActionSpecial& generalSwitched() { return gen_switched_; }
	static const ActionSpecial& generalManual() { return gen_manual_; }
	static void                 initGlobal();

private:
	wxString name_;
	wxString group_;
	TagType  tagged_;
	ArgSpec  args_;
	int      number_;

	static ActionSpecial unknown_;
	static ActionSpecial gen_switched_;
	static ActionSpecial gen_manual_;
};
} // namespace Game
