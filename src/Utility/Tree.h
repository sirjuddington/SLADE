#pragma once

// Some notes:
// - createChild should simply create a STreeNode of the derived type, NOT set its parent
//   (via the constructor or otherwise)
// - Deleting a STreeNode will not remove it from its parent, this must be done manually

class STreeNode
{
public:
	STreeNode(STreeNode* parent);
	virtual ~STreeNode();

	void allowDup(bool dup) { allow_dup_child_ = dup; }
	bool allowDup() const { return allow_dup_child_; }

	STreeNode*       parent() const { return parent_; }
	virtual wxString name()                 = 0;
	virtual void     setName(wxString name) = 0;
	virtual wxString path();

	unsigned                   nChildren() const { return children_.size(); }
	STreeNode*                 child(unsigned index);
	virtual STreeNode*         child(wxString name);
	virtual vector<STreeNode*> children(wxString name);
	virtual void               addChild(STreeNode* child);
	virtual STreeNode*         addChild(wxString name);
	virtual bool               removeChild(STreeNode* child);
	const vector<STreeNode*>&  allChildren() const { return children_; }

	virtual bool isLeaf() { return children_.empty(); }

protected:
	vector<STreeNode*> children_;
	STreeNode*         parent_;
	bool               allow_dup_child_;

	virtual STreeNode* createChild(wxString name) = 0;
};
