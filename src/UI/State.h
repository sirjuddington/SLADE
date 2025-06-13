#pragma once

namespace slade::ui
{
void initStateProps();
bool hasSavedState(const char* name);

bool   getStateBool(string_view name);
int    getStateInt(string_view name);
double getStateFloat(string_view name);
string getStateString(string_view name);

void saveStateBool(string_view name, bool value);
void saveStateInt(string_view name, int value);
void saveStateFloat(string_view name, double value);
void saveStateString(string_view name, string_view value);
void toggleStateBool(string_view name);
} // namespace slade::ui
