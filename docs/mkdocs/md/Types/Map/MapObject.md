<article-head>MapObject</article-head>

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

### HasProperty

<fdef>function <type>MapObject</type>.<func>HasProperty</func>(<arg>*self*</arg>, <arg>name</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the property to check

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the object has a property matching the given <arg>name</arg>

---
### BoolProperty

<fdef>function <type>MapObject</type>.<func>BoolProperty</func>(<arg>*self*</arg>, <arg>name</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the property to get

<listhead>Returns</listhead>

* <type>boolean</type>: The value of the property, or `false` if no applicable value was found

#### Notes

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### IntProperty

<fdef>function <type>MapObject</type>.<func>IntProperty</func>(<arg>*self*</arg>, <arg>name</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the property to get

<listhead>Returns</listhead>

* <type>number</type>: The value of the property, or `0` if no applicable value was found

#### Notes

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### FloatProperty

<fdef>function <type>MapObject</type>.<func>FloatProperty</func>(<arg>*self*</arg>, <arg>name</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the property to get

<listhead>Returns</listhead>

* <type>number</type>: The value of the property, or `0` if no applicable value was found

#### Notes

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### StringProperty

<fdef>function <type>MapObject</type>.<func>StringProperty</func>(<arg>*self*</arg>, <arg>name</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the property to get

<listhead>Returns</listhead>

* <type>string</type>: The value of the property, or an empty string if no applicable value was found

#### Notes

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### SetBoolProperty

<fdef>function <type>MapObject</type>.<func>SetBoolProperty</func>(<arg>*self*</arg>, <arg>name</arg>, <arg>value</arg>)</fdef>

Sets the property <arg>name</arg> to <arg>value</arg>.

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the property to set
* <arg>value</arg> (<type>boolean</type>): The value to apply

#### Notes

The property is created if it doesn't already exist.

---
### SetIntProperty

<fdef>function <type>MapObject</type>.<func>SetIntProperty</func>(<arg>*self*</arg>, <arg>name</arg>, <arg>value</arg>)</fdef>

Sets the property <arg>name</arg> to <arg>value</arg>.

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the property to set
* <arg>value</arg> (<type>number</type>): The value to apply

#### Notes

The property is created if it doesn't already exist.

---
### SetFloatProperty

<fdef>function <type>MapObject</type>.<func>SetFloatProperty</func>(<arg>*self*</arg>, <arg>name</arg>, <arg>value</arg>)</fdef>

Sets the property <arg>name</arg> to <arg>value</arg>.

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the property to set
* <arg>value</arg> (<type>number</type>): The value to apply

#### Notes

The property is created if it doesn't already exist.

---
### SetStringProperty

<fdef>function <type>MapObject</type>.<func>SetStringProperty</func>(<arg>*self*</arg>, <arg>name</arg>, <arg>value</arg>)</fdef>

Sets the property <arg>name</arg> to <arg>value</arg>.

<listhead>Parameters</listhead>

* <arg>name</arg> (<type>string</type>): The name of the property to set
* <arg>value</arg> (<type>string</type>): The value to apply

#### Notes

The property is created if it doesn't already exist.
