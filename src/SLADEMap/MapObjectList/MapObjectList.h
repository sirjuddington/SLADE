#pragma once

namespace slade
{
template<class T> class MapObjectList
{
public:
	typedef typename vector<T*>::const_iterator const_iterator;
	typedef typename vector<T*>::iterator       iterator;
	typedef typename vector<T*>::value_type     value_type;

	virtual ~MapObjectList() = default;

	// Vector-like interface
	const_iterator begin() const { return objects_.begin(); }
	const_iterator end() const { return objects_.end(); }
	iterator       begin() { return objects_.begin(); }
	iterator       end() { return objects_.end(); }
	T*             operator[](unsigned index) { return objects_[index]; }
	T*             operator[](unsigned index) const { return objects_[index]; }
	unsigned       size() const { return count_; }
	T*             at(unsigned index) const { return index < count_ ? objects_[index] : nullptr; }
	virtual void   clear()
	{
		objects_.clear();
		count_ = 0;
	}
	T*   back() { return objects_.back(); }
	bool empty() const { return count_ == 0; }

	// Access
	const vector<T*>& all() const { return objects_; }
	T*                first() const { return objects_.empty() ? nullptr : objects_[0]; }
	T*                last() const { return objects_.empty() ? nullptr : objects_.back(); }

	// Info
	bool contains(const T* object) const
	{
		return std::find(objects_.begin(), objects_.end(), object) != objects_.end();
	}


	// Modification ------------------------------------------------------------

	// Add [object] to the list
	virtual void add(T* object)
	{
		objects_.push_back(object);
		++count_;
	}

	// Remove object at [index]
	virtual void remove(unsigned index)
	{
		if (index < count_)
		{
			objects_.erase(objects_.begin() + index);
			--count_;
		}
	}

	// Remove [object] from the list
	virtual void remove(T* object)
	{
		auto it = std::find(objects_.begin(), objects_.end(), object);
		if (it != objects_.end())
		{
			objects_.erase(it);
			--count_;
		}
	}

	// Remove objects at [indices]
	virtual void remove(const vector<unsigned>& indices)
	{
		// Null out objects to remove
		auto removed_count = 0;
		for (auto index : indices)
			if (index < count_ && objects_[index])
			{
				objects_[index] = nullptr;
				removed_count++;
			}

		// Rebuild list
		auto old = objects_;
		objects_.clear();
		objects_.resize(old.size() - removed_count);
		auto index = 0;
		for (auto& object : old)
			if (object)
				objects_[index++] = object;

		count_ = objects_.size();
	}

	// Remove last object
	virtual void removeLast()
	{
		objects_.pop_back();
		--count_;
	}

protected:
	vector<T*> objects_;
	unsigned   count_ = 0;
};
} // namespace slade
