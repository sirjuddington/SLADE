The **Archive** type represents an archive (wad/pk3/etc) in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>filename</prop> | <type>string</type> | The full path to the archive file on disk
<prop>entries</prop> | <type>[ArchiveEntry](ArchiveEntry.md)\[\]</type> | An array of all entries in the archive
<prop>rootDir</prop>  | <type>[ArchiveDir](ArchiveDir.md)</type> | The root directory of the archive

## Functions

### getDir

**Parameters**

* <type>string</type> <arg>path</arg>: The path of the directory to get

**Returns** <type>[ArchiveDir](ArchiveDir.md)</type>

Returns the directory in the archive at <arg>path</arg>, or `null` if the path does not exist. If the archive does not support directories (eg. Doom Wad format) the 'root' directory is always returned, regardless of <arg>path</arg>.

---
### createEntry

**Parameters**

* <type>string</type> <arg>fullPath</arg>: The full path and name of the entry to create
* <type>number</type> <arg>position</arg>: The position to insert the entry

**Returns** <type>[ArchiveEntry](ArchiveEntry.md)</type>

!!! note
    Description required

---
### createEntryInNamespace

**Parameters**

* <type>string</type> <arg>name</arg>: The name of the entry
* <type>string</type> <arg>namespace</arg>: The namespace to add the entry to

**Returns** <type>[ArchiveEntry](ArchiveEntry.md)</type>

!!! note
    Description required

---
### removeEntry

**Parameters**

* <type>[ArchiveEntry](ArchiveEntry.md)</type> <arg>entry</arg>: The entry to remove

**Returns** boolean

Removes the given entry from the archive (but does not delete it). Returns `false` if the entry was not found in the archive.

---
### renameEntry

**Parameters**

* <type>[ArchiveEntry](ArchiveEntry.md)</type> <arg>entry</arg>: The entry to rename
* <type>string</type> <arg>name</arg>: The new name for the entry

**Returns** boolean

Renames the given entry. Returns `false` if the entry was not found in the archive.
