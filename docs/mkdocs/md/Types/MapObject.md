The **MapObject** type is a base type for all map editor objects.

**Regarding MapObject `*Property` and `set*Property` functions**  
MapObject properties in SLADE generally mirror the properties defined in the [UDMF](https://doomwiki.org/wiki/UDMF) specification. As an example, setting the `texturetop` string property on a <type>[MapSide](MapSide.md)</type> MapObject will set its upper texture.

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
<prop>index</prop>     | <type>number</type> | The object's index in the map
<prop>typeName</prop>  | <type>string</type> | The object type name (eg. `Vertex`)

## Functions

### hasProperty

**Parameters**

* <type>string</type> <arg>key</arg>: The property key to check

**Returns** <type>boolean</type>

Returns `true` if the object has a property matching the given <arg>key</arg>

---
### boolProperty

**Parameters**

* <type>string</type> <arg>key</arg>: The property key

**Returns** <type>boolean</type>

Returns the <type>boolean</type> value of the property matching the given <arg>key</arg>. If the property doesn't exist, the game configuration is checked for a default value. Otherwise, returns `false`.

---
### intProperty

**Parameters**

* <type>string</type> <arg>key</arg>: The property key

**Returns** <type>number</type>

Returns the integer <type>number</type> value of the property matching the given <arg>key</arg>. If the property doesn't exist, the game configuration is checked for a default value. Otherwise, returns 0.

---
### floatProperty

**Parameters**

* <type>string</type> <arg>key</arg>: The property key

**Returns** <type>number</type>

Returns the floating point <type>number</type> value of the property matching the given <arg>key</arg>. If the property doesn't exist, the game configuration is checked for a default value. Otherwise, returns 0.

---
### stringProperty

**Parameters**

* <type>string</type> <arg>key</arg>: The property key

**Returns** <type>string</type>

Returns the <type>string</type> value of the property matching the given <arg>key</arg>. If the property doesn't exist, the game configuration is checked for a default value. Otherwise, returns an empty string.

---
### setBoolProperty

**Parameters**

* <type>string</type> <arg>key</arg>: The key of the property to set
* <type>boolean</type> <arg>value</arg>: The value to apply

Sets the property <arg>key</arg> to <arg>value</arg>. The property is created if it doesn't already exist.

---
### setIntProperty

**Parameters**

* <type>string</type> <arg>key</arg>: The key of the property to set
* <type>number</type> <arg>value</arg>: The value to apply

Sets the property <arg>key</arg> to <arg>value</arg>. The property is created if it doesn't already exist.

---
### setFloatProperty

**Parameters**

* <type>string</type> <arg>key</arg>: The key of the property to set
* <type>number</type> <arg>value</arg>: The value to apply

Sets the property <arg>key</arg> to <arg>value</arg>. The property is created if it doesn't already exist.

---
### setStringProperty

**Parameters**

* <type>string</type> <arg>key</arg>: The key of the property to set
* <type>string</type> <arg>value</arg>: The value to apply

Sets the property <arg>key</arg> to <arg>value</arg>. The property is created if it doesn't already exist.
