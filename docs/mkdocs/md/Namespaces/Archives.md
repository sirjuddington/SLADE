<article-head>Archives</article-head>

The `Archives` scripting namespace contains functions for managing archives.

## Functions - Archive Management

### All

```lua
function Archives.All()
```

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive.md)\[\]</type>: An array of all currently open archives

---
### Create

```lua
function Archives.Create(formatId)
```

Creates a new archive of the format specified in <arg>formatId</arg>.

<listhead>Parameters</listhead>

* <type>string</type> <arg>formatId</arg>: The <type>[ArchiveFormat](../Types/ArchiveFormat.md)</type> id to create

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive.md)</type>: The created archive, or `nil` if creation failed
* <type>string</type>: An error message if creation failed

**Notes**

Currently only `wad` and `zip` formats are supported for creation.

**Example**

```lua
local archive, err = Archives.Create('wad')
if archive == nil then
    App.LogMessage('Error creating archive: ' .. err)
end
```

---
### OpenFile

```lua
function Archives.OpenFile(path)
```

Attempts to open the archive file at <arg>path</arg> on disk.

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The full path (on disk) of the archive file to open

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive.md)</type>: The opened archive, or `nil` if opening failed
* <type>string</type>: An error message if opening failed

---
### Close <sup>(1)</sup>

```lua
function Archives.Close(archive)
```

Closes the given <arg>archive</arg>.

<listhead>Parameters</listhead>

* <type>[Archive](../Types/Archive.md)</type> <arg>archive</arg>: The archive to close

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if <arg>archive</arg> is invalid or not currently open

**Notes**

!!! warning
    Please be careful when using the `Close*` functions - attempting to access a closed archive from a script will currently cause a crash.

---
### Close <sup>(2)</sup>

```lua
function Archives.Close(index)
```

Closes the archive at <arg>index</arg> in the list of currently open archives (see <code>[All](#All)</code>).

<listhead>Parameters</listhead>

* <type>number</type> <arg>index</arg>: The index of the archive to close

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given <arg>index</arg> is invalid

---
### CloseAll

```lua
function Archives.CloseAll()
```

Closes all currently open archives.

## Functions - Resource Archives

### BaseResource

```lua
function Archives.BaseResource()
```

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive.md)</type>: The currently loaded base resource archive

---
### BaseResourcePaths

```lua
function Archives.BaseResourcePaths()
```

<listhead>Returns</listhead>

* <type>string[]</type>: An array of configured base resource archive file paths

**Notes**

This is the list of base resource archive paths as seen in the base resource configuration dialog.

---
### OpenBaseResource

```lua
function Archives.OpenBaseResource(index)
```

Opens the base resource archive at <arg>index</arg> in the list of base resource archive file paths (see <code>[BaseResourcePaths](#baseresourcepaths)</code>).

<listhead>Parameters</listhead>

* <type>number</type> <arg>index</arg>: The base resource path index to open

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given index was out of range

---
### ProgramResource

```lua
function Archives.ProgramResource()
```

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive.md)</type>: the program resource archive (either `slade.pk3` or the `res` folder if you are running a dev build)

## Functions - Bookmarks

### Bookmarks

```lua
function Archives.Bookmarks()
```

<listhead>Returns</listhead>

* <type>[ArchiveEntry](../Types/ArchiveEntry.md)`[`]</type>: An array of all currently bookmarked entries

---
### AddBookmark

```lua
function Archives.AddBookmark(entry)
```

Adds <arg>entry</arg> as a bookmark.

<listhead>Parameters</listhead>

* <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type> <arg>entry</arg>: The entry to bookmark

---
### RemoveBookmark

```lua
function Archives.RemoveBookmark(entry)
```

Removes <arg>entry</arg> from the bookmarked entries list.

<listhead>Parameters</listhead>

* <type>[ArchiveEntry](../Types/ArchiveEntry.md)</type> <arg>entry</arg>: The entry to un-bookmark

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given <arg>entry</arg> was not currently bookmarked

## Functions - Misc

### FileExtensionsString

```lua
function Archives.FileExtensionsString()
```

<listhead>Returns</listhead>

* <type>string</type>: The extension filter string for all supported archive file types

**Notes**

See <code>[App.BrowseFile](App.md#browsefile)</code> and the [Open Archive](../Examples/OpenArchive.md) example for more information.

---
### RecentFiles

```lua
function Archives.RecentFiles()
```

<listhead>Returns</listhead>

* <type>string[]</type>: An array of file paths to recently opened archives

---
### EntryType

```lua
function Archives.EntryType(type)
```

<listhead>Parameters</listhead>

* <type>string</type> <arg>id</arg>: The id of the <type>[EntryType](../Types/EntryType.md)</type> to get

<listhead>Returns</listhead>

* <type>[EntryType](../Types/EntryType.md)</type>: The entry type with the given <arg>id</arg>, or `nil` if no type has that id

**Example**

```lua
-- Will write 'Wad Archive' to the log
local type = Archives.EntryType('wad')
App.LogMessage(type.name)
```
