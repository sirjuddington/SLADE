<subhead>Type</subhead>
<header>ArchiveEntry</header>

The <type>ArchiveEntry</type> type represents an entry in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">name</prop> | <type>string</type> | The entry name.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt` this will be `Gun1.txt`
<prop class="ro">path</prop> | <type>string</type> | The path to the entry in the archive, beginning and ending with `/`.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt`, this will be `/Actors/Weapons/`. If the entry isn't in a directory this will be just `/`
<prop class="ro">type</prop> | <type>[EntryType](EntryType.md)</type> | The entry's type information
<prop class="ro">size</prop> | <type>integer</type> | The size of the entry in bytes
<prop class="ro">data</prop> | <type>[DataBlock](../DataBlock.md)</type> | The entry's data
<prop class="ro">index</prop> | <type>integer</type> | The index of the entry within its containing archive or directory
<prop class="ro">crc32</prop> | <type>integer</type> | The 32-bit [crc](https://en.wikipedia.org/wiki/Cyclic_redundancy_check) value calculated from the entry's data
<prop class="ro">parentArchive</prop> | <type>[Archive](Archive.md)</type> | The <type>Archive</type> that contains this entry
<prop class="ro">parentDir</prop> | <type>[ArchiveDir](ArchiveDir.md)</type> | The <type>ArchiveDir</type> that contains this entry

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Archive.CreateEntry](Archive.md#createentry)</code>
* <code>[Archive.CreateEntryInNamespace](Archive.md#createentryinnamespace)</code>

## Functions

### Overview

#### General

<fdef>[FormattedName](#formattedname)(<arg>[includePath]</arg>, <arg>[includeExtension]</arg>, <arg>[upperCase]</arg>) -> <type>string</type></fdef>
<fdef>[FormattedSize](#formattedsize)() -> <type>string</type></fdef>
<fdef>[Rename](#rename)(<arg>newName</arg>)</fdef>

#### Data Import/Export

<fdef>[ExportFile](#exportfile)(<arg>path</arg>) -> <type>boolean</type>, <type>string</type></fdef>
<fdef>[ImportData](#importdata-1)(<arg>dataString</arg>) -> <type>boolean</type>, <type>string</type></fdef>
<fdef>[ImportData](#importdata-2)(<arg>data</arg>) -> <type>boolean</type>, <type>string</type></fdef>
<fdef>[ImportEntry](#importentry)(<arg>entry</arg>) -> <type>boolean</type>, <type>string</type></fdef>
<fdef>[ImportFile](#importfile)(<arg>path</arg>) -> <type>boolean</type>, <type>string</type></fdef>

---
### FormattedName

Gets the entry name, formatted depending on the parameters given.

#### Parameters

* <arg>[includePath]</arg> (<type>boolean</type>): Whether to include the entry path. Default is `true`
* <arg>[includeExtension]</arg> (<type>boolean</type>): Whether to include the entry extension. Default is `true`
* <arg>[upperCase]</arg> (<type>boolean</type>): Whether to convert the entry name to uppercase. Default is `false`

#### Returns

* <type>string</type>: A formatted name of the entry, depending on the parameters given

#### Notes

Note that <arg>upperCase</arg> will not affect the path.

---
### FormattedSize

Gets the size of the entry in a formatted string, eg. `1.3kb`

#### Returns

* <type>string</type>: The size of the entry

---
### Rename

Renames the entry.

#### Parameters

* <arg>newName</arg> (<type>string</type>): The new name for the entry

#### Notes

If the entry has a <prop>parent</prop> <type>[Archive](Archive.md)</type>, the archive's naming rules will be applied to the new name (see [ArchiveFormat](ArchiveFormat.md)).

---
### ExportFile

Exports the entry data to a file at <arg>path</arg>.

#### Parameters

* <arg>path</arg> (<type>string</type>): The full path to the file to export

#### Returns

* <type>boolean</type>: `true` if the export succeeded
* <type>string</type>: An error message if the export failed

#### Notes

If a file already exists at <arg>path</arg>, it will be overwritten.

---
### ImportData <sup>(1)</sup>

Imports <arg>dataString</arg> into the entry.

#### Parameters

* <arg>dataString</arg> (<type>string</type>): The data to import

#### Returns

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed

---
### ImportData <sup>(2)</sup>

Imports <arg>data</arg> into the entry.

#### Parameters

* <arg>data</arg> (<type>[DataBlock](../DataBlock.md)</type>): The data to import

#### Returns

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed

---
### ImportEntry

Imports (copies) the data from <arg>entry</arg>.

#### Parameters

* <arg>entry</arg> (<type>ArchiveEntry</type>): The entry to import data from

#### Returns

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed

---
### ImportFile

Imports the file at the given <arg>path</arg> into the entry.

#### Parameters

* <arg>path</arg> (<type>string</type>): The full path to the file to import

#### Returns

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed
