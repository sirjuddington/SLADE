#pragma once

class ClipboardItem
{
public:
	enum class Type
	{
		EntryTree,
		CompositeTexture,
		Patch,
		MapArchitecture,
		MapThings,
		Unknown
	};

	ClipboardItem(Type type = Type::Unknown) : type_{ type } {}
	virtual ~ClipboardItem() = default;

	Type type() const { return type_; }

private:
	Type type_;
};

class Clipboard
{
public:
	Clipboard()  = default;
	~Clipboard() = default;

	unsigned size() const { return items_.size(); }
	bool     empty() const { return items_.empty(); }

	ClipboardItem* item(unsigned index) const { return index < items_.size() ? items_[index].get() : nullptr; }

	void add(unique_ptr<ClipboardItem> item) { items_.push_back(std::move(item)); }
	void add(vector<unique_ptr<ClipboardItem>>& items)
	{
		for (auto& item : items)
			items_.push_back(std::move(item));
	}

	void clear() { items_.clear(); }

private:
	vector<unique_ptr<ClipboardItem>> items_;
};
