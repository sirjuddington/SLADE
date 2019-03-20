The `EntryType` type contains information about an entry's type/format.

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
    This type can not be created directly in scripts. Use [Archives.EntryType](../Namespaces/Archives.md#entrytype) to retrieve one.
