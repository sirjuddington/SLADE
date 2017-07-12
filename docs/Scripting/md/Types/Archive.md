The **Archive** type represents an archive (wad/pk3/etc) in SLADE.

## Properties

| Name | Type | Description |
|:-----|:-----|:------------|
`filename`  | `string` | The full path to the archive file on disk
`entries`   | <code>[ArchiveEntry](ArchiveEntry.md)</code> array | An array of all entries in the archive
`rootDir`   | <code>[ArchiveDir](ArchiveDir.md)</code> | The root <code>[ArchiveDir](ActrchiveDir.md)</code> of the archive

## Functions

### getDir

**Parameters**

* `string` **path**: The path of the directory to get

**Returns** <code>[ArchiveDir](ArchiveDir.md)</code>

Returns the directory in the archive at `path`, or `null` if the path does not exist. If the archive does not support directories (eg. Doom Wad format) the 'root' directory is always returned, regardless of `path`.

---
### createEntry

**Parameters**

* `string` **fullPath**: The full path and name of the entry to create
* `number` **position**: The position to insert the entry

**Returns** <code>[ArchiveEntry](ArchiveEntry.md)</code>

!!! note
    Description required

---
### createEntryInNamespace

**Parameters**

* `string` **name**: The name of the entry
* `string` **namespace**: The namespace to add the entry to

**Returns** <code>[ArchiveEntry](ArchiveEntry.md)</code>

!!! note
    Description required

---
### removeEntry

**Parameters**

* <code>[ArchiveEntry](ArchiveEntry.md)</code> **entry**: The entry to remove

**Returns** `boolean`

Removes the given entry from the archive (but does not delete it). Returns `false` if the entry was not found in the archive.

---
### renameEntry

**Parameters**

* <code>[ArchiveEntry](ArchiveEntry.md)</code> **entry**: The entry to rename
* `string` **name**: The new name for the entry

**Returns** `boolean`

Renames the given entry. Returns `false` if the entry was not found in the archive.
