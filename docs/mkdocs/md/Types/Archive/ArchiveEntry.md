<article-head>ArchiveEntry</article-head>

The <type>ArchiveEntry</type> type represents an entry in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">name</prop> | <type>string</type> | The entry name.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt` this will be `Gun1.txt`
<prop class="ro">path</prop> | <type>string</type> | The path to the entry in the archive, beginning and ending with `/`.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt`, this will be `/Actors/Weapons/`. If the entry isn't in a directory this will be just `/`
<prop class="ro">type</prop> | <type>[EntryType](EntryType.md)</type> | The entry's type information
<prop class="ro">size</prop> | <type>number</type> | The size of the entry in bytes
<prop class="ro">data</prop> | <type>string</type> | The entry's data
<prop class="ro">index</prop> | <type>number</type> | The index of the entry within its containing archive or directory
<prop class="ro">crc32</prop> | <type>number</type> | The 32-bit [crc](https://en.wikipedia.org/wiki/Cyclic_redundancy_check) value calculated from the entry's data

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

<listhead>See:</listhead>

* <code>[Archive.CreateEntry](Archive.md#createentry)</code>
* <code>[Archive.CreateEntryInNamespace](Archive.md#createentryinnamespace)</code>

## Functions - General

### FormattedName

<fdef>function <type>ArchiveEntry</type>.<func>FormattedName</func>(<arg>*self*</arg>, <arg>includePath</arg>, <arg>includeExtension</arg>, <arg>upperCase</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>[includePath]</arg> (<type>boolean</type>, default `true`): Whether to include the entry path
* <arg>[includeExtension]</arg> (<type>boolean</type>, default `true`): Whether to include the entry extension
* <arg>[upperCase]</arg> (<type>boolean</type>, default `false`): Whether to convert the entry name to uppercase

<listhead>Returns</listhead>

* <type>string</type>: A formatted name of the entry, depending on the parameters given

**Notes**

Note that <arg>upperCase</arg> will not affect the path.

---
### FormattedSize

<fdef>function <type>ArchiveEntry</type>.<func>FormattedSize</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>string</type>: The size of the entry in a formatted string, eg. `1.3kb`

## Functions - Data Import/Export

### ExportFile

<fdef>function <type>ArchiveEntry</type>.<func>ExportFile</func>(<arg>*self*</arg>, <arg>path</arg>)</fdef>

Exports the entry data to a file at <arg>path</arg>.

<listhead>Parameters</listhead>

* <arg>path</arg> (<type>string</type>): The full path to the file to export

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the export succeeded
* <type>string</type>: An error message if the export failed

**Notes**

If a file already exists at <arg>path</arg>, it will be overwritten.

---
### ImportData

<fdef>function <type>ArchiveEntry</type>.<func>ImportData</func>(<arg>*self*</arg>, <arg>data</arg>)</fdef>

Imports <arg>data</arg> into the entry.

<listhead>Parameters</listhead>

* <arg>data</arg> (<type>string</type>): The data to import

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed

---
### ImportEntry

<fdef>function <type>ArchiveEntry</type>.<func>ImportEntry</func>(<arg>*self*</arg>, <arg>entry</arg>)</fdef>

Imports (copies) the data from <arg>entry</arg>.

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>ArchiveEntry</type>): The entry to import data from

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed

---
### ImportFile

<fdef>function <type>ArchiveEntry</type>.<func>ImportFile</func>(<arg>*self*</arg>, <arg>path</arg>)</fdef>

Imports the file at the given <arg>path</arg> into the entry.

<listhead>Parameters</listhead>

* <arg>path</arg> (<type>string</type>): The full path to the file to import

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed
