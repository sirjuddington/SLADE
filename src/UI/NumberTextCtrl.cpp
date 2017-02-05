
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    NumberTextCtrl.cpp
 * Description: NumberTextCtrl class, simple text box that only
 *              allows entry of an integer, with an optional '++' or
 *              '--' in front to signify an increment/decrement
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *******************************************************************/


/*******************************************************************
 * INCLUDES
 *******************************************************************/
#include "Main.h"
#include "NumberTextCtrl.h"


/*******************************************************************
 * NUMBERTEXTCTRL CLASS FUNCTIONS
 *******************************************************************/

// TODO this class could possibly be replaced by a validator on a regular
// wxTextCtrl

/* NumberTextCtrl::NumberTextCtrl
 * NumberTextCtrl class constructor
 *******************************************************************/
NumberTextCtrl::NumberTextCtrl(wxWindow* parent, bool allow_decimal) : wxTextCtrl(parent, -1)
{
	last_point = 0;
	this->allow_decimal = allow_decimal;

	SetToolTip("Use ++ or -- to make relative changes, e.g., ++16 to increase by 16");

	// Bind events
	Bind(wxEVT_CHAR, &NumberTextCtrl::onChar, this);
	Bind(wxEVT_TEXT, &NumberTextCtrl::onChanged, this);
}

/* NumberTextCtrl::getNumber
 * Returns the number currently entered. If it's an incrememt or
 * decrement, returns [base] incremented/decremented by the number
 *******************************************************************/
int NumberTextCtrl::getNumber(int base)
{
	string val = GetValue();

	// Get integer value
	long lval;
	if (val.IsEmpty())
		return 0;
	else if (val.StartsWith("--") || val.StartsWith("++"))
		val = val.Mid(2);
	else if (val.StartsWith("+"))
		val = val.Mid(1);
	val.ToLong(&lval);

	// Return it (incremented/decremented based on [base])
	if (isIncrement())
		return base + lval;
	else if (isDecrement())
		return base - lval;
	else
		return lval;
}

/* NumberTextCtrl::getDecNumber
 * Returns the number currently entered. If it's an incrememt or
 * decrement, returns [base] incremented/decremented by the number
 *******************************************************************/
double NumberTextCtrl::getDecNumber(double base)
{
	// If decimals aren't allowed just return truncated integral value
	if (!allow_decimal)
		return getNumber(base);

	string val = GetValue();

	// Get double value
	double dval;
	if (val.IsEmpty())
		return 0;
	else if (val.StartsWith("--") || val.StartsWith("++"))
		val = val.Mid(2);
	else if (val.StartsWith("+"))
		val = val.Mid(1);
	val.ToDouble(&dval);

	// Return it (incremented/decremented based on [base])
	if (isIncrement())
		return base + dval;
	else if (isDecrement())
		return base - dval;
	else
		return dval;
}

/* NumberTextCtrl::isIncrement
 * Returns true if the entered value is an increment
 *******************************************************************/
bool NumberTextCtrl::isIncrement()
{
	return GetValue().StartsWith("++");
}

/* NumberTextCtrl::isDecrement
 * Returns true if the entered value is a decrement
 *******************************************************************/
bool NumberTextCtrl::isDecrement()
{
	return GetValue().StartsWith("--");
}

/* NumberTextCtrl::setNumber
 * Sets the text control value to [num]
 *******************************************************************/
void NumberTextCtrl::setNumber(int num)
{
	ChangeValue(S_FMT("%d", num));
}

/* NumberTextCtrl::setDecNumber
 * Sets the text control (decimal) value to [num]
 *******************************************************************/
void NumberTextCtrl::setDecNumber(double num)
{
	ChangeValue(S_FMT("%1.3f", num));
}


/*******************************************************************
 * NUMBERTEXTCTRL CLASS EVENTS
 *******************************************************************/

/* NumberTextCtrl::onChar
 * Called when a character is entered into the control
 *******************************************************************/
void NumberTextCtrl::onChar(wxKeyEvent& e)
{
	// Don't try to validate non-printable characters
	wxChar key = e.GetUnicodeKey();
	if (key == WXK_NONE || key < 32)
	{
		e.Skip();
		return;
	}

	// Check if valid numeric character
	bool valid = false;
	if (key >= '0' && key <= '9')
		valid = true;
	if (key == '-' || key == '+')
		valid = true;
	if (allow_decimal && key == '.')
		valid = true;

	if (valid)
		wxTextCtrl::OnChar(e);
}

/* NumberTextCtrl::onChanged
 * Called when the value is changed
 *******************************************************************/
void NumberTextCtrl::onChanged(wxCommandEvent& e)
{
	string new_value = GetValue();

	// Check if valid
	// Can begin with '+', '++', '-' or '--', rest has to be numeric
	bool num = false;
	bool valid = true;
	int plus = 0;
	int minus = 0;
	int decimal = 0;
	for (unsigned a = 0; a < new_value.size(); a++)
	{
		// Check for number
		if (new_value.at(a) >= '0' && new_value.at(a) <= '9')
		{
			num = true;
			continue;
		}

		// Check for +
		if (new_value.at(a) == '+')
		{
			if (num || plus == 2 || minus > 0)
			{
				// We've had a number, '-' or 2 '+' already, invalid
				valid = false;
				break;
			}
			else
				plus++;
		}

		// Check for -
		else if (new_value.at(a) == '-')
		{
			if (num || minus == 2 || plus > 0)
			{
				// We've had a number, '+' or 2 '-' already, invalid
				valid = false;
				break;
			}
			else
				minus++;
		}

		// Check for .
		else if (new_value.at(a) == '.')
		{
			if (!num || decimal > 0)
			{
				// We've already had a decimal, or no numbers yet, invalid
				valid = false;
				break;
			}
			else
				decimal++;
		}
	}

	// If invalid revert to previous value
	if (!valid)
	{
		ChangeValue(last_value);
		SetInsertionPoint(last_point);
	}
	else
	{
		last_value = new_value;
		last_point = GetInsertionPoint();
		e.Skip();
	}
}
