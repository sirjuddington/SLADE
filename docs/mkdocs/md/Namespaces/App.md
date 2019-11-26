<article-head>App</article-head>

The `App` scripting namespace contains a set of functions for general interaction with the SLADE application.

## Functions

### LogMessage

<fdef>function App.<func>LogMessage</func>(<arg>message</arg>)</fdef>

Writes a message to the SLADE log.

<listhead>Parameters</listhead>

* <arg>message</arg> (<type>string</type>): The message to print to the log

#### Example

```lua
App.LogMessage('This is a log message')
```

---
### LogWarning

<fdef>function App.<func>LogWarning</func>(<arg>message</arg>)</fdef>

Writes a warning message to the SLADE log. Warning messages are displayed in a yellow colour in the console log window.

<listhead>Parameters</listhead>

* <arg>message</arg> (<type>string</type>): The message to print to the log

---
### LogError

<fdef>function App.<func>LogError</func>(<arg>message</arg>)</fdef>

Writes an error message to the SLADE log. Error messages are displayed in a red colour in the console log window.

<listhead>Parameters</listhead>

* <arg>message</arg> (<type>string</type>): The message to print to the log

---
### CurrentArchive

<fdef>function App.<func>CurrentArchive</func>()</fdef>

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive/Archive.md)</type>: The archive for the currently open tab in the main SLADE window

---
### CurrentEntry

<fdef>function App.<func>CurrentEntry</func>()</fdef>

<listhead>Returns</listhead>

* <type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>: The currently open entry in the main SLADE window

---
### CurrentEntrySelection

<fdef>function App.<func>CurrentEntrySelection</func>()</fdef>

<listhead>Returns</listhead>

* <type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)\[\]</type>: An array of the currently selected entries in the main SLADE window

---
### CurrentPalette

<fdef>function App.<func>CurrentPalette</func>(<arg>entryFor</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>[entryFor]</arg> (<type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>, default `nil`): If given, the appropriate palette for this entry will be found (if 'Existing/Global' is selected)

<listhead>Returns</listhead>

* <type>[Palette](../Types/Graphics/Palette.md)</type>: The currently selected palette

#### Notes

If 'Existing/Global' is selected in the main window palette dropdown, this will return the palette from the currently selected base resource archive. Additionally, if <arg>entryFor</arg> was given, this will check for certain cases where an entry does not use `PLAYPAL` as its palette, and return the appropriate palette instead.

---
### ShowArchive

<fdef>function App.<func>ShowArchive</func>(<arg>archive</arg>)</fdef>

Shows the tab for the given <arg>archive</arg> in the main SLADE window.

<listhead>Parameters</listhead>

  * <arg>archive</arg> (<type>[Archive](../Types/Archive/Archive.md)</type>): The archive to show

---
### ShowEntry

<fdef>function App.<func>ShowEntry</func>(<arg>entry</arg>)</fdef>

Shows the given <arg>entry</arg> in a tab in the main SLADE window.

<listhead>Parameters</listhead>

  * <arg>entry</arg> (<type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>): The entry to show

---
### MapEditor

<fdef>function App.<func>MapEditor</func>()</fdef>

<listhead>Returns</listhead>

* <type>[MapEditor](../Types/Map/MapEditor.md)</type>: The currently open map editor
