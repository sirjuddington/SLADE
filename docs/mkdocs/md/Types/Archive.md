<article-head>Archive</article-head>

The `Archive` type represents an archive (wad/pk3/etc) in SLADE.

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

* <code>[Archives.Create](../Namespaces/Archives.md#create)</code>
* <code>[Archives.OpenFile](../Namespaces/Archives.md#openfile)</code>

## Functions - General

### DirAtPath

```lua
function Archive.DirAtPath(self, path)
```

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The path of the directory to get

<listhead>Returns</listhead>

* <type>[ArchiveDir](ArchiveDir.md)</type>: The directory in the archive at <arg>path</arg>, or `nil` if the path does not exist

**Notes**

If the archive does not support directories (eg. Doom Wad format) the 'root' directory is always returned, regardless of <arg>path</arg>.

---
### EntryAtPath

```lua
function Archive.EntryAtPath(self, path)
```

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The path of the entry to get

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The entry in the archive at <arg>path</arg>, or `nil` if no entry at the given path exists

**Notes**

If multiple entries exist with the same <arg>path</arg>, the first match is returned.

---
### FilenameNoPath

```lua
function Archive.FilenameNoPath(self)
```

<listhead>Returns</listhead>

* <type>string</type>: The archive's <prop>filename</prop> without the full path

**Example**

As an example, if <prop>filename</prop> is `C:/games/doom/archive.wad`, this will return `archive.wad`.

---
### Save

```lua
function Archive.Save(self, path)
```

Saves the archive to disk.

<listhead>Parameters</listhead>

* `[`<type>string</type> <arg>path</arg>`]`: The full path to the file to save as. Defaults to the archive's <prop>filename</prop> property if not given

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if saving succeeded
* <type>string</type>: An error message if saving failed

**Notes**

If <arg>path</arg> is given, this will work like 'Save As' - the archive will be saved to a new file at the given path, overwriting the file if it already exists. This will also update the <prop>filename</prop> property.

**Example**

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

```lua
function Archive.CreateEntry(self, fullPath, position)
```

Creates a new entry named <arg>fullPath</arg> in the archive at <arg>position</arg> within the target directory.

<listhead>Parameters</listhead>

* <type>string</type> <arg>fullPath</arg>: The full path and name of the entry to create
* <type>number</type> <arg>position</arg>: The position to insert the entry

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The created entry

**Notes**

If the Archive is a format that supports directories, <arg>fullPath</arg> can optionally contain a path eg. `Scripts/NewScript.txt`.

The new entry will be inserted at <arg>position</arg> in the directory it is added to (always the root for Archives that don't support directories). If <arg>position</arg> is negative or larger than the number of entries in the destination directory, the new entry will be added at the end.

**Example**

```lua
-- Create entry in the root directory of a zip, after all other entries
newEntry = zip:CreateEntry('InRoot.txt', -1)

-- Create entry in a subdirectory of a zip, before all other entries in the subdirectory
newEntry = zip:CreateEntry('Path/To/NewEntry.txt', 0)

-- Create entry in the middle of a wad somewhere
newEntry = wad:CreateEntry('NEWENTRY', 12)
```

---
### CreateEntryInNamespace

```lua
function Archive.CreateEntryInNamespace(self, name, namespace)
```

Creates a new entry named <arg>name</arg> in the Archive, at the end of <arg>namespace</arg>.

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the entry
* <type>string</type> <arg>namespace</arg>: The namespace to add the entry to

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The created entry

**Notes**

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

```lua
function Archive.RemoveEntry(self, entry)
```

Removes the given <arg>entry</arg> from the archive (but does not delete it).

<listhead>Parameters</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type> <arg>entry</arg>: The entry to remove

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the entry was not found in the archive

---
### RenameEntry

```lua
function Archive.RenameEntry(self, entry, name)
```

Renames the given entry.

<listhead>Parameters</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type> <arg>entry</arg>: The entry to rename
* <type>string</type> <arg>name</arg>: The new name for the entry

<listhead>Parameters</listhead>

* <type>boolean</type>: `false` if the entry was not found in the archive

## Functions - Entry Search

### FindFirst

```lua
function Archive.FindFirst(self, options)
```

<listhead>Parameters</listhead>

* <type>[ArchiveSearchOptions](ArchiveSearchOptions.md)</type> <arg>options</arg>: The search criteria

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The **first** entry found in the archive matching the given <arg>options</arg>, or `nil` if no match was found

**Notes**

If <prop>searchSubdirs</prop> is true in the <arg>options</arg>, subdirectories will be searched *after* the entries in the specified <prop>dir</prop>.

---
### FindLast

```lua
function Archive.FindLast(self, options)
```

<listhead>Parameters</listhead>

* <type>[ArchiveSearchOptions](ArchiveSearchOptions.md)</type> <arg>options</arg>: The search criteria

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)</type>: The **last** entry found in the archive matching the given <arg>options</arg>, or `nil` if no match was found

**Notes**

If <prop>searchSubdirs</prop> is true in the <arg>options</arg>, subdirectories will be searched *after* the entries in the specified <prop>dir</prop>.

---
### FindAll

```lua
function Archive.FindAll(self, options)
```

<listhead>Parameters</listhead>

* <type>[ArchiveSearchOptions](ArchiveSearchOptions.md)</type> <arg>options</arg>: The search criteria

<listhead>Returns</listhead>

* <type>[ArchiveEntry](ArchiveEntry.md)\[\]</type>: All entries found in the archive matching the given <arg>options</arg>, or an empty array if no match is found
