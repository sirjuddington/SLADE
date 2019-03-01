The **EntryType** type contains information about an entry's type/format.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">id</prop>        | <type>string</type> | The type id
<prop class="ro">name</prop>      | <type>string</type> | The type name
<prop class="ro">extension</prop> | <type>string</type> | The file extension used when saving entries of this type to a file
<prop class="ro">formatId</prop>  | <type>string</type> | The type's data format id
<prop class="ro">editor</prop>    | <type>string</type> | The id of the editor used to open entries of this type
<prop class="ro">category</prop>  | <type>string</type> | The type category, used in the entry list filter

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts. Retrieve one by using the `fromId` function below.

## Functions

### `fromId` (static)

<listhead>Parameters</listhead>

* <type>string</type> <arg>id</arg>: The id of the `EntryType` to get

**Returns** <type>EntryType</type>

Returns the `EntryType` with the given <arg>id</arg>, or `nil` if no type has that id.

*Note that this is a 'static' function, and does not need to be called from an `EntryType` object (see the example below)*

**Example**

```lua
-- Will write 'Wad Archive' to the log
local type = EntryType.fromId('wad')
App.logMessage(type.name)
```
