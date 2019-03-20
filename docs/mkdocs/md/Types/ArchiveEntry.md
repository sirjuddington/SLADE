The **ArchiveEntry** type represents an entry in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">name</prop> | <type>string</type> | The entry name.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt` this will be `Gun1.txt`
<prop class="ro">path</prop> | <type>string</type> | The path to the entry in the archive, beginning and ending with `/`.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt`, this will be `/Actors/Weapons/`. If the entry isn't in a directory this will be just `/`
<prop class="ro">type</prop> | <type>[EntryType](EntryType.md)</type> | The entry's type information
<prop class="ro">size</prop> | <type>number</type> | The size of the entry in bytes
<prop class="ro">data</prop> | <type>string</type> | The entry's data
<prop class="ro">crc32</prop> | <type>number</type> | The 32-bit [crc](https://en.wikipedia.org/wiki/Cyclic_redundancy_check) value calculated from the entry's data

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

<listhead>See:</listhead>

* <code>[Archive:CreateEntry](Archive.md#createentry)</code>
* <code>[Archive:CreateEntryInNamespace](Archive.md#createentryinnamespace)</code>

## Functions - General

### `FormattedName`

<listhead>Parameters</listhead>

* `[`<type>boolean</type> <arg>includePath</arg> : `true]`: Whether to include the entry path
* `[`<type>boolean</type> <arg>includeExtension</arg> : `true]`: Whether to include the entry extension
* `[`<type>boolean</type> <arg>upperCase</arg> : `false]`: Whether to convert the entry name to uppercase

**Returns** <type>string</type>

Returns a formatted name of the entry, depending on the parameters given. Note that <arg>upperCase</arg> will not affect the path.

---
### `FormattedSize`

**Returns** <type>string</type>

Returns the size of the entry in a formatted string, eg. `1.3kb`

## Functions - Data Import/Export

### `ExportFile`

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The full path to the file to export

**Returns** <type>boolean</type>, <type>string</type>

Exports the entry data to a file at <arg>path</arg>. If a file already exists at <arg>path</arg>, it will be overwritten. Returns `false` and an error message if the export failed.

---
### `ImportData`

<listhead>Parameters</listhead>

* <type>string</type> <arg>data</arg>: The data to import

**Returns** <type>boolean</type>, <type>string</type>

Imports <arg>data</arg> into the entry. Returns `false` and an error message if the import failed.

---
### `ImportEntry`

<listhead>Parameters</listhead>

* <type>ArchiveEntry</type> <arg>entry</arg>: The entry to import data from

**Returns** <type>boolean</type>, <type>string</type>

Imports (copies) the data from <arg>entry</arg>. Returns `false` and an error message if the import failed.

---
### `ImportFile`

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The full path to the file to import

**Returns** <type>boolean</type>, <type>string</type>

Imports the file at the given <arg>path</arg> into the entry. Returns `false` and an error message if the import failed.
