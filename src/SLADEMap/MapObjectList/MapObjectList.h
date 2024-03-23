#pragma once

namespace slade
{
class MapObject;

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

	// Modification
	virtual void add(T* object)
	{
		objects_.push_back(object);
		++count_;
	}
	virtual void remove(unsigned index)
	{
		if (index < count_)
		{
			objects_[index] = objects_.back();
			objects_.pop_back();
			--count_;
		}
	}
	virtual void removeLast()
	{
		objects_.pop_back();
		--count_;
	}

	// Misc
	/*void putModifiedObjects(long since, vector<MapObject*>& modified_objects) const
	{
		for (const auto& object : objects_)
			if (object->modifiedTime() >= since)
				modified_objects.push_back(object);
	}
	bool modifiedSince(long since) const
	{
		for (const auto& object : objects_)
			if (object->modifiedTime() > since)
				return true;

		return false;
	}*/

protected:
	vector<T*> objects_;
	unsigned   count_ = 0;
};
} // namespace slade
