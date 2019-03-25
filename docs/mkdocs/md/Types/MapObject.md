<article-head>MapObject</article-head>

The **MapObject** type is a base type for all map editor objects.

!!! note "Regarding MapObject `*Property` and `Set*Property` functions"
    MapObject properties in SLADE generally mirror the properties defined in the [UDMF](https://doomwiki.org/wiki/UDMF) specification. As an example, setting the `texturetop` string property on a <type>[MapSide](MapSide.md)</type> MapObject will set its upper texture. Note that not all basic UDMF properties are supported for non-UDMF maps.

### Derived Types

The following types inherit all MapObject properties and functions:

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

### HasProperty

```lua
function MapObject.HasProperty(self, name)
```

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the property to check

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the object has a property matching the given <arg>name</arg>

---
### BoolProperty

```lua
function MapObject.BoolProperty(self, name)
```

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the property to get

<listhead>Returns</listhead>

* <type>boolean</type>: The value of the property, or `false` if no applicable value was found

**Notes**

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### IntProperty

```lua
function MapObject.IntProperty(self, name)
```

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the property to get

<listhead>Returns</listhead>

* <type>number</type>: The value of the property, or `0` if no applicable value was found

**Notes**

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### FloatProperty

```lua
function MapObject.FloatProperty(self, name)
```

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the property to get

<listhead>Returns</listhead>

* <type>number</type>: The value of the property, or `0` if no applicable value was found

**Notes**

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### StringProperty

```lua
function MapObject.StringProperty(self, name)
```

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the property to get

<listhead>Returns</listhead>

* <type>string</type>: The value of the property, or an empty string if no applicable value was found

**Notes**

If the property doesn't exist in the object, the game configuration is checked for a default value.

---
### SetBoolProperty (<arg>name</arg>, <arg>value</arg>)

```lua
function MapObject.SetBoolProperty(self, name, value)
```

Sets the property <arg>name</arg> to <arg>value</arg>.

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the property to set
* <type>boolean</type> <arg>value</arg>: The value to apply

**Notes**

The property is created if it doesn't already exist.

---
### SetIntProperty

```lua
function MapObject.SetIntProperty(self, name, value)
```

Sets the property <arg>name</arg> to <arg>value</arg>.

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the property to set
* <type>number</type> <arg>value</arg>: The value to apply

**Notes**

The property is created if it doesn't already exist.

---
### SetFloatProperty

```lua
function MapObject.SetFloatProperty(self, name, value)
```

Sets the property <arg>name</arg> to <arg>value</arg>.

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the property to set
* <type>number</type> <arg>value</arg>: The value to apply

**Notes**

The property is created if it doesn't already exist.

---
### SetStringProperty

```lua
function MapObject.SetStringProperty(self, name, value)
```

Sets the property <arg>name</arg> to <arg>value</arg>.

<listhead>Parameters</listhead>

* <type>string</type> <arg>name</arg>: The name of the property to set
* <type>string</type> <arg>value</arg>: The value to apply

**Notes**

The property is created if it doesn't already exist.
