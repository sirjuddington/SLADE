The **ArchiveEntry** type represents an entry in SLADE.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>name</prop> | <type>string</type> | The entry name.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt` this will be `Gun1.txt`
<prop>path</prop> | <type>string</type> | The path to the entry in the archive, beginning and ending with `/`.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt`, this will be `/Actors/Weapons/`. If the entry isn't in a directory this will be just `/`
<prop>type</prop> | <type>[EntryType](EntryType.md)</type> | The entry's type information
<prop>size</prop> | <type>number</type> | The size of the entry in bytes

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

## Functions

### `formattedName`

<params>Parameters</params>

* `[`<type>boolean</type> <arg>includePath</arg> : `true]`: Whether to include the entry path
* `[`<type>boolean</type> <arg>includeExtension</arg> : `true]`: Whether to include the entry extension
* `[`<type>boolean</type> <arg>upperCase</arg> : `false]`: Whether to convert the entry name to uppercase

**Returns** <type>string</type>

Returns a formatted name of the entry, depending on the parameters given. Note that <arg>upperCase</arg> will not affect the path.

### `formattedSize`

**Returns** <type>string</type>

Returns the size of the entry in a formatted string, eg. `1.3kb`
