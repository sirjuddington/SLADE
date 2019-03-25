<article-head>ArchiveEntry</article-head>

The `ArchiveEntry` type represents an entry in SLADE.

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

### FormattedName

```lua
function ArchiveEntry.FormattedName(self, includePath, includeExtension, upperCase)
```

<listhead>Parameters</listhead>

* `[`<type>boolean</type> <arg>includePath</arg>`]`: Whether to include the entry path. Defaults to `true` if not given
* `[`<type>boolean</type> <arg>includeExtension</arg>`]`: Whether to include the entry extension. Defaults to `true` if not given
* `[`<type>boolean</type> <arg>upperCase</arg>`]`: Whether to convert the entry name to uppercase. Defaults to `false` if not given

<listhead>Returns</listhead>

* <type>string</type>: A formatted name of the entry, depending on the parameters given

**Notes**

Note that <arg>upperCase</arg> will not affect the path.

---
### FormattedSize

```lua
function ArchiveEntry.FormattedSize(self)
```

<listhead>Returns</listhead>

* <type>string</type>: The size of the entry in a formatted string, eg. `1.3kb`

## Functions - Data Import/Export

### ExportFile

```lua
function ArchiveEntry.ExportFile(self, path)
```

Exports the entry data to a file at <arg>path</arg>.

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The full path to the file to export

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the export succeeded
* <type>string</type>: An error message if the export failed

**Notes**

If a file already exists at <arg>path</arg>, it will be overwritten.

---
### ImportData

```lua
function ArchiveEntry.ImportData(self, data)
```

Imports <arg>data</arg> into the entry.

<listhead>Parameters</listhead>

* <type>string</type> <arg>data</arg>: The data to import

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed

---
### ImportEntry

```lua
function ArchiveEntry.ImportEntry(self, entry)
```

Imports (copies) the data from <arg>entry</arg>.

<listhead>Parameters</listhead>

* <type>ArchiveEntry</type> <arg>entry</arg>: The entry to import data from

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed

---
### ImportFile

```lua
function ArchiveEntry.ImportFile(self, path)
```

Imports the file at the given <arg>path</arg> into the entry.

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The full path to the file to import

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the import succeeded
* <type>string</type>: An error message if the import failed
