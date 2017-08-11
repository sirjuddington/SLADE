The **ArchiveEntry** type represents an entry in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>name</prop> | <type>string</type> | The entry name.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt` this will be `Gun1.txt`
<prop>path</prop> | <type>string</type> | The path to the entry in the archive, beginning and ending with `/`.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt`, this will be `/Actors/Weapons/`. If the entry isn't in a directory this will be just `/`
<prop>type</prop> | <type>[EntryType](EntryType.md)</type> | The entry's type information
<prop>size</prop> | <type>number</type> | The size of the entry in bytes
<prop>data</prop> | <type>string</type> | The entry's data
<prop>crc32</prop> | <type>number</type> | The 32-bit [crc](https://en.wikipedia.org/wiki/Cyclic_redundancy_check) value calculated from the entry's data

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

<listhead>See:</listhead>

* <code>[Archive:createEntry](Archive.md#createentry)</code>
* <code>[Archive:createEntryInNamespace](Archive.md#createentryinnamespace)</code>

## Functions - General

### `formattedName`

<listhead>Parameters</listhead>

* `[`<type>boolean</type> <arg>include_path</arg> : `true]`: Whether to include the entry path
* `[`<type>boolean</type> <arg>include_extension</arg> : `true]`: Whether to include the entry extension
* `[`<type>boolean</type> <arg>upper_case</arg> : `false]`: Whether to convert the entry name to uppercase

**Returns** <type>string</type>

Returns a formatted name of the entry, depending on the parameters given. Note that <arg>upper_case</arg> will not affect the path.

---
### `formattedSize`

**Returns** <type>string</type>

Returns the size of the entry in a formatted string, eg. `1.3kb`

## Functions - Data Import/Export

### `exportFile`

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The full path to the file to export

**Returns** <type>boolean</type>

Exports the entry data to a file at <arg>path</arg>. If a file already exists at <arg>path</arg>, it will be overwritten. Returns `true` if the export succeeded.

If the export fails, the error that occurred should be available via <code>[slade.globalError](../Namespaces/App.md#globalerror)()</code>.

---
### `importData`

<listhead>Parameters</listhead>

* <type>string</type> <arg>data</arg>: The data to import

**Returns** <type>boolean</type>

Imports <arg>data</arg> into the entry. Returns `true` if the import succeeded.

If the import fails, the error that occurred should be available via <code>[slade.globalError](../Namespaces/App.md#globalerror)()</code>.

---
### `importEntry`

<listhead>Parameters</listhead>

* <type>ArchiveEntry</type> <arg>entry</arg>: The entry to import data from

**Returns** <type>boolean</type>

Imports (copies) the data from <arg>entry</arg>. Returns `true` if the import succeeded.

If the import fails, the error that occurred should be available via <code>[slade.globalError](../Namespaces/App.md#globalerror)()</code>.

---
### `importFile`

<listhead>Parameters</listhead>

* <type>string</type> <arg>path</arg>: The full path to the file to import

**Returns** <type>boolean</type>

Imports the file at the given <arg>path</arg> into the entry. Returns `true` if the import succeeded.

If the import fails, the error that occurred should be available via <code>[slade.globalError](../Namespaces/App.md#globalerror)()</code>.
