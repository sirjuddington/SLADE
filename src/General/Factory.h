#pragma once

// Taken from https://www.nirfriedman.com/2018/04/29/unforgettable-factory/
// for self-registering factory classes

// Currently unused, was going to use it for ArchiveFormatHandlers but it didn't
// really fit the use case. Might look at using it for some other things like
// SIFormats and EntryDataFormats

#ifndef FACTORY_KEY_NAME
#define FACTORY_KEY_NAME name
#endif

namespace slade
{
template<class Base, class... Args> class Factory
{
public:
	template<class... T> static unique_ptr<Base> make(const string& id, T&&... args)
	{
		return data().at(id)(std::forward<T>(args)...);
	}

	template<class T> struct Registrar : Base
	{
		friend T;

		static bool registerT()
		{
			const auto name       = T::FACTORY_KEY_NAME;
			Factory::data()[name] = [](Args... args) -> unique_ptr<Base>
			{ return std::make_unique<T>(std::forward<Args>(args)...); };
			return true;
		}

		static bool registered;

	private:
		Registrar() { (void)registered; }
	};

	friend Base;

private:
	using FuncType = unique_ptr<Base> (*)(Args...);

	Factory() = default;

	static auto& data()
	{
		static std::unordered_map<string, FuncType> s;
		return s;
	}
};

template<class Base, class... Args>
template<class T>
bool Factory<Base, Args...>::Registrar<T>::registered = registerT();
} // namespace slade
