
/*******************************************************************
 * SLADE - It's a Doom Editor
 * Copyright (C) 2008-2014 Simon Judd
 *
 * Email:       sirjuddington@gmail.com
 * Web:         http://slade.mancubus.net
 * Filename:    CIEDeltaEquations.cpp
 * Description: C++ implementations of CIE Delta E colour difference
 *              equations. Since they are meant for comparison of
 *              differences to find the closest match in a palette,
 *              these lack the final sqrt() of the result.
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

/* The CIEDE 2000 implementation and test data was taken from
 * "The CIEDE2000 Color-Difference Formula: Implementation Notes,
 * Supplementary Test Data, and Mathematical Observations" by
 * Gaurav Sharma, Wencheng Wu, and Edul N. Dalal; and the associated
 * Excel spreadsheet.
 * http://www.ece.rochester.edu/~gsharma/ciede2000/ciede2000noteCRNA.pdf
 * http://www.ece.rochester.edu/~gsharma/ciede2000/dataNprograms/CIEDE2000.xls
 * However, conversions between radians and degrees were avoided
 * whenever possible, contrarily to the Excel implementation.
 */

/*******************************************************************
 * INCLUDES
 *******************************************************************/

#include "CIEDeltaEquations.h"
#include "MathStuff.h"

/*******************************************************************
 * CONSTANTS
 *******************************************************************/
#define P257 6103515625.0 // 25 to the power of 7

/*******************************************************************
 * VARIABLES
 *******************************************************************/
CVAR(Float, col_cie_kl, 1.000, CVAR_SAVE)			// KL: 1.000 for graphics, 2.000 for textile
CVAR(Float, col_cie_k1, 0.045, CVAR_SAVE)			// K1: 0.045 for graphics, 0.048 for textile
CVAR(Float, col_cie_k2, 0.015, CVAR_SAVE)			// K2: 0.015 for graphics, 0.014 for textile
CVAR(Float, col_cie_kc, 1.000, CVAR_SAVE)			// KC: didn't find a standard value anywhere
CVAR(Float, col_cie_kh, 1.000, CVAR_SAVE)			// KH: didn't find a standard value anywhere
CVAR(Float, col_cie_tristim_x,  95.02, CVAR_SAVE)	// These default tristimulus values correspond
CVAR(Float, col_cie_tristim_z, 108.82, CVAR_SAVE)	// to illuminant D65 and 2° observer.

/*******************************************************************
 * FUNCTIONS
 *******************************************************************/

/* CIE::CIE76
 * The oldest and simplest formula, merely the geometric distance
 * between two points in the colorspace.
 *******************************************************************/
double CIE::CIE76(lab_t& col1, lab_t& col2)
{
	double dl = col1.l - col2.l;
	double da = col1.a - col2.a;
	double db = col1.b - col2.b;

	return dl*dl + da*da + db*db;
}

/* CIE::CIE94
 * This one starts to become complicated as it transforms the Lab
 * colorspace into an LCh colorspace to try to be more accurate.
 *******************************************************************/
double CIE::CIE94(lab_t& col1, lab_t& col2)
{
	double dl = col1.l - col2.l;
	double da = col1.a - col2.a;
	double db = col1.b - col2.b;
	double c1 = sqrt(col1.a * col1.a + col1.b * col1.b);
	double c2 = sqrt(col2.a * col2.a + col2.b * col2.b);
	double dc = c1 - c2;
	double dh = sqrt((da * da) + (db * db) - (dc * dc));

	// Divide by the relevant factors
	dl /= col_cie_kl;
	dc /= (1 + (col_cie_k1 * c1));
	dh /= (1 + (col_cie_k2 * c1));

	return dl*dl + dc*dc + dh*dh;
}

/* CIE::CIEDE2000
 * And here is an unholy abomination of a code, whose sight makes
 * children cry. Adds hue rotation and multiple compensations. But
 * it really is a lot better than CIE94 for color matching.
 *******************************************************************/
