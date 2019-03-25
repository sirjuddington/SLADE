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

```lua
function UI.MessageBox(title, message, icon)
```

Shows a simple message dialog.

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>message</arg>: The message to display
  * `[`<type>number</type> <arg>icon</arg>`]`: The icon to display on the message box, see the `MB_ICON_` constants above. Defaults to `MB_ICON_INFO` if not given

---
### MessageBoxExt

```lua
function UI.MessageBoxExt(title, message, detail)
```

Shows an extended message box with an extra scrollable text box displaying <arg>detail</arg>.

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>message</arg>: The message to display
  * <type>string</type> <arg>detail</arg>: The detailed message to display

---
### PromptString

```lua
function UI.PromptString(title, message, defaultValue)
```

Shows a dialog prompt for the user to enter a string value.

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>message</arg>: The message to display
  * <type>string</type> <arg>defaultValue</arg>: The initial default value

<listhead>Returns</listhead>

* <type>string</type>: The text entered by the user

---
### PromptNumber

```lua
function UI.PromptNumber(title, message, defaultValue, min, max)
```

Shows a dialog prompt for the user to enter a numeric value.

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>message</arg>: The message to display
  * <type>number</type> <arg>defaultValue</arg>: The initial default value
  * <type>number</type> <arg>min</arg>: The minimum value allowed
  * <type>number</type> <arg>max</arg>: The maximum value allowed

<listhead>Returns</listhead>

* <type>number</type>: The number entered by the user

---
### PromptYesNo

```lua
function UI.PromptYesNo(title, message)
```

Shows a dialog prompt with 'Yes' and 'No' buttons.

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>message</arg>: The message to display

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the user clicked 'Yes'

## Functions - File Browsing

### BrowseFile

```lua
function UI.BrowseFile(title, extensions, filename)
```

Shows a file browser dialog allowing the user to select a file.

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>extensions</arg>: A formatted list of selectable file extensions (see description for format)
  * <type>string</type> <arg>filename</arg>: The name of the file to browse for

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

```lua
function UI.BrowseFiles(title, extensions)
```

Shows a file browser dialog allowing the user to select multiple files.

<listhead>Parameters</listhead>

  * <type>string</type> <arg>title</arg>: The dialog caption
  * <type>string</type> <arg>extensions</arg>: A formatted list of selectable file extensions

<listhead>Returns</listhead>

* <type>string[]</type>: An array of full paths to the selected files

**Notes**

See [BrowseFile](#browsefile) above for the formatting of the <arg>extensions</arg> parameter.

## Functions - Splash Window

### ShowSplash

```lua
function UI.ShowSplash(message, showProgress)
```

Shows the splash window with <arg>message</arg>. 

<listhead>Parameters</listhead>

* <type>string</type> <arg>message</arg>: The message to show
* `[`<type>boolean</type> <arg>showProgress</arg>`]`: If `true`, a progress bar will be shown on the splash window (see <code>[SplashProgress](#splashprogress)</code>, <code>[SetSplashProgressMessage](#setsplashprogressmessage)</code>, <code>[SetSplashProgress](#setsplashprogress)</code> below). Defaults to `false` if not given

**Example**

```lua
-- Show with no progress bar
UI.ShowSplash('This is a splash window message')

-- Show with progress bar
UI.ShowSplash('Doing some things...', true)
```

---
### HideSplash

```lua
function UI.HideSplash()
```

Hides the splash window if it is currently showing.

---
### UpdateSplash

```lua
function UI.UpdateSplash()
```

Updates and redraws the splash window.

---
### SplashProgress

```lua
function UI.SplashProgress()
```

<listhead>Returns</listhead>

* <type>number</type>: The current progress bar progress. This is a floating point number between `0.0` (empty) and `1.0` (full)

---
### SetSplashMessage

```lua
function UI.SetSplashMessage(message)
```

Sets the splash window message.

<listhead>Parameters</listhead>

* <type>string</type> <arg>message</arg>: The message to show

---
### SetSplashProgressMessage

```lua
function UI.SetSplashProgressMessage(message)
```

Sets the small message within the progress bar.

<listhead>Parameters</listhead>

* <type>string</type> <arg>message</arg>: The message to show in the progress bar

---
### SetSplashProgress

```lua
function UI.SetSplashProgress(progress)
```

Sets the progress bar progress amount.

<listhead>Parameters</listhead>

* <type>number</type> <arg>progress</arg>: The progress amount. This is a floating point number between `0.0` (empty) and `1.0` (full)
