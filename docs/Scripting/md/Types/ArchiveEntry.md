The **ArchiveEntry** type represents an entry in SLADE.

## Properties

| Name | Type | Description |
|:-----|:-----|:------------|
`name` | `string` | The entry name.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt` this will be `Gun1.txt`
`path` | `string` | The path to the entry in the archive, beginning and ending with `/`.<br/>As an example, for an entry `Actors/Weapons/Gun1.txt`, this will be `/Actors/Weapons/`. If the entry isn't in a directory this will be just `/`
`type` | <code>[EntryType](EntryType.md)</code> | The entry's type information
`size` | `number` | The size of the entry in bytes

## Functions

### formattedName

**Parameters**

* `boolean` **includePath**: Whether to include the entry path_
* `boolean` **includeExtension**: Whether to include the entry extension_
* `boolean` **upperCase**: Whether to convert the entry name to uppercase_

**Returns** `string`

Returns a formatted name of the entry, depending on the parameters given. Note that `upperCase` will not affect the path.

### formattedSize

**Returns** `string`

Returns the size of the entry in a formatted string, eg. "1.3kb"
