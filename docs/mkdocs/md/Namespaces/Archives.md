The `Archives` scripting namespace contains functions for managing archives.

## Functions

### Overview

#### Archive Management

<fdef>[All](#all)() -> <type>[Archive](../Types/Archive/Archive.md)\[\]</type></fdef>
<fdef>[Create](#create)(<arg>formatId</arg>) -> <type>[Archive](../Types/Archive/Archive.md)</type>, <type>string</type></fdef>
<fdef>[OpenFile](#openfile)(<arg>path</arg>) -> <type>[Archive](../Types/Archive/Archive.md)</type>, <type>string</type></fdef>
<fdef>[Close](#close-1)(<arg>archive</arg>) -> <type>boolean</type></fdef>
<fdef>[Close](#close-2)(<arg>index</arg>) -> <type>boolean</type></fdef>
<fdef>[CloseAll](#closeall)()</fdef>

#### Resource Archives

<fdef>[BaseResource](#baseresource)() -> <type>[Archive](../Types/Archive/Archive.md)</type></fdef>
<fdef>[BaseResourcePaths](#baseresourcepaths)() -> <type>string[]</type></fdef>
<fdef>[OpenBaseResource](#openbaseresource)(<arg>index</arg>) -> <type>boolean</type></fdef>
<fdef>[ProgramResource](#programresource)() -> <type>[Archive](../Types/Archive/Archive.md)</type></fdef>

#### Bookmarks

<fdef>[Bookmarks](#bookmarks)() -> <type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)\[\]</type></fdef>
<fdef>[AddBookmark](#addbookmark)(<arg>entry</arg>)</fdef>
<fdef>[RemoveBookmark](#removebookmark)(<arg>entry</arg>) -> <type>boolean</type></fdef>

#### Misc

<fdef>[FileExtensionsString](#fileextensionsstring)() -> <type>string</type></fdef>
<fdef>[RecentFiles](#recentfiles)() -> <type>string[]</type></fdef>
<fdef>[EntryType](#entrytype)(<arg>type</arg>) -> <type>[EntryType](../Types/Archive/EntryType.md)</type></fdef>

---
### All

#### Returns

* <type>[Archive](../Types/Archive/Archive.md)\[\]</type>: An array of all currently open archives

---
### Create

Creates a new archive of the format specified in <arg>formatId</arg>.

#### Parameters

* <arg>formatId</arg> (<type>string</type>): The <type>[ArchiveFormat](../Types/Archive/ArchiveFormat.md)</type> id to create

#### Returns

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

Attempts to open the archive file at <arg>path</arg> on disk.

#### Parameters

* <arg>path</arg> (<type>string</type>): The full path (on disk) of the archive file to open

#### Returns

* <type>[Archive](../Types/Archive/Archive.md)</type>: The opened archive, or `nil` if opening failed
* <type>string</type>: An error message if opening failed

---
### Close <sup>(1)</sup>

Closes the given <arg>archive</arg>.

#### Parameters

* <arg>archive</arg> (<type>[Archive](../Types/Archive/Archive.md)</type>): The archive to close

#### Returns

* <type>boolean</type>: `false` if <arg>archive</arg> is invalid or not currently open

#### Notes

!!! warning
    Please be careful when using the `Close*` functions - attempting to access a closed archive from a script will currently cause a crash.

---
### Close <sup>(2)</sup>

Closes the archive at <arg>index</arg> in the list of currently open archives (see <code>[All](#All)</code>).

#### Parameters

* <arg>index</arg> (<type>integer</type>): The index of the archive to close

#### Returns

* <type>boolean</type>: `false` if the given <arg>index</arg> is invalid

---
### CloseAll

Closes all currently open archives.

---
### BaseResource

Gets the currently loaded base resource archive.

#### Returns

* <type>[Archive](../Types/Archive/Archive.md)</type>: The currently loaded base resource archive

---
### BaseResourcePaths

Gets all configured base resource archive file paths.

#### Returns

* <type>string[]</type>: An array of configured base resource archive file paths

#### Notes

This is the list of base resource archive paths as seen in the base resource configuration dialog.

---
### OpenBaseResource

Opens the base resource archive at <arg>index</arg> in the list of base resource archive file paths (see <code>[BaseResourcePaths](#baseresourcepaths)</code>).

#### Parameters

* <arg>index</arg> (<type>integer</type>): The base resource path index to open

#### Returns

* <type>boolean</type>: `false` if the given index was out of range

---
### ProgramResource

Gets the program resource archive.

#### Returns

* <type>[Archive](../Types/Archive/Archive.md)</type>: the program resource archive (either `slade.pk3` or the `res` folder if you are running a dev build)

---
### Bookmarks

Gets all currently bookmarked entries.

#### Returns

* <type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)`[`]</type>: An array of all currently bookmarked entries

---
### AddBookmark

Adds <arg>entry</arg> as a bookmark.

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>): The entry to bookmark

---
### RemoveBookmark

Removes <arg>entry</arg> from the bookmarked entries list.

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](../Types/Archive/ArchiveEntry.md)</type>): The entry to un-bookmark

#### Returns

* <type>boolean</type>: `false` if the given <arg>entry</arg> was not currently bookmarked

---
### FileExtensionsString

Gets an extension filter string for all supported archive file types.

#### Returns

* <type>string</type>: The extension filter string for all supported archive file types

#### Notes

See <code>[UI.PromptOpenFile](UI.md#promptopenfile)</code> and the [Open Archive](../Examples/OpenArchive.md) example for more information.

---
### RecentFiles

Gets all recently opened archive file paths.

#### Returns

* <type>string[]</type>: An array of file paths to recently opened archives

---
### EntryType

Gets the entry type with the given <arg>id</arg>.

#### Parameters

* <arg>id</arg> (<type>string</type>): The id of the <type>[EntryType](../Types/Archive/EntryType.md)</type> to get

#### Returns

* <type>[EntryType](../Types/Archive/EntryType.md)</type>: The entry type with the given <arg>id</arg>, or `nil` if no type has that id

#### Example

```lua
-- Will write 'Wad Archive' to the log
local type = Archives.EntryType('wad')
App.LogMessage(type.name)
```
