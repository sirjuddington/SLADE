
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    NumberTextCtrl.cpp
// Description: NumberTextCtrl class, simple text box that only allows entry of
//              an integer, with an optional '++' or '--' in front to signify an
//              increment/decrement
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "NumberTextCtrl.h"


// -----------------------------------------------------------------------------
//
// NumberTextCtrl Class Functions
//
// -----------------------------------------------------------------------------

// TODO this class could possibly be replaced by a validator on a regular wxTextCtrl

// -----------------------------------------------------------------------------
// NumberTextCtrl class constructor
// -----------------------------------------------------------------------------
NumberTextCtrl::NumberTextCtrl(wxWindow* parent, bool allow_decimal) :
	wxTextCtrl(parent, -1),
	allow_decimal_{ allow_decimal }
{
	SetToolTip("Use ++, --, *, / to make relative changes, e.g., ++16 to increase by 16");

	// Bind events
	Bind(wxEVT_CHAR, &NumberTextCtrl::onChar, this);
	Bind(wxEVT_TEXT, &NumberTextCtrl::onChanged, this);
}

// -----------------------------------------------------------------------------
// Returns the number currently entered.
// If it's an incrememt or decrement, returns [base] incremented/decremented by
// the number
// -----------------------------------------------------------------------------
int NumberTextCtrl::number(int base) const
{
	string val = GetValue();

	// Get integer value
	long lval;
	if (val.IsEmpty())
		return 0;
	else if (val.StartsWith("--") || val.StartsWith("++") || val.StartsWith("**") || val.StartsWith("//"))
		val = val.Mid(2);
	else if (val.StartsWith("+") || val.StartsWith("*") || val.StartsWith("/"))
		val = val.Mid(1);
	val.ToLong(&lval);

	// Return it (incremented/decremented based on [base])
	if (isIncrement())
		return base + lval;
	else if (isDecrement())
		return base - lval;
	else if (isFactor())
		return base * lval;
	else if (isDivisor())
		return base / lval;
	else
		return lval;
}

// -----------------------------------------------------------------------------
// Returns the number currently entered.
// If it's an incrememt or decrement, returns [base] incremented/decremented by
// the number
// -----------------------------------------------------------------------------
double NumberTextCtrl::decNumber(double base) const
{
	// If decimals aren't allowed just return truncated integral value
	if (!allow_decimal_)
		return number(base);

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

// -----------------------------------------------------------------------------
// Returns true if the entered value is an increment
// -----------------------------------------------------------------------------
bool NumberTextCtrl::isIncrement() const
{
	return GetValue().StartsWith("++");
}

// -----------------------------------------------------------------------------
// Returns true if the entered value is a decrement
// -----------------------------------------------------------------------------
bool NumberTextCtrl::isDecrement() const
{
	return GetValue().StartsWith("--");
}

// -----------------------------------------------------------------------------
// Returns true if the entered value is a multiplication factor
// -----------------------------------------------------------------------------
bool NumberTextCtrl::isFactor() const
{
	return GetValue().StartsWith("*");
}

// -----------------------------------------------------------------------------
// Returns true if the entered value is a divisor
// -----------------------------------------------------------------------------
bool NumberTextCtrl::isDivisor() const
{
	return GetValue().StartsWith("/");
}

// -----------------------------------------------------------------------------
// Sets the text control value to [num]
// -----------------------------------------------------------------------------
void NumberTextCtrl::setNumber(int num)
{
	ChangeValue(S_FMT("%d", num));
}

// -----------------------------------------------------------------------------
// Sets the text control (decimal) value to [num]
// -----------------------------------------------------------------------------
void NumberTextCtrl::setDecNumber(double num)
{
	ChangeValue(S_FMT("%1.3f", num));
}


// -----------------------------------------------------------------------------
//
// NumberTextCtrl Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when a character is entered into the control
// -----------------------------------------------------------------------------
void NumberTextCtrl::onChar(wxKeyEvent& e)
{
	// Don't try to validate non-printable characters
	auto key = e.GetUnicodeKey();
	if (key == WXK_NONE || key < 32)
	{
		e.Skip();
		return;
	}

	// Check if valid numeric character
	bool valid = false;
	if (key >= '0' && key <= '9')
		valid = true;
	if (key == '-' || key == '+' || key == '*' || key == '/')
		valid = true;
	if (allow_decimal_ && key == '.')
		valid = true;

	if (valid)
		wxTextCtrl::OnChar(e);
}

// -----------------------------------------------------------------------------
// Called when the value is changed
// -----------------------------------------------------------------------------
void NumberTextCtrl::onChanged(wxCommandEvent& e)
{
	string new_value = GetValue();

	// Check if valid
	// Can begin with '+', '++', '-' or '--', rest has to be numeric
	bool num     = false;
	bool valid   = true;
	int  plus    = 0;
	int  minus   = 0;
	int  splat   = 0;
	int  slash   = 0;
	int  decimal = 0;
	for (auto&& c : new_value)
	{
		// Check for number
		if (c >= '0' && c <= '9')
		{
			num = true;
			continue;
		}

		// Check for +
		if (c == '+')
		{
			if (num || plus == 2 || minus > 0 || splat > 0 || slash > 0)
			{
				// We've had a number, a different operator, or 2 '+' already, invalid
				valid = false;
				break;
			}
			else
				plus++;
		}

		// Check for -
		else if (c == '-')
		{
			if (num || minus == 2 || plus > 0 || splat > 0 || slash > 0)
			{
				// We've had a number, a different operator, or 2 '-' already, invalid
				valid = false;
				break;
			}
			else
				minus++;
		}

		// Check for *
		if (c == '*')
		{
			if (num || splat == 2 || plus > 0 || minus > 0 || slash > 0)
			{
				// We've had a number, a different operator, or 2 '*' already, invalid
				valid = false;
				break;
			}
			else
				splat++;
		}

		// Check for /
		if (c == '/')
		{
			if (num || slash == 2 || plus > 0 || minus > 0 || splat > 0)
			{
				// We've had a number, a different operator, or 2 '/' already, invalid
				valid = false;
				break;
			}
			else
				slash++;
		}

		// Check for .
		else if (c == '.')
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
		ChangeValue(last_value_);
		SetInsertionPoint(last_point_);
	}
	else
	{
		last_value_ = new_value;
		last_point_ = GetInsertionPoint();
		e.Skip();
	}
}
