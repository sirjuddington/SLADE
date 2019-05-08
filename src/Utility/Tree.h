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

	STreeNode*            parent() const { return parent_; }
	virtual const string& name() const              = 0;
	virtual void          setName(string_view name) = 0;
	virtual string        path();

	unsigned                   nChildren() const { return children_.size(); }
	STreeNode*                 child(unsigned index);
	virtual STreeNode*         child(string_view name);
	virtual vector<STreeNode*> children(string_view name);
	virtual void               addChild(STreeNode* child);
	virtual STreeNode*         addChild(string_view name);
	virtual bool               removeChild(STreeNode* child);
	const vector<STreeNode*>&  allChildren() const { return children_; }

	virtual bool isLeaf() { return children_.empty(); }

protected:
	vector<STreeNode*> children_;
	STreeNode*         parent_;
	bool               allow_dup_child_;

	virtual STreeNode* createChild(string_view name) = 0;
};