double CIE::CIEDE2000(lab_t& col1, lab_t& col2)
{
	// Compute chroma values
	double c1 = sqrt(col1.a * col1.a + col1.b * col1.b);
	double c2 = sqrt(col2.a * col2.a + col2.b * col2.b);
	double cavg = (c1 + c2) / 2.0;

	// Compute G
	double c7 = pow(cavg, 7.0);
	double g = 0.5 * (1.0 - sqrt(c7 / (c7 + P257)));

	// Compute a'1 and a'2
	double ap1 = (1.0 + g) * col1.a;
	double ap2 = (1.0 + g) * col2.a;

	// Compute C'1 and C'2
	double cp1 = sqrt(ap1*ap1 + col1.b*col1.b);
	double cp2 = sqrt(ap2*ap2 + col2.b*col2.b);

	// Compute h'1 and h'2
	double hp1 = 0; if (col1.b != ap1 || ap1 != 0) { hp1 = atan2(col1.b, ap1); if (col1.b <= 0) hp1 += 2.0 * PI; }
	double hp2 = 0; if (col2.b != ap2 || ap2 != 0) { hp2 = atan2(col2.b, ap2); if (col2.b <= 0) hp2 += 2.0 * PI; }

	// Compute Delta-L'
	double dlp = col2.l - col1.l;

	// Compute Delta-C'
	double dcp = cp2 - cp1;

	// Compute Delta-h'
	double dhmp = 0;
	if (cp1 * cp2 != 0.0)
	{
		dhmp = hp2 - hp1;
		if (dhmp > PI) dhmp -= 2.0 * PI;
		else if (dhmp < -PI) dhmp += 2.0 * PI;
	}

	// Compute Delta-H'
	double dhp = 2.0 * sqrt(cp1 * cp2) * sin(dhmp / 2.0);

	// Compute L' average and C' average
	double lpavg = (col1.l + col2.l) / 2.0;
	double cpavg = (cp1 + cp2) / 2.0;

	// Compute h' average
	double hpavg = hp1 + hp2;
	if (cp1 * cp2 != 0)
	{
		if (fabs(hp1 - hp2) > PI)
		{
			if (hp1 + hp2 < 2.0 * PI)	hpavg += 2.0 * PI;
			else						hpavg -= 2.0 * PI;
		}
		hpavg /= 2.0;
	}

	// Compute T
	double t = 1.0
	           - 0.17 * cos(hpavg - (PI/6.0))
	           + 0.24 * cos(hpavg * 2.0)
	           + 0.32 * cos(hpavg * 3.0 + (PI/30.0))
	           - 0.20 * cos(hpavg * 4.0 - (21.0 * PI / 60.0));

	// Compute Delta-Theta (we need to convert to degrees for proper exp())
	double dtdegree = (hpavg * 180.0 / PI) - 275;
	double dt = 30.0 * exp(-1.0 * dtdegree * dtdegree / 625.0);

	// Compute RC
	double rc = 2.0 * sqrt(pow(cpavg, 7.0)/(pow(cpavg, 7.0) + P257));

	// Compute SL
	double lpavg502 = (lpavg - 50) * (lpavg - 50);
	double sl = 1.0 + (0.015 * lpavg502) / sqrt(20 + lpavg502);

	// Compute SC
	double sc = 1 + 0.045 * cpavg;

	// Compute SH
	double sh = 1 + 0.015 * cpavg * t;

	// Compute RT (we need to get back into radians for proper sin())
	double dtradian = dt * PI / 180.0;
	double rt = 0.0 - sin(2.0 * dtradian) * rc;

	// And finally, finally, compute Delta-E (without sqrt)
	double d1 = dlp / (col_cie_kl * sl);
	double d2 = dcp / (col_cie_kc * sc);
	double d3 = dhp / (col_cie_kh * sh);
	return d1*d1 + d2*d2 + d3*d3 + rt*d2*d3;
}

#ifdef DEBUGCIEDE2000
// This can be moved to before the "return" line in CIE::CIEDE2000() and de-commented to investigate incorrect results.
/*	LOG_MESSAGE(	// Fun fact: this call hits the parameter limit for wx's log system. One more and it breaks.
		"Lab1 (%f; %f; %f), Lab2 (%f; %f; %f), c1 %f, c2 %f, average c %f, a'1 %f, a'2 %f, C'1 %f, C'2 %f, "
		"h'1 %f, h'2 %f, h' avg %f, G %f, T %f, DL' %f, DC' %f, DH' %f, dTheta %f, RT %f, RC %f, SL %f, SC %f, SH %f, "
		"d1 %f, d2 %f, d3 %f\n",
		col1.l, col1.a, col1.b, col2.l, col2.a, col2.b, c1, c2, cavg, ap1, ap2, cp1, cp2,
		hp1, hp2, hpavg, g, t, dlp, dcp, dhp, dt, rt, rc, sl, sc, sh, d1, d2, d3);
*/

/* This function verifies the validity of the algorithm for the "official" test data.
 * The provided results have a precision of four decimal points, so it's the margin of
 * precision that'll be used. The results are only valid if KL, KC and KH are set to 1.
 */
