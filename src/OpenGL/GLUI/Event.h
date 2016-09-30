#pragma once

namespace GLUI
{
	enum class MouseBtn
	{
		None,
		Left,
		Right,
		Middle
	};

	class Widget;

	// Event info structs
	struct EventInfo
	{
		Widget*	sender;
	};
	struct MouseEventInfo : public EventInfo
	{
		int			x_pos;
		int			y_pos;
		MouseBtn	button;

		MouseEventInfo(Widget* sender, int x_pos = 0, int y_pos = 0, MouseBtn button = MouseBtn::None)
			: EventInfo{ sender }, x_pos(x_pos), y_pos(y_pos), button(button) {}
	};
	struct KeyEventInfo : public EventInfo
	{
		string	key;
		bool	mod_shift = false;
		bool	mod_ctrl = false;
		bool	mod_alt = false;
		bool	handled = false;

		KeyEventInfo(Widget* sender, string key, bool mod_shift = false, bool mod_ctrl = false, bool mod_alt = false)
			: EventInfo{ sender }, key(key), mod_shift(mod_shift), mod_ctrl(mod_ctrl), mod_alt(mod_alt) {}
	};

	typedef std::function<void(EventInfo&)> EventFunc;
	typedef std::function<void(MouseEventInfo&)> MouseEventFunc;
	typedef std::function<void(KeyEventInfo&)> KeyEventFunc;

	template <class T>
	class EventHandler
	{
	public:
		EventHandler<T>() {}

		void bind(Widget* widget, std::function<void(T)> func)
		{
			handlers[widget] = func;
		}

		void unbind(Widget* widget)
		{
			auto remove = std::find_if(
				handlers.begin(),
				handlers.end(),
				[&](auto& handler) -> bool({ return handler.first() == widget; })
			);

			if (remove != handlers.end())
				handlers.erase(remove);
		}

		void invoke(T info)
		{
			for (auto handler : handlers)
				handler.second(info);
		}

	private:
		std::map<Widget*, std::function<void(T)>>	handlers;
	};
}
