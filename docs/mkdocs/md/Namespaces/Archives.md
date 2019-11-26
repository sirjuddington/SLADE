<article-head>Archives</article-head>

The `Archives` scripting namespace contains functions for managing archives.

## Functions - Archive Management

### All

<fdef>function Archives.<func>All</func>()</fdef>

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive/Archive.md)\[\]</type>: An array of all currently open archives

---
### Create

<fdef>function Archives.<func>Create</func>(<arg>formatId</arg>)</fdef>

Creates a new archive of the format specified in <arg>formatId</arg>.

<listhead>Parameters</listhead>

* <arg>formatId</arg> (<type>string</type>): The <type>[ArchiveFormat](../Types/Archive/ArchiveFormat.md)</type> id to create

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive/Archive.md)</type>: The created archive, or `nil` if creation failed
* <type>string</type>: An error message if creation failed

#### Notes

Currently only `wad` and `zip` formats are supported for creation.

#### Example

```lua
local archive, err = Archives.Create('wad')
if archive == nil then
    App.LogMessage('Error creating archive: ' .. err)
end
```

---
### OpenFile

<fdef>function Archives.<func>OpenFile</func>(<arg>path</arg>)</fdef>

Attempts to open the archive file at <arg>path</arg> on disk.

<listhead>Parameters</listhead>

* <arg>path</arg> (<type>string</type>): The full path (on disk) of the archive file to open

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive/Archive.md)</type>: The opened archive, or `nil` if opening failed
* <type>string</type>: An error message if opening failed

---
### Close <sup>(1)</sup>

<fdef>function Archives.<func>Close</func>(<arg>archive</arg>)</fdef>

Closes the given <arg>archive</arg>.

<listhead>Parameters</listhead>

* <arg>archive</arg> (<type>[Archive](../Types/Archive/Archive.md)</type>): The archive to close

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if <arg>archive</arg> is invalid or not currently open

#### Notes

!!! warning
    Please be careful when using the `Close*` functions - attempting to access a closed archive from a script will currently cause a crash.

---
### Close <sup>(2)</sup>

<fdef>function Archives.<func>Close</func>(<arg>index</arg>)</fdef>

Closes the archive at <arg>index</arg> in the list of currently open archives (see <code>[All](#All)</code>).

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The index of the archive to close

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given <arg>index</arg> is invalid

---
### CloseAll

<fdef>function Archives.<func>CloseAll</func>()</fdef>

Closes all currently open archives.

## Functions - Resource Archives

### BaseResource

<fdef>function Archives.<func>BaseResource</func>()</fdef>

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive/Archive.md)</type>: The currently loaded base resource archive

---
### BaseResourcePaths

<fdef>function Archives.<func>BaseResourcePaths</func>()</fdef>

<listhead>Returns</listhead>

* <type>string[]</type>: An array of configured base resource archive file paths

#### Notes

This is the list of base resource archive paths as seen in the base resource configuration dialog.

---
### OpenBaseResource

<fdef>function Archives.<func>OpenBaseResource</func>(<arg>index</arg>)</fde>

Opens the base resource archive at <arg>index</arg> in the list of base resource archive file paths (see <code>[BaseResourcePaths](#baseresourcepaths)</code>).

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The base resource path index to open

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given index was out of range

---
### ProgramResource

<fdef>function Archives.<func>ProgramResource</func>()</fdef>

<listhead>Returns</listhead>

* <type>[Archive](../Types/Archive/Archive.md)</type>: the program resource archive (either `slade.pk3` or the `res` folder if you are running a dev build)

## Functions - Bookmarks

### Bookmarks

<fdef>function Archives.<func>Bookmarks</func>()</fdef>

<listhead>Returns</listhead>

* <type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)`[`]</type>: An array of all currently bookmarked entries

---
### AddBookmark

<fdef>function Archives.<func>AddBookmark</func>(<arg>entry</arg>)</fdef>

Adds <arg>entry</arg> as a bookmark.

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>): The entry to bookmark

---
### RemoveBookmark

<fdef>function Archives.<func>RemoveBookmark</func>(<arg>entry</arg>)</fdef>

Removes <arg>entry</arg> from the bookmarked entries list.

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>): The entry to un-bookmark

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given <arg>entry</arg> was not currently bookmarked

## Functions - Misc

### FileExtensionsString

<fdef>function Archives.<func>FileExtensionsString</func>()</fdef>

<listhead>Returns</listhead>

* <type>string</type>: The extension filter string for all supported archive file types

#### Notes

See <code>[App.BrowseFile](App.md#browsefile)</code> and the [Open Archive](../Examples/OpenArchive.md) example for more information.

---
### RecentFiles

<fdef>function Archives.<func>RecentFiles</func>()</fdef>

<listhead>Returns</listhead>

* <type>string[]</type>: An array of file paths to recently opened archives

---
### EntryType

<fdef>function Archives.<func>EntryType</func>(<arg>type</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>id</arg> (<type>string</type>): The id of the <type>[EntryType](../Types/Archive/EntryType.md)</type> to get

<listhead>Returns</listhead>

* <type>[EntryType](../Types/Archive/EntryType.md)</type>: The entry type with the given <arg>id</arg>, or `nil` if no type has that id

#### Example

```lua
-- Will write 'Wad Archive' to the log
local type = Archives.EntryType('wad')
App.LogMessage(type.name)
```
