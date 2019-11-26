<article-head>Archive</article-head>

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

<listhead>See:</listhead>

* <code>[Archives.Create](../../Namespaces/Archives.md#create)</code>
* <code>[Archives.OpenFile](../../Namespaces/Archives.md#openfile)</code>

## Functions - General

### DirAtPath

<fdef>function <type>Archive</type>.<func>DirAtPath</func>(<arg>*self*</arg>, <arg>path</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>path</arg> (<type>string</type>): The path of the directory to get

<listhead>Returns</listhead>

* <type>[ArchiveDir](ArchiveDir.md)</type>: The directory in the archive at <arg>path</arg>, or `nil` if the path does not exist

#### Notes

If the archive does not support directories (eg. Doom Wad format) the 'root' directory is always returned, regardless of <arg>path</arg>.

---
### EntryAtPath

<fdef>function <type>Archive</type>.<func>EntryAtPath</func>(<arg>*self*</arg>, <arg>path</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>path</arg> (<type>string</type>): The path of the entry to get

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The entry in the archive at <arg>path</arg>, or `nil` if no entry at the given path exists

#### Notes

If multiple entries exist with the same <arg>path</arg>, the first match is returned.

---
### FilenameNoPath

<fdef>function <type>Archive</type>.<func>FilenameNoPath</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>string</type>: The archive's <prop>filename</prop> without the full path

#### Example

```lua
local archive = Archives.OpenFile('C:/games/doom/archive.wad')
App.LogMessage(archive:FilenameNoPath) -- 'archive.wad'
```

---
### Save

<fdef>function <type>Archive</type>.<func>Save</func>(<arg>*self*</arg>, <arg>path</arg>)</fdef>

Saves the archive to disk.

<listhead>Parameters</listhead>

* <arg>[path]</arg> (<type>string</type>, defaults to <prop>filename</prop>): The full path to the file to save as

<listhead>Returns</listhead>

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

## Functions - Entry Manipulation

### CreateEntry

<fdef>function <type>Archive</type>.<func>CreateEntry</func>(<arg>*self*</arg>, <arg>fullPath</arg>, <arg>index</arg>)</fdef>

Creates a new entry named <arg>fullPath</arg> in the archive at <arg>index</arg> within the target directory.

<listhead>Parameters</listhead>

* <arg>fullPath</arg> (<type>string</type>): The full path and name of the entry to create
* <arg>index</arg> (<type>number</type>): The index to insert the entry

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The created entry

#### Notes

If the Archive is a format that supports directories, <arg>fullPath</arg> can optionally contain a path eg. `Scripts/NewScript.txt`.

The new entry will be inserted at <arg>index</arg> in the directory it is added to (always the root for Archives that don't support directories). If <arg>index</arg> is `0` or larger than the number of entries in the destination directory, the new entry will be added at the end.

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

<fdef>function <type>Archive</type>.<func>CreateEntryInNamespace</func>(<arg>*self*</arg>, <arg>name</arg>, <arg>namespace</arg>)</fdef>

Creates a new entry named <arg>name</arg> in the Archive, at the end of <arg>namespace</arg>.

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the entry
* <arg>namespace</arg> (<type>string</type>): The namespace to add the entry to

<listhead>Returns</listhead>

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

<fdef>function <type>Archive</type>.<func>RemoveEntry</func>(<arg>*self*</arg>, <arg>entry</arg>)</fdef>

Removes the given <arg>entry</arg> from the archive (but does not delete it).

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>[ArchiveEntry](ArchiveEntry.md)</type>): The entry to remove

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the entry was not found in the archive

---
### RenameEntry

<fdef>function <type>Archive</type>.<func>RenameEntry</func>(<arg>*self*</arg>, <arg>entry</arg>, <arg>name</arg>)</fdef>

Renames the given entry.

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>[ArchiveEntry](ArchiveEntry.md)</type>): The entry to rename
* <arg>name</arg> (<type>string</type>): The new name for the entry

<listhead>Parameters</listhead>

* <type>boolean</type>: `false` if the entry was not found in the archive

## Functions - Entry Search

### FindFirst

<fdef>function <type>Archive</type>.<func>FindFirst</func>(<arg>*self*</arg>, <arg>options</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>options</arg> (<type>[ArchiveSearchOptions](ArchiveSearchOptions.md)</type>): The search criteria

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The **first** entry found in the archive matching the given <arg>options</arg>, or `nil` if no match was found

#### Notes

If <prop>searchSubdirs</prop> is true in the <arg>options</arg>, subdirectories will be searched *after* the entries in the specified <prop>dir</prop>.

---
### FindLast

<fdef>function <type>Archive</type>.<func>FindLast</func>(<arg>*self*</arg>, <arg>options</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>options</arg> (<type>[ArchiveSearchOptions](ArchiveSearchOptions.md)</type>): The search criteria

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The **last** entry found in the archive matching the given <arg>options</arg>, or `nil` if no match was found

#### Notes

If <prop>searchSubdirs</prop> is true in the <arg>options</arg>, subdirectories will be searched *after* the entries in the specified <prop>dir</prop>.

---
### FindAll

<fdef>function <type>Archive</type>.<func>FindAll</func>(<arg>*self*</arg>, <arg>options</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>options</arg> (<type>[ArchiveSearchOptions](ArchiveSearchOptions.md)</type>): The search criteria

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)\[\]</type>: All entries found in the archive matching the given <arg>options</arg>, or an empty array if no match is found
