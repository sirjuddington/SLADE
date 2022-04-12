<subhead>Namespace</subhead>
<header>App</header>

The `App` scripting namespace contains a set of functions for general interaction with the SLADE application.

## Functions

### Overview

#### Logging

<fdef>[LogMessage](#logmessage)(<arg>message</arg>)</fdef>
<fdef>[LogWarning](#logwarning)(<arg>message</arg>)</fdef>
<fdef>[LogError](#logerror)(<arg>message</arg>)</fdef>

#### Editor

<fdef>[CurrentArchive](#currentarchive)() -> <type>[Archive](../Types/Archive/Archive.md)</type></fdef>
<fdef>[CurrentEntry](#currententry)() -> <type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type></fdef>
<fdef>[CurrentEntrySelection](#currententryselection)() -> <type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)\[\]</type></fdef>
<fdef>[CurrentPalette](#currentpalette)(<arg>[entryFor]</arg>) -> <type>[Palette](../Types/Graphics/Palette.md)</type></fdef>
<fdef>[ShowArchive](#showarchive)(<arg>archive</arg>)</fdef>
<fdef>[ShowEntry](#showentry)(<arg>entry</arg>)</fdef>
<fdef>[MapEditor](#mapeditor)() -> <type>[MapEditor](../Types/Map/MapEditor.md)</type></fdef>

---
### LogMessage

Writes a message to the SLADE log.

#### Parameters

* <arg>message</arg> (<type>string</type>): The message to print to the log

#### Example

```lua
App.LogMessage('This is a log message')
```

---
### LogWarning

Writes a warning message to the SLADE log. Warning messages are displayed in a yellow colour in the console log window.

#### Parameters

* <arg>message</arg> (<type>string</type>): The message to print to the log

---
### LogError

Writes an error message to the SLADE log. Error messages are displayed in a red colour in the console log window.

#### Parameters

* <arg>message</arg> (<type>string</type>): The message to print to the log

---
### CurrentArchive

Gets the archive for the currently open tab in the main SLADE window.

#### Returns

* <type>[Archive](../Types/Archive/Archive.md)</type>: The archive for the currently open tab in the main SLADE window

---
### CurrentEntry

Gets the currently open entry in the main SLADE window.

#### Returns

* <type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>: The currently open entry in the main SLADE window

---
### CurrentEntrySelection

Gets the currently selected entries in the main SLADE window.

#### Returns

* <type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)\[\]</type>: An array of the currently selected entries in the main SLADE window

---
### CurrentPalette

Gets the current palette.

#### Parameters

* <arg>[entryFor]</arg> (<type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>): If given, the appropriate palette for this entry will be found (if 'Existing/Global' is selected). Default is `nil`

#### Returns

* <type>[Palette](../Types/Graphics/Palette.md)</type>: The currently selected palette

#### Notes

If 'Existing/Global' is selected in the main window palette dropdown, this will return the palette from the currently selected base resource archive. Additionally, if <arg>entryFor</arg> was given, this will check for certain cases where an entry does not use `PLAYPAL` as its palette, and return the appropriate palette instead.

---
### ShowArchive

Shows the tab for the given <arg>archive</arg> in the main SLADE window.

#### Parameters

* <arg>archive</arg> (<type>[Archive](../Types/Archive/Archive.md)</type>): The archive to show

---
### ShowEntry

Shows the given <arg>entry</arg> in a tab in the main SLADE window.

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>): The entry to show

---
### MapEditor

Gets the currently open map editor (if any).

#### Returns

* <type>[MapEditor](../Types/Map/MapEditor.md)</type>: The currently open map editor
