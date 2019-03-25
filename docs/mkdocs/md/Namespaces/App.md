<article-head>App</article-head>

The `App` scripting namespace contains a set of functions for general interaction with the SLADE application.

## Functions

### LogMessage

```lua
function App.LogMessage(message)
```

Writes a message to the SLADE log.

<listhead>Parameters</listhead>

* <type>string</type> <arg>message</arg>: The message to print to the log

**Example**

```lua
App.LogMessage('This is a log message')
```

---
### CurrentArchive

```lua
function App.CurrentArchive()
```

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive.md)</type>: The archive for the currently open tab in the main SLADE window

---
### CurrentEntry

```lua
function App.CurrentEntry()
```

<listhead>Returns</listhead>

* <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type>: The currently open entry in the main SLADE window

---
### CurrentEntrySelection

```lua
function App.CurrentEntrySelection()
```

<listhead>Returns</listhead>

* <type>[ArchiveEntry](../Types/ArchiveEntry.md)\[\]</type>: An array of the currently selected entries in the main SLADE window

---
### ShowArchive

```lua
function App.ShowArchive(archive)
```

Shows the tab for the given <arg>archive</arg> in the main SLADE window.

<listhead>Parameters</listhead>

  * <type>[Archive](../Types/Archive.md)</type> <arg>archive</arg>: The archive to show

---
### ShowEntry

```lua
function App.ShowEntry(entry)
```

Shows the given <arg>entry</arg> in a tab in the main SLADE window.

<listhead>Parameters</listhead>

  * <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type> <arg>entry</arg>: The entry to show

---
### MapEditor

```lua
function App.MapEditor()
```

<listhead>Returns</listhead>

* <type>[MapEditor](../Types/MapEditor.md)</type>: The currently open map editor
