<article-head>UI</article-head>

The `UI` scripting namespace contains functions for interacting with and displaying anything UI related.

## Constants

| Name | Value |
|:-----|:-----:|
`MB_ICON_INFO` | 0
`MB_ICON_QUESTION` | 1
`MB_ICON_WARNING` | 2
`MB_ICON_ERROR` | 3

## Functions - Message Boxes

### MessageBox

<fdef>function UI.<func>MessageBox</func>(<arg>title</arg>, <arg>message</arg>, <arg>icon</arg>)</fdef>

Shows a simple message dialog.

<listhead>Parameters</listhead>

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display
  * <arg>[icon]</arg> (<type>number</type>, default `MB_ICON_INFO`): The icon to display on the message box, see the `MB_ICON_` constants above

---
### MessageBoxExt

<fdef>function UI.<func>MessageBoxExt</func>(<arg>title</arg>, <arg>message</arg>, <arg>detail</arg>)</fdef>

Shows an extended message box with an extra scrollable text box displaying <arg>detail</arg>.

<listhead>Parameters</listhead>

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display
  * <arg>detail</arg> (<type>string</type>): The detailed message to display

---
### PromptString

<fdef>function UI.<func>PromptString</func>(<arg>title</arg>, <arg>message</arg>, <arg>defaultValue</arg>)</fdef>

Shows a dialog prompt for the user to enter a string value.

<listhead>Parameters</listhead>

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display
  * <arg>defaultValue</arg> (<type>string</type>): The initial default value

<listhead>Returns</listhead>

* <type>string</type>: The text entered by the user

---
### PromptNumber

<fdef>function UI.<func>PromptNumber</func>(<arg>title</arg>, <arg>message</arg>, <arg>defaultValue</arg>, <arg>min</arg>, <arg>max</arg>)</fdef>

Shows a dialog prompt for the user to enter a numeric value.

<listhead>Parameters</listhead>

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display
  * <arg>defaultValue</arg> (<type>number</type>): The initial default value
  * <arg>min</arg> (<type>number</type>): The minimum value allowed
  * <arg>max</arg> (<type>number</type>): The maximum value allowed

<listhead>Returns</listhead>

* <type>number</type>: The number entered by the user

---
### PromptYesNo

<fdef>function UI.<func>PromptYesNo</func>(<arg>title</arg>, <arg>message</arg>)</fdef>

Shows a dialog prompt with 'Yes' and 'No' buttons.

<listhead>Parameters</listhead>

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>message</arg> (<type>string</type>): The message to display

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the user clicked 'Yes'

## Functions - File Browsing

### BrowseFile

<fdef>function UI.<func>BrowseFile</func>(<arg>title</arg>, <arg>extensions</arg>, <arg>filename</arg>)</fdef>

Shows a file browser dialog allowing the user to select a file.

<listhead>Parameters</listhead>

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>extensions</arg> (<type>string</type>): A formatted list of selectable file extensions (see description for format)
  * <arg>filename</arg> (<type>string</type>): The name of the file to browse for

<listhead>Returns</listhead>

* <type>string</type>: The full path to the selected file, or an empty string if none selected

**Notes**

The <arg>extensions</arg> parameter must be in the following format:

> \[Type Name 1\]|\[Extension 1\]|\[Type Name 2\]|\[Extension 2\]|...

Where `Type Name X` is the name to display in the type selection dropdown, and `Extension` is the wildcard file extension for that type. For an example see below.

**Example**

```lua
local path = UI.BrowseFile('Select a File', 'Wad Files (*.wad)|*.wad|All Files|*.*', '')
App.LogMessage('Selected file ' .. path)
```

---
### BrowseFiles

<fdef>function UI.<func>BrowseFiles</func>(<arg>title</arg>, <arg>extensions</arg>)</fdef>

Shows a file browser dialog allowing the user to select multiple files.

<listhead>Parameters</listhead>

  * <arg>title</arg> (<type>string</type>): The dialog caption
  * <arg>extensions</arg> (<type>string</type>): A formatted list of selectable file extensions

<listhead>Returns</listhead>

* <type>string[]</type>: An array of full paths to the selected files

**Notes**

See <code>[BrowseFile](#browsefile)</code> above for the formatting of the <arg>extensions</arg> parameter.

## Functions - Splash Window

### ShowSplash

<fdef>function UI.<func>ShowSplash</func>(<arg>message</arg>, <arg>showProgress</arg>)</fdef>

Shows the splash window with <arg>message</arg>. 

<listhead>Parameters</listhead>

* <arg>message</arg> (<type>string</type>): The message to show
* <arg>[showProgress]</arg> (<type>boolean</type>, default `false`): If `true`, a progress bar will be shown on the splash window (see <code>[SplashProgress](#splashprogress)</code>, <code>[SetSplashProgressMessage](#setsplashprogressmessage)</code>, <code>[SetSplashProgress](#setsplashprogress)</code> below)

**Example**

```lua
-- Show with no progress bar
UI.ShowSplash('This is a splash window message')

-- Show with progress bar
UI.ShowSplash('Doing some things...', true)
```

---
### HideSplash

<fdef>function UI.<func>HideSplash</func>()</fdef>

Hides the splash window if it is currently showing.

---
### UpdateSplash

<fdef>function UI.<func>UpdateSplash</func>()</fdef>

Updates and redraws the splash window.

---
### SplashProgress

<fdef>function UI.<func>SplashProgress</func>()</fdef>

<listhead>Returns</listhead>

* <type>number</type>: The current progress bar progress. This is a floating point number between `0.0` (empty) and `1.0` (full)

---
### SetSplashMessage

<fdef>function UI.<func>SetSplashMessage</func>(<arg>message</arg>)</fdef>

Sets the splash window message.

<listhead>Parameters</listhead>

* <arg>message</arg> (<type>string</type>): The message to show

---
### SetSplashProgressMessage

<fdef>function UI.<func>SetSplashProgressMessage</func>(<arg>message</arg>)</fdef>

Sets the small message within the progress bar.

<listhead>Parameters</listhead>

* <arg>message</arg> (<type>string</type>): The message to show in the progress bar

---
### SetSplashProgress

<fdef>function UI.<func>SetSplashProgress</func>(<arg>progress</arg>)</fdef>

Sets the progress bar progress amount.

<listhead>Parameters</listhead>

* <arg>progress</arg> (<type>number</type>): The progress amount. This is a floating point number between `0.0` (empty) and `1.0` (full)
