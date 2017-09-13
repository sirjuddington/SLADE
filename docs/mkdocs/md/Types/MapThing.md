Represents a map thing (ie. an item/actor on the map).

### Inherits <type>[MapObject](MapObject.md)</type>  
All properties and functions of <type>[MapObject](MapObject.md)</type> can be used in addition to those below.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">x</prop> | <type>number</type> | The X coordinate of the thing
<prop class="ro">y</prop> | <type>number</type> | The Y coordinate of the thing
<prop class="ro">type</prop> | <type>number</type> | The thing type
<prop class="ro">angle</prop> | <type>number</type> | The direction the thing is facing in degrees, with 0 being east.

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

## Functions

### `flag`

<listhead>Parameters</listhead>

* <type>string</type> <arg>flag_name</arg>: The name of the flag to check

**Returns** <type>boolean</type>

Returns `true` if the given flag is set.

If the parent <type>[Map](Map.md)</type>'s format is UDMF, this behaves exactly the same as <code>[MapObject:boolProperty](MapObject.md#boolproperty)</code>. Otherwise, <arg>flag_name</arg> needs to be one of the following:

* `skill1`
* `skill2`
* `skill3`
* `skill4`
* `skill5`
* `single`
* `coop`
* `dm`
* `class1`
* `class2`
* `class3`

---
### `setAnglePoint`

<listhead>Parameters</listhead>

* <type>[Point](Point.md)</type> <arg>position</arg>: Position in map coordinates

Sets the <prop>angle</prop> property so that the thing is facing towards <arg>position</arg>.
