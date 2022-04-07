<subhead>Namespace</subhead>
<header>UI</header>

The `UI` scripting namespace contains functions for interacting with and displaying anything UI related.

## Constants

| Name | Value |
|:-----|:-----:|
`MB_ICON_INFO` | 0
`MB_ICON_QUESTION` | 1
`MB_ICON_WARNING` | 2
`MB_ICON_ERROR` | 3

## Functions

### Overview

#### Message Boxes

<fdef>[MessageBox](#messagebox)</func>(<arg>title</arg>, <arg>message</arg>, <arg>[icon]</arg>)</fdef>
<fdef>[MessageBoxExt](#messageboxext)</func>(<arg>title</arg>, <arg>message</arg>, <arg>detail</arg>)</fdef>
<fdef>[PromptString](#promptstring)</func>(<arg>title</arg>, <arg>message</arg>, <arg>defaultValue</arg>) -> <type>string</type></fdef>
<fdef>[PromptNumber](#promptnumber)</func>(<arg>title</arg>, <arg>message</arg>, <arg>defaultValue</arg>, <arg>min</arg>, <arg>max</arg>) -> <type>integer</type></fdef>
<fdef>[PromptYesNo](#promptyesno)</func>(<arg>title</arg>, <arg>message</arg>) -> <type>boolean</type></fdef>

#### File Dialogs

<fdef>[PromptOpenFile](#promptopenfile)</func>(<arg>title</arg>, <arg>extensions</arg>, <arg>filename</arg>) -> <type>string</type></fdef>
<fdef>[PromptOpenFiles](#promptopenfiles)</func>(<arg>title</arg>, <arg>extensions</arg>) -> <type>string[]</type></fdef>
<fdef>[PromptSaveFile](#promptsavefile)</func>(<arg>title</arg>, <arg>extensions</arg>, <arg>[defaultFilename]</arg>) -> <type>string</type></fdef>
<fdef>[PromptSaveFiles](#promptsavefiles)</func>(<arg>title</arg>, <arg>extensions</arg>) -> <type>string</type>, <type>string</type></fdef>

#### Splash Window

<fdef>[ShowSplash](#showsplash)</func>(<arg>message</arg>, <arg>[showProgress]</arg>)</fdef>
<fdef>[HideSplash](#hidesplash)</func>()</fdef>
<fdef>[UpdateSplash](#updatesplash)</func>()</fdef>
<fdef>[SplashProgress](#splashprogress)</func>() -> <type>float</type></fdef>
<fdef>[SetSplashMessage](#setsplashmessage)</func>(<arg>message</arg>)</fdef>
<fdef>[SetSplashProgressMessage](#setsplashprogressmessage)</func>(<arg>message</arg>)</fdef>
<fdef>[SetSplashProgress](#setsplashprogress)</func>(<arg>progress</arg>)</fdef>

---
### MessageBox

Shows a simple message dialog.

#### Parameters

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display
  * <arg>[icon]</arg> (<type>integer</type>): The icon to display on the message box, see the `MB_ICON_` constants above. Default is `MB_ICON_INFO`

---
### MessageBoxExt

Shows an extended message box with an extra scrollable text box displaying <arg>detail</arg>.

#### Parameters

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display
  * <arg>detail</arg> (<type>string</type>): The detailed message to display

---
### PromptString

Shows a dialog prompt for the user to enter a string value.

#### Parameters

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display
  * <arg>defaultValue</arg> (<type>string</type>): The initial default value

#### Returns

* <type>string</type>: The text entered by the user

---
### PromptNumber

Shows a dialog prompt for the user to enter a numeric value.

#### Parameters

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display
  * <arg>defaultValue</arg> (<type>integer</type>): The initial default value
  * <arg>min</arg> (<type>integer</type>): The minimum value allowed
  * <arg>max</arg> (<type>integer</type>): The maximum value allowed

#### Returns

* <type>integer</type>: The number entered by the user

---
### PromptYesNo

Shows a dialog prompt with 'Yes' and 'No' buttons.

#### Parameters

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display

#### Returns

* <type>boolean</type>: `true` if the user clicked 'Yes'

---
### PromptOpenFile

Shows a file browser dialog allowing the user to select a file to open.

#### Parameters

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>extensions</arg> (<type>string</type>): A formatted list of selectable file extensions (see description for format)
  * <arg>filename</arg> (<type>string</type>): The name of the file to browse for

#### Returns

* <type>string</type>: The full path to the selected file, or an empty string if none selected

#### Notes

The <arg>extensions</arg> parameter must be in the following format:

> \[Type Name 1\]|\[Extension 1\]|\[Type Name 2\]|\[Extension 2\]|...

Where `Type Name X` is the name to display in the type selection dropdown, and `Extension` is the wildcard file extension for that type. For an example see below.

#### Example

```lua
local path = UI.PromptOpenFile('Select a File', 'Wad Files (*.wad)|*.wad|All Files|*.*', '')
App.LogMessage('Selected file ' .. path)
```

---
### PromptOpenFiles

Shows a file browser dialog allowing the user to select multiple files to open.

#### Parameters

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>extensions</arg> (<type>string</type>): A formatted list of selectable file extensions

#### Returns

* <type>string[]</type>: An array of full paths to the selected files

#### Notes

See <code>[PromptOpenFile](#promptopenfile)</code> above for the formatting of the <arg>extensions</arg> parameter.

---
### PromptSaveFile

Shows a file browser dialog allowing the user to select a path+filename to save a single file to.

#### Parameters

* <arg>title</arg> (<type>string</type>): The dialog caption
* <arg>extensions</arg> (<type>string</type>): A formatted list of selectable file extensions
* <arg>[defaultFilename]</arg> (<type>string</type>): The default name to put in the browser dialog 'file name' text box. Default is `""`

#### Returns

* <type>string</type>: The full selected path, or an empty string if the dialog was cancelled

#### Notes

See <code>[PromptOpenFile](#promptopenfile)</code> above for the formatting of the <arg>extensions</arg> parameter.

---
### PromptSaveFiles

Shows a file browser dialog allowing the user to select a path to save multiple files to.

#### Parameters

* <arg>title</arg> (<type>string</type>): The dialog caption
* <arg>extensions</arg> (<type>string</type>): A formatted list of selectable file extensions

#### Returns

* <type>string</type>: The directory path to save the files to
* <type>string</type>: The file extension that was selected (not the full wildcard, just the extension - eg. `wad` or `*`)

#### Notes

See <code>[PromptOpenFile](#promptopenfile)</code> above for the formatting of the <arg>extensions</arg> parameter.

---
### ShowSplash

Shows the splash window with <arg>message</arg>. 

#### Parameters

* <arg>message</arg> (<type>string</type>): The message to show
* <arg>[showProgress]</arg> (<type>boolean</type>): If `true`, a progress bar will be shown on the splash window (see <code>[SplashProgress](#splashprogress)</code>, <code>[SetSplashProgressMessage](#setsplashprogressmessage)</code>, <code>[SetSplashProgress](#setsplashprogress)</code> below). Default is `false`

#### Example

```lua
-- Show with no progress bar
UI.ShowSplash('This is a splash window message')

-- Show with progress bar
UI.ShowSplash('Doing some things...', true)
```

---
### HideSplash

Hides the splash window if it is currently showing.

---
### UpdateSplash

Updates and redraws the splash window.

---
### SplashProgress

#### Returns

* <type>float</type>: The current progress bar progress. This is a floating point number between `0.0` (empty) and `1.0` (full)

---
### SetSplashMessage

Sets the splash window message.

#### Parameters

* <arg>message</arg> (<type>string</type>): The message to show

---
### SetSplashProgressMessage

Sets the small message within the progress bar.

#### Parameters

* <arg>message</arg> (<type>string</type>): The message to show in the progress bar

---
### SetSplashProgress

Sets the progress bar progress amount.

#### Parameters

* <arg>progress</arg> (<type>float</type>): The progress amount. This is a floating point number between `0.0` (empty) and `1.0` (full)
