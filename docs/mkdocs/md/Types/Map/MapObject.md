The <type>MapObject</type> type is a base type for all map editor objects.

### Derived Types

The following types inherit all `MapObject` properties and functions:

* <type>[MapVertex](MapVertex.md)</type>
* <type>[MapLine](MapLine.md)</type>
* <type>[MapSide](MapSide.md)</type>
* <type>[MapSector](MapSector.md)</type>
* <type>[MapThing](MapThing.md)</type>

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">index</prop>     | <type>number</type> | The object's index in the map
<prop class="ro">type</prop>      | <type>number</type> | The object's type (see `TYPE_` constants)
<prop class="ro">typeName</prop>  | <type>string</type> | The object type name (eg. `Vertex`)

## Constants

| Name | Value |
|:-----|:------|
`TYPE_OBJECT` | 0
`TYPE_VERTEX` | 1
`TYPE_LINE` | 2
`TYPE_SIDE` | 3
`TYPE_SECTOR` | 4
`TYPE_THING` | 5

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

## Functions

!!! note "Regarding MapObject `*Property` and `Set*Property` functions"
    MapObject properties in SLADE generally mirror the properties defined in the [UDMF](https://doomwiki.org/wiki/UDMF) specification. As an example, setting the `texturetop` string property on a <type>[MapSide](MapSide.md)</type> MapObject will set its upper texture. Note that not all basic UDMF properties are supported for non-UDMF maps.

### Overview

#### Properties

<fdef>[HasProperty](#hasproperty)(<arg>name</arg>) -> <type>boolean</type></fdef>
<fdef>[BoolProperty](#boolproperty)(<arg>name</arg>) -> <type>boolean</type></fdef>
<fdef>[IntProperty](#intproperty)(<arg>name</arg>) -> <type>number</type></fdef>
<fdef>[FloatProperty](#floatproperty)(<arg>name</arg>) -> <type>number</type></fdef>
<fdef>[StringProperty](#stringproperty)(<arg>name</arg>) -> <type>string</type></fdef>
<fdef>[SetBoolProperty](#setboolproperty)(<arg>name</arg>, <arg>value</arg>)</fdef>
<fdef>[SetIntProperty](#setintproperty)(<arg>name</arg>, <arg>value</arg>)</fdef>
<fdef>[SetFloatProperty](#setfloatproperty)(<arg>name</arg>, <arg>value</arg>)</fdef>
<fdef>[SetStringProperty](#setstringproperty)(<arg>name</arg>, <arg>value</arg>)</fdef>

---
### HasProperty

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the property to check

#### Returns

* <type>boolean</type>: `true` if the object has a property matching the given <arg>name</arg>

---
### BoolProperty

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the property to get

#### Returns

* <type>boolean</type>: The value of the property, or `false` if no applicable value was found

#### Notes

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### IntProperty

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the property to get

#### Returns

* <type>number</type>: The value of the property, or `0` if no applicable value was found

#### Notes

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### FloatProperty

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the property to get

#### Returns

* <type>number</type>: The value of the property, or `0` if no applicable value was found

#### Notes

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### StringProperty

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the property to get

#### Returns

* <type>string</type>: The value of the property, or an empty string if no applicable value was found

#### Notes

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### SetBoolProperty

Sets the property <arg>name</arg> to <arg>value</arg>.

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the property to set
* <arg>value</arg> (<type>boolean</type>): The value to apply

#### Notes

The property is added if it doesn't already exist.

---
### SetIntProperty

Sets the property <arg>name</arg> to <arg>value</arg>.

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the property to set
* <arg>value</arg> (<type>number</type>): The value to apply

#### Notes

The property is added if it doesn't already exist.

---
### SetFloatProperty

Sets the property <arg>name</arg> to <arg>value</arg>.

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the property to set
* <arg>value</arg> (<type>number</type>): The value to apply

#### Notes

The property is added if it doesn't already exist.

---
### SetStringProperty

Sets the property <arg>name</arg> to <arg>value</arg>.

#### Parameters

* <arg>name</arg> (<type>string</type>): The name of the property to set
* <arg>value</arg> (<type>string</type>): The value to apply

#### Notes

The property is added if it doesn't already exist.
