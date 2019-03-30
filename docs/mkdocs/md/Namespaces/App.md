<article-head>App</article-head>

The `App` scripting namespace contains a set of functions for general interaction with the SLADE application.

## Functions

### LogMessage

<fdef>function App.<func>LogMessage</func>(<arg>message</arg>)</fdef>

Writes a message to the SLADE log.

<listhead>Parameters</listhead>

* <arg>message</arg> (<type>string</type>): The message to print to the log

**Example**

```lua
App.LogMessage('This is a log message')
```

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
