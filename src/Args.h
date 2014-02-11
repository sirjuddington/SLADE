
#ifndef __ARGS_H__
#define __ARGS_H__

enum
{
	ARGT_NUMBER = 0,
	ARGT_YESNO,
	ARGT_NOYES,
	ARGT_ANGLE,
	ARGT_CUSTOM,
};

struct arg_val_t
{
	int		value;
	string	name;
};

struct arg_t
{
	string				name;
	string				desc;
	int					type;
	vector<arg_val_t>	custom_values;
	vector<arg_val_t>	custom_flags;

	arg_t() { name = ""; type = 0; }

	string valueString(int value)
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


		// Custom
		else if (type == ARGT_CUSTOM)
		{
			for (unsigned a = 0; a < custom_values.size(); a++)
			{
				if (custom_values[a].value == value)
					return custom_values[a].name;
			}
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