#include "General/Console/Console.h"
CONSOLE_COMMAND(testciede, 0)
{
	lab_t labs[34][2] =
	{
		{ lab_t(50.0000,   2.6772, -79.7751), lab_t(50.0000,   0.0000, -82.7485) },	//   1
		{ lab_t(50.0000,   3.1571, -77.2803), lab_t(50.0000,   0.0000, -82.7485) },	//   2
		{ lab_t(50.0000,   2.8361, -74.0200), lab_t(50.0000,   0.0000, -82.7485) },	//   3
		{ lab_t(50.0000,  -1.3802, -84.2814), lab_t(50.0000,   0.0000, -82.7485) },	//   4
		{ lab_t(50.0000,  -1.1848, -84.8006), lab_t(50.0000,   0.0000, -82.7485) },	//   5
		{ lab_t(50.0000,  -0.9009, -85.5211), lab_t(50.0000,   0.0000, -82.7485) },	//   6
		{ lab_t(50.0000,   0.0000,   0.0000), lab_t(50.0000,  -1.0000,   2.0000) },	//   7
		{ lab_t(50.0000,  -1.0000,   2.0000), lab_t(50.0000,   0.0000,   0.0000) },	//   8
		{ lab_t(50.0000,   2.4900,  -0.0010), lab_t(50.0000,  -2.4900,   0.0009) },	//   9
		{ lab_t(50.0000,   2.4900,  -0.0010), lab_t(50.0000,  -2.4900,   0.0010) },	//  10
		{ lab_t(50.0000,   2.4900,  -0.0010), lab_t(50.0000,  -2.4900,   0.0011) },	//  11
		{ lab_t(50.0000,   2.4900,  -0.0010), lab_t(50.0000,  -2.4900,   0.0012) },	//  12
		{ lab_t(50.0000,  -0.0010,   2.4900), lab_t(50.0000,   0.0009,  -2.4900) },	//  13
		{ lab_t(50.0000,  -0.0010,   2.4900), lab_t(50.0000,   0.0010,  -2.4900) },	//  14
		{ lab_t(50.0000,  -0.0010,   2.4900), lab_t(50.0000,   0.0011,  -2.4900) },	//  15
		{ lab_t(50.0000,   2.5000,   0.0000), lab_t(50.0000,   0.0000,  -2.5000) },	//  16
		{ lab_t(50.0000,   2.5000,   0.0000), lab_t(73.0000,  25.0000, -18.0000) },	//  17
		{ lab_t(50.0000,   2.5000,   0.0000), lab_t(61.0000,  -5.0000,  29.0000) },	//  18
		{ lab_t(50.0000,   2.5000,   0.0000), lab_t(56.0000, -27.0000,  -3.0000) },	//  19
		{ lab_t(50.0000,   2.5000,   0.0000), lab_t(58.0000,  24.0000,  15.0000) },	//  20
		{ lab_t(50.0000,   2.5000,   0.0000), lab_t(50.0000,   3.1736,   0.5854) },	//  21
		{ lab_t(50.0000,   2.5000,   0.0000), lab_t(50.0000,   3.2972,   0.0000) },	//  22
		{ lab_t(50.0000,   2.5000,   0.0000), lab_t(50.0000,   1.8634,   0.5757) },	//  23
		{ lab_t(50.0000,   2.5000,   0.0000), lab_t(50.0000,   3.2592,   0.3350) },	//  24
		{ lab_t(60.2574, -34.0099,  36.2677), lab_t(60.4626, -34.1751,  39.4387) },	//  25
		{ lab_t(63.0109, -31.0961,  -5.8663), lab_t(62.8187, -29.7946,  -4.0864) },	//  26
		{ lab_t(61.2901,   3.7196,  -5.3901), lab_t(61.4292,   2.2480,  -4.9620) },	//  27
		{ lab_t(35.0831, -44.1164,   3.7933), lab_t(35.0232, -40.0716,   1.5901) },	//  28
		{ lab_t(22.7233,  20.0904, -46.6940), lab_t(23.0331,  14.9730, -42.5619) },	//  29
		{ lab_t(36.4612,  47.8580,  18.3852), lab_t(36.2715,  50.5065,  21.2231) },	//  30
		{ lab_t(90.8027,  -2.0831,   1.4410), lab_t(91.1528,  -1.6435,   0.0447) },	//  31
		{ lab_t(90.9257,  -0.5406,  -0.9208), lab_t(88.6381,  -0.8985,  -0.7239) },	//  32
		{ lab_t( 6.7747,  -0.2908,  -2.4247), lab_t( 5.8714,  -0.0985,  -2.2286) },	//  33
		{ lab_t( 2.0776,   0.0795,  -1.1350), lab_t( 0.9033,  -0.0636,  -0.5514) },	//  34
	};
	double results[34] =
	{
		2.0425, 2.8615, 3.4412, 1.0000, 1.0000, 1.0000, 2.3669, 2.3669,
		7.1792, 7.1792, 7.2195, 7.2195, 4.8045, 4.8045, 4.7461, 4.3065,
		27.1492, 22.8977, 31.9030, 19.4535,
		1.0000, 1.0000, 1.0000, 1.0000, 1.2644, 1.2630, 1.8731, 1.8645,
		2.0373, 1.4146, 1.4441, 1.5381, 0.6377, 0.9082
	};

	string report = "Testing CIEDE 2000 return values...";
	int errors = 0;
	for (size_t i = 0; i < 34; ++i)
	{
		double delta = sqrt(CIE::CIEDE2000(labs[i][0], labs[i][1]));
		bool margin = abs(delta - results[i]) < 0.0001;
		if (!margin) ++errors;
		report += S_FMT("\n%02d: [% #08.4f, % #08.4f, % #08.4f]:[% #08.4f, % #08.4f, % #08.4f] = % #08.4f (%s)",
		                i+1,
		                labs[i][0].l, labs[i][0].a, labs[i][0].b,
		                labs[i][1].l, labs[i][1].a, labs[i][1].b,
		                delta, margin?"Correct":"Erroneous"
		               );
	}
	if (errors)	report += S_FMT("\nThere were %d error%s in the results.", errors, (errors>1?"s":""));
	else		report += "\nAll results are accurate enough.";
	LOG_MESSAGE(report);
}
#endif // DEBUGCIEDE2000
