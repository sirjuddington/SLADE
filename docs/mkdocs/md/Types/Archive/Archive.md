<subhead>Type</subhead>
<header>Archive</header>

The <type>Archive</type> type represents an archive (wad/pk3/etc) in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">filename</prop> | <type>string</type> | The full path to the archive file on disk
<prop class="ro">entries</prop> | <type>[ArchiveEntry](ArchiveEntry.md)\[\]</type> | An array of all entries in the archive
<prop class="ro">rootDir</prop> | <type>[ArchiveDir](ArchiveDir.md)</type> | The root directory of the archive
<prop class="ro">format</prop> | <type>[ArchiveFormat](ArchiveFormat.md)</type> | Information about the archive's format

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Archives.Create](../../Namespaces/Archives.md#create)</code>
* <code>[Archives.OpenFile](../../Namespaces/Archives.md#openfile)</code>

## Functions

### Overview

#### General

<fdef>[DirAtPath](#diratpath)(<arg>path</arg>) -> <type>[ArchiveDir](ArchiveDir.md)</type></fdef>
<fdef>[EntryAtPath](#entryatpath)(<arg>path</arg>) -> <type>[ArchiveEntry](ArchiveEntry.md)</type></fdef>
<fdef>[FilenameNoPath](#filenamenopath)() -> <type>string</type></fdef>
<fdef>[Save](#save)(<arg>[path]</arg>) -> <type>boolean</type>, <type>string</type></fdef>

#### Entry Manipulation

<fdef>[CreateDir](#createdir)(<arg>path</arg>) -> <type>[ArchiveDir](ArchiveDir.md)</type></fdef>
<fdef>[CreateEntry](#createentry)(<arg>fullPath</arg>, <arg>index</arg>) -> <type>[ArchiveEntry](ArchiveEntry.md)</type></fdef>
<fdef>[CreateEntryInNamespace](#createentryinnamespace)(<arg>name</arg>, <arg>namespace</arg>) -> <type>[ArchiveEntry](ArchiveEntry.md)</type></fdef>
<fdef>[RemoveEntry](#removeentry)(<arg>entry</arg>) -> <type>boolean</type></fdef>
<fdef>[RenameEntry](#renameentry)(<arg>entry</arg>, <arg>name</arg>) -> <type>boolean</type></fdef>

#### Entry Search

<fdef>[FindFirst](#findfirst)(<arg>options</arg>) -> <type>[ArchiveEntry](ArchiveEntry.md)</type></fdef>
<fdef>[FindLast](#findlast)(<arg>options</arg>) -> <type>[ArchiveEntry](ArchiveEntry.md)</type></fdef>
<fdef>[FindAll](#findall)(<arg>options</arg>) -> <type>[ArchiveEntry](ArchiveEntry.md)\[\]</type></fdef>

---
### DirAtPath

#### Parameters

* <arg>path</arg> (<type>string</type>): The path of the directory to get

#### Returns

* <type>[ArchiveDir](ArchiveDir.md)</type>: The directory in the archive at <arg>path</arg>, or `nil` if the path does not exist

#### Notes

If the archive does not support directories (eg. Doom Wad format) the 'root' directory is always returned, regardless of <arg>path</arg>.

---
### EntryAtPath

#### Parameters

* <arg>path</arg> (<type>string</type>): The path of the entry to get

#### Returns

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The entry in the archive at <arg>path</arg>, or `nil` if no entry at the given path exists

#### Notes

If multiple entries exist with the same <arg>path</arg>, the first match is returned.

---
### FilenameNoPath

Gets the archive's <prop>filename</prop> without the full path

#### Returns

* <type>string</type>: The archive's <prop>filename</prop> without the full path

#### Example

```lua
local archive = Archives.OpenFile('C:/games/doom/archive.wad')
App.LogMessage(archive:FilenameNoPath) -- 'archive.wad'
```

---
### Save

Saves the archive to disk.

#### Parameters

* <arg>[path]</arg> (<type>string</type>): The full path to the file to save as. Default is `""`, which will use the archive's existing <prop>filename</prop>

#### Returns

* <type>boolean</type>: `true` if saving succeeded
* <type>string</type>: An error message if saving failed

#### Notes

If <arg>path</arg> is given, this will work like 'Save As' - the archive will be saved to a new file at the given path, overwriting the file if it already exists. This will also update the <prop>filename</prop> property.

#### Example

```lua
-- Open an archive
local archive = Archives.OpenFile('c:/filename.wad')
App.LogMessage(archive.filename) -- 'c:/filename.wad'

-- Save to existing file (c:/filename.wad)
local ok, err = archive:save()
if not ok then
    App.LogMessage('Failed to save: ' .. err)
end

-- Save as new file
ok, err = archive:Save('c:/newfile.wad')
if not ok then
    App.LogMessage('Failed to save as new file: ' .. err)
end

App.LogMessage(archive.filename) -- 'c:/newfile.wad'
```

---
### CreateDir

Creates a new directory at <arg>path</arg> in the archive. Has no effect if the archive format doesn't support directories.

#### Parameters

* <arg>path</arg> (<type>string</type>): The path of the directory to create

#### Returns

* <type>[ArchiveDir](ArchiveDir.md)</type>: The directory that was created or `nil` if the archive format doesn't support directories

---
### CreateEntry

Creates a new entry named <arg>fullPath</arg> in the archive at <arg>index</arg> within the target directory.

#### Parameters

* <arg>fullPath</arg> (<type>string</type>): The full path and name of the entry to create
* <arg>index</arg> (<type>integer</type>): The index to insert the entry

#### Returns

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The created entry

#### Notes

If the Archive is a format that supports directories, <arg>fullPath</arg> can optionally contain a path eg. `Scripts/NewScript.txt`.

The new entry will be inserted at <arg>index</arg> in the directory it is added to (always the root for Archives that don't support directories). If <arg>index</arg> is `-1` or larger than the number of entries in the destination directory, the new entry will be added at the end.

#### Example

```lua
-- Create entry in the root directory of a zip, after all other entries
newEntry = zip:CreateEntry('InRoot.txt', 0)

-- Create entry in a subdirectory of a zip, before all other entries in the subdirectory
newEntry = zip:CreateEntry('Path/To/NewEntry.txt', 1)

-- Create entry in the middle of a wad somewhere
newEntry = wad:CreateEntry('NEWENTRY', 12)
```

---
### CreateEntryInNamespace

Creates a new entry named <arg>name</arg> in the Archive, at the end of <arg>namespace</arg>.

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the entry
* <arg>namespace</arg> (<type>string</type>): The namespace to add the entry to

#### Returns

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The created entry

#### Notes

If the Archive supports directories, <arg>namespace</arg> can be a path.

See below for a list of supported namespaces:

| Namespace | Wad Archive Markers | Zip Archive Directory |
|-----------|---------------------|-----------------------|
`patches` | `P_START` / `P_END` | `patches`
`sprites` | `S_START` / `S_END` | `sprites`
`flats` | `F_START` / `F_END` | `flats`
`textures` | `TX_START` / `TX_END` | `textures`
`hires` | `HI_START` / `HI_END` | `hires`
`colormaps` | `C_START` / `C_END` | `colormaps`
`acs` | `A_START` / `A_END` | `acs`
`voices` | `V_START` / `V_END` | `voices`
`voxels` | `VX_START` / `VX_END` | `voxels`
`sounds` | `DS_START` / `DS_END` | `sounds`

---
### RemoveEntry

Removes the given <arg>entry</arg> from the archive (but does not delete it).

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](ArchiveEntry.md)</type>): The entry to remove

#### Returns

* <type>boolean</type>: `false` if the entry was not found in the archive

---
### RenameEntry

Renames the given entry.

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](ArchiveEntry.md)</type>): The entry to rename
* <arg>name</arg> (<type>string</type>): The new name for the entry

#### Returns

* <type>boolean</type>: `false` if the entry was not found in the archive

---
### FindFirst

#### Parameters

* <arg>options</arg> (<type>[ArchiveSearchOptions](ArchiveSearchOptions.md)</type>): The search criteria

#### Returns

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The **first** entry found in the archive matching the given <arg>options</arg>, or `nil` if no match was found

#### Notes

If <prop>searchSubdirs</prop> is true in the <arg>options</arg>, subdirectories will be searched *after* the entries in the specified <prop>dir</prop>.

---
### FindLast

#### Parameters

* <arg>options</arg> (<type>[ArchiveSearchOptions](ArchiveSearchOptions.md)</type>): The search criteria

#### Returns

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The **last** entry found in the archive matching the given <arg>options</arg>, or `nil` if no match was found

#### Notes

If <prop>searchSubdirs</prop> is true in the <arg>options</arg>, subdirectories will be searched *after* the entries in the specified <prop>dir</prop>.

---
### FindAll

#### Parameters

* <arg>options</arg> (<type>[ArchiveSearchOptions](ArchiveSearchOptions.md)</type>): The search criteria

#### Returns

* <type>[ArchiveEntry](ArchiveEntry.md)\[\]</type>: All entries found in the archive matching the given <arg>options</arg>, or an empty array if no match is found
