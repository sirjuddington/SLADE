// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2026 Simon Judd
//
// Filename:    Globals.cpp
// Description: Definitions for application-wide global state
// -----------------------------------------------------------------------------

#include "Main.h"

namespace slade::global
{
string error;

#ifdef GIT_DESCRIPTION
string sc_rev = GIT_DESCRIPTION;
#else
string sc_rev;
#endif

#ifdef DEBUG
bool debug = true;
#else
bool debug = false;
#endif

int win_version_major = 0;
int win_version_minor = 0;
} // namespace slade::global
