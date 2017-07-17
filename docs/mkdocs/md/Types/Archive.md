The **Archive** type represents an archive (wad/pk3/etc) in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>filename</prop> | <type>string</type> | The full path to the archive file on disk
<prop>entries</prop> | <type>[ArchiveEntry](ArchiveEntry.md)\[\]</type> | An array of all entries in the archive
<prop>rootDir</prop>  | <type>[ArchiveDir](ArchiveDir.md)</type> | The root directory of the archive

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

## Functions

### `getDir`

<params>Parameters</params>

* <type>string</type> <arg>path</arg>: The path of the directory to get

**Returns** <type>[ArchiveDir](ArchiveDir.md)</type>

Returns the directory in the archive at <arg>path</arg>, or `null` if the path does not exist. If the archive does not support directories (eg. Doom Wad format) the 'root' directory is always returned, regardless of <arg>path</arg>.

---
### `createEntry`

<params>Parameters</params>

* <type>string</type> <arg>fullPath</arg>: The full path and name of the entry to create
* <type>number</type> <arg>position</arg>: The position to insert the entry

**Returns** <type>[ArchiveEntry](ArchiveEntry.md)</type>

Creates a new entry named <arg>fullPath</arg> in the Archive. If the Archive is a format that supports directories, <arg>fullPath</arg> can optionally contain a path eg. `Scripts/NewScript.txt`.

The new entry will be inserted at <arg>position</arg> in the directory it is added to (always the root for Archives that don't support directories). If <arg>position</arg> is negative or larger than the number of entries in the destination directory, the new entry will be added at the end.

**Example**

```lua
-- Create entry in the root directory of a zip, after all other entries
newEntry = zip.createEntry('InRoot.txt', -1)

-- Create entry in a subdirectory of a zip, before all other entries in the subdirectory
newEntry = zip.createEntry('Path/To/NewEntry.txt', 0)

-- Create entry in the middle of a wad somewhere
newEntry = wad.createEntry('NEWENTRY', 12)
```

---
### `createEntryInNamespace`

<params>Parameters</params>

* <type>string</type> <arg>name</arg>: The name of the entry
* <type>string</type> <arg>namespace</arg>: The namespace to add the entry to

**Returns** <type>[ArchiveEntry](ArchiveEntry.md)</type>

Creates a new entry named <arg>name</arg> in the Archive, at the end of <arg>namespace</arg>. If the Archive supports directories, <arg>namespace</arg> can be a path.

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
### `removeEntry`

<params>Parameters</params>

* <type>[ArchiveEntry](ArchiveEntry.md)</type> <arg>entry</arg>: The entry to remove

**Returns** <type>boolean</type>

Removes the given entry from the archive (but does not delete it). Returns `false` if the entry was not found in the archive.

---
### `renameEntry`

<params>Parameters</params>

* <type>[ArchiveEntry](ArchiveEntry.md)</type> <arg>entry</arg>: The entry to rename
* <type>string</type> <arg>name</arg>: The new name for the entry

**Returns** <type>boolean</type>

Renames the given entry. Returns `false` if the entry was not found in the archive.
