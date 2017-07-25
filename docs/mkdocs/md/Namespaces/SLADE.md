The `slade` scripting namespace contains a set of functions for general interaction with the SLADE application.

## Functions

### `logMessage`

<listhead>Parameters</listhead>

* <type>string</type> <arg>message</arg>: The message to print to the log

Writes a message to the SLADE log.

**Example**

```lua
slade.logMessage('This is a log message')
```

---
### `globalError`

**Returns** <type>string</type>

Returns the most recently generated error message

---
### `messageBox`

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>message</arg>: The message to display

Shows a simple message dialog

---
### `promptString`

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>message</arg>: The message to display
  * <type>string</type> <arg>defaultValue</arg>: The initial default value

**Returns** <type>string</type>

Shows a dialog prompt for the user to enter a string value

---
### `promptNumber`

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>message</arg>: The message to display
  * <type>number</type> <arg>defaultValue</arg>: The initial default value
  * <type>number</type> <arg>min</arg>: The minimum value allowed
  * <type>number</type> <arg>max</arg>: The maximum value allowed

**Returns** <type>number</type>

Shows a dialog prompt for the user to enter a numeric value

---
### `promptYesNo`

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>message</arg>: The message to display

**Returns** <type>boolean</type>

Shows a dialog prompt with 'Yes' and 'No' buttons, returning `true` for yes or `false` for no

---
### `browseFile`

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>extensions</arg>: A formatted list of selectable file extensions (see description for format)
  * <type>string</type> <arg>filename</arg>: The name of the file to browse for

**Returns** <type>string</type>

Shows a file browser dialog allowing the user to select a file and returns the full path to the selected file. If no file was selected it will return an empty <type>string</type>.

The extensions parameter must be in the following format:

> \[Type Name 1\]|\[Extension 1\]|\[Type Name 2\]|\[Extension 2\]|...

Where `Type Name X` is the name to display in the type selection dropdown, and `Extension` is the wildcard file extension for that type. For an example see below

**Example**

```lua
local path = slade.browseFile('Select a File', 'Wad Files (*.wad)|*.wad|All Files|*.*', '')
slade.logMessage('Selected file ' .. path)
```

---
### `browseFiles`

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>extensions</arg>: A formatted list of selectable file extensions

**Returns** <type>string[]</type>

Shows a file browser dialog allowing the user to select multiple files and returns an array of full paths to the selected files. If no file was selected it will return an empty array. See **browseFile** above for the formatting of the <arg>extensions</arg> parameter.

---
### `currentArchive`

**Returns** <type>[Archive](../Types/Archive.md)</type>

Returns the archive for the currently open tab in the main SLADE window.

---
### `currentEntry`

**Returns** <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type>

Returns the currently open entry in the main SLADE window.

---
### `currentEntrySelection`

**Returns** <type>[ArchiveEntry](../Types/ArchiveEntry.md)\[\]</type>

Returns an array of the currently selected entries in the main SLADE window.

---
### `showArchive`

<listhead>Parameters</listhead>

  * <type>[Archive](../Types/Archive.md)</type> <arg>archive</arg>: The archive to show

Shows the tab for the given archive in the main SLADE window.

---
### `showEntry`

<listhead>Parameters</listhead>

  * <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type> <arg>entry</arg>: The entry to show_

Shows the given entry in a tab in the main SLADE window.

---
### `mapEditor`

**Returns** <type>[MapEditor](../Types/MapEditor.md)</type>

Returns the currently open map editor
