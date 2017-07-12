The `slade` scripting namespace contains a set of functions for general interaction with the SLADE application.

## Functions

### logMessage

**Parameters**

  * `string` **message**: The message to print to the log

Writes a message to the SLADE log.

**Example**

```
slade.logMessage('This is a log message')
```

---
### globalError

**Returns** `string`

Returns the most recently generated error message

---
### messageBox

**Parameters**

  * `string` **title**: The dialog caption
  * `string` **message**: The message to display

Shows a simple message dialog

---
### promptString

**Parameters**

  * `string` **title**: The dialog caption
  * `string` **message**: The message to display
  * `string` **defaultValue**: The initial default value

**Returns** `string`

Shows a dialog prompt for the user to enter a string value

---
### promptNumber

**Parameters**

  * `string` **title**: The dialog caption
  * `string` **message**: The message to display
  * `number` **defaultValue**: The initial default value
  * `number` **min**: The minimum value allowed
  * `number` **max**: The maximum value allowed

**Returns** `number`

Shows a dialog prompt for the user to enter a numeric value

---
### promptYesNo

**Parameters**

  * `string` **title**: The dialog caption
  * `string` **message**: The message to display

**Returns** `boolean`

Shows a dialog prompt with 'Yes' and 'No' buttons, returning `true` for yes or `false` for no

---
### browseFile

**Parameters**

  * `string` **title**: The dialog caption
  * `string` **extensions**: A formatted list of selectable file extensions (see description for format)
  * `string` **filename**: The name of the file to browse for

**Returns** `string`

Shows a file browser dialog allowing the user to select a file and returns the full path to the selected file. If no file was selected it will return an empty `string`.

The extensions parameter must be in the following format:

> `Type Name 1`|`Extension 1`|`Type Name 2`|`Extension 2`|...

Where `Type Name X` is the name to display in the type selection dropdown, and `Extension` is the wildcard file extension for that type. For an example see below

**Example**

    local path = slade.browseFile('Select a File', 'Wad Files (*.wad)|*.wad|All Files|*.*', '')
    slade.logMessage('Selected file ' .. path)

---
### browseFiles

**Parameters**

  * `string` **title**: The dialog caption
  * `string` **extensions**: A formatted list of selectable file extensions

**Returns** `string` array

Shows a file browser dialog allowing the user to select multiple files and returns an array of full paths to the selected files. If no file was selected it will return an empty array. See **browseFile** above for the formatting of the `extensions` parameter.

---
### currentArchive

**Returns** <code>[Archive](../Types/Archive.md)</code>

Returns the archive for the currently open tab in the main SLADE window.

---
### currentEntry

**Returns** <code>[ArchiveEntry](../Types/ArchiveEntry.md)</code>

Returns the currently open entry in the main SLADE window.

---
### currentEntrySelection

**Returns** <code>[ArchiveEntry](../Types/ArchiveEntry.md)</code> array

Returns an array of the currently selected entries in the main SLADE window.

---
### showArchive

**Parameters**

  * <code>[Archive](../Types/Archive.md)</code> archive: The archive to show

Shows the tab for the given archive in the main SLADE window.

---
### showEntry

**Parameters**

  * <code>[ArchiveEntry](../Types/ArchiveEntry.md)</code> entry: The entry to show_

Shows the given entry in a tab in the main SLADE window.
