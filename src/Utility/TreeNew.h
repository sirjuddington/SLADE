#pragma once

template<class T>
class TreeNode
{
public:
	typedef std::unique_ptr<TreeNode<T>>	UPtr;
	typedef std::shared_ptr<TreeNode<T>>	SPtr;
	typedef std::weak_ptr<TreeNode<T>>		WPtr;

	TreeNode(TreeNode<T>* parent = nullptr) : parent_{ parent } {}

	WPtr						parent() { return parent_; }
	const vector<TreeNode<T>>&	children() const { return children_; }
	const string&				name() const { return name_; }
	T&							data() { return data_; }

	string	path();

private:
	TreeNode<T>*		parent_ = nullptr;
	vector<TreeNode<T>>	children_;
	string				name_;
	T					data_;
};
