
#ifndef __ARGS_H__
#define __ARGS_H__

enum
{
	ARGT_NUMBER = 0,
	ARGT_YESNO,
	ARGT_NOYES,
	ARGT_ANGLE,
	ARGT_CHOICE,
	ARGT_FLAGS,
};

struct arg_val_t
{
	wxString	name;
	int			value;

	arg_val_t(wxString name, int value) : name(name), value(value) {}
};

struct arg_t
{
	wxString			name;
	wxString			desc;
	int					type;
	vector<arg_val_t>	custom_values;
	vector<arg_val_t>	custom_flags;

	arg_t() { name = ""; type = 0; }

	wxString valueString(int value)
	{
		// Yes/No
		if (type == ARGT_YESNO)
		{
			if (value > 0)
				return "Yes";
			else
				return "No";
		}

		// No/Yes
		else if (type == ARGT_NOYES)
		{
			if (value > 0)
				return "No";
			else
				return "Yes";
		}

		// Custom list of choices
		else if (type == ARGT_CHOICE)
		{
			for (unsigned a = 0; a < custom_values.size(); a++)
			{
				if (custom_values[a].value == value)
					return custom_values[a].name;
			}
		}

		// Custom list of flags
		else if (type == ARGT_FLAGS)
		{
			// This has to go in REVERSE order to correctly handle multi-bit
			// enums (so we see 3 before 1 and 2)
			vector<wxString> flags;
			size_t final_length = 0;
			int last_group = 0;
			int original_value = value;
			bool has_flag;
			for (vector<arg_val_t>::reverse_iterator it = custom_flags.rbegin();
					it != custom_flags.rend();
					it++)
			{
				if ((it->value & (it->value - 1)) != 0)
					// Not a power of two, so must be a group
					last_group = value;

				if (value == 0)
					// Zero is special: it only counts as a flag value if the
					// most recent "group" is empty
					has_flag = (last_group && (original_value & last_group) == 0);
				else
					has_flag = ((value & it->value) == it->value);

				if (has_flag)
				{
					value &= ~it->value;
					flags.push_back(it->name);
					final_length += it->name.size() + 3;
				}
			}

			if (value || !flags.size())
				flags.push_back(S_FMT("%d", value));

			// Join 'em, in reverse again, to restore the original order
			wxString out;
			out.Alloc(final_length);
			vector<wxString>::reverse_iterator it = flags.rbegin();
			while (true)
			{
				out += *it;
				it++;
				if (it == flags.rend())
					break;
				out += " + ";
			}
			return out;
		}

		// Angle
		else if (type == ARGT_ANGLE)
			return S_FMT("%d Degrees", value);	// TODO: E/S/W/N/etc

		// Any other type
		return S_FMT("%d", value);
	}
};

struct argspec_t
{
private:
	arg_t*	args;  // pointer to existing array of 5 elements

public:
	int		count;

	argspec_t(arg_t _args[5], int _count) : args(_args), count(_count) {}

	arg_t&	getArg(int index) { if (index >= 0 && index < 5) return args[index]; else return args[0]; }
};

#endif//__ARGS_H__
