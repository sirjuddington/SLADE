The `Archives` scripting namespace contains functions for managing archives.

## Functions

### `All`

**Returns** <type>[Archive](../Types/Archive.md)\[\]</type>

Returns an array of all currently open archives.

---
### `Create`

<listhead>Parameters</listhead>

* <type>string</type> <arg>formatId</arg>: The <type>[ArchiveFormat](../Types/ArchiveFormat.md)</type> id to create

**Returns** <type>[Archive](../Types/Archive.md)</type>, <type>string</type>

Creates and returns a new archive of the format specified in <arg>formatId</arg>. If the archive could not be created (generally if the <arg>formatId</arg> is invalid), returns `nil` and an error message.

!!! Note
    Currently only `wad` and `zip` formats are supported for creation

---
### `OpenFile`

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The full path (on disk) of the archive file to open

**Returns** <type>[Archive](../Types/Archive.md)</type>, <type>string</type>

Attempts to open the file at <arg>path</arg> on disk. Returns the archive if it opened successfully, or `nil` and an error message if it could not be opened.

---
### `Close` <sup>(1)</sup>

<listhead>Parameters</listhead>

* <type>[Archive](../Types/Archive.md)</type> <arg>archive</arg>: The archive to close

**Returns** <type>boolean</type>

Closes the given <arg>archive</arg>. Returns `false` if <arg>archive</arg> is invalid or not currently open.

---
### `Close` <sup>(2)</sup>

<listhead>Parameters</listhead>

* <type>number</type> <arg>index</arg>: The index of the archive to close

**Returns** <type>boolean</type>

Closes the archive at <arg>index</arg> in the list of currently open archives (see <code>[All](#All)</code>). Returns `false` if the given <arg>index</arg> is invalid.

---
### `CloseAll`

Closes all currently open archives.

---
### `FileExtensionsString`

**Returns** <type>string</type>

Returns a string with the extension filter for all supported archive file types.

See <code>[App.BrowseFile](App.md#browsefile)</code> and the [Open Archive](../Examples/OpenArchive.md) example for more information.

---
### `BaseResource`

**Returns** <type>[Archive](../Types/Archive.md)</type>

Returns the currently loaded base resource archive.

---
### `BaseResourcePaths`

**Returns** <type>string[]</type>

Returns an array of configured base resource archive file paths.

!!! note "TODO"
    Needs a better description

---
### `OpenBaseResource`

<listhead>Parameters</listhead>

* <type>number</type> <arg>index</arg>: The base resource path index to open

**Returns** <type>boolean</type>

Opens the base resource archive from the path at <arg>index</arg> in the list of base resource archive file paths (see <code>[BaseResourcePaths](#baseresourcepaths)</code>).

!!! note "TODO"
    Needs a better description

---
### `ProgramResource`

**Returns** <type>[Archive](../Types/Archive.md)</type>

Returns the program resource archive (either `slade.pk3` or the `res` folder if you are running a dev build).

---
### `RecentFiles`

**Returns** <type>string[]</type>

Returns an array of file paths to recently opened archives.

---
### `Bookmarks`

**Returns** <type>[ArchiveEntry](../Types/ArchiveEntry.md)`[`]</type>

Returns an array of all currently bookmarked entries.

---
### `AddBookmark`

<listhead>Parameters</listhead>

* <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type> <arg>entry</arg>: The entry to bookmark

Adds <arg>entry</arg> as a bookmark.

---
### `RemoveBookmark`

<listhead>Parameters</listhead>

* <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type> <arg>entry</arg>: The entry to un-bookmark

**Returns** <type>boolean</type>

Removes <arg>entry</arg> from the bookmarked entries list. Returns `false` if the given <arg>entry</arg> was not currently bookmarked.

---
### `EntryType`

<listhead>Parameters</listhead>

* <type>string</type> <arg>id</arg>: The id of the <type>[EntryType](../Types/EntryType.md)</type> to get

**Returns** <type>[EntryType](../Types/EntryType.md)</type>

Returns the <type>[EntryType](../Types/EntryType.md)</type> with the given <arg>id</arg>, or `nil` if no type has that id.

**Example**

```lua
-- Will write 'Wad Archive' to the log
local type = Archives.EntryType('wad')
App.LogMessage(type.name)
```
