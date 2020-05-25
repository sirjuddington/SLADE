<subhead>Type</subhead>
<header>MapThing</header>

Represents a map thing (ie. an item/actor on the map).

### Inherits <type>[MapObject](MapObject.md)</type>  
All properties and functions of <type>[MapObject](MapObject.md)</type> can be used in addition to those below.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">x</prop> | <type>float</type> | The X coordinate of the thing
<prop class="ro">y</prop> | <type>float</type> | The Y coordinate of the thing
<prop class="ro">type</prop> | <type>integer</type> | The thing type
<prop class="ro">angle</prop> | <type>integer</type> | The direction the thing is facing in degrees, with `0` being east.

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Map.things](Map.md#properties)</code>

## Functions

### Overview

#### Info

<fdef>[Flag](#flag)(<arg>flagName</arg>) -> <type>boolean</type></fdef>

#### Modification

<fdef>[SetAnglePoint](#setanglepoint)(<arg>position</arg>)</fdef>

---
### Flag

#### Parameters

* <arg>flagName</arg> (<type>string</type>): The name of the flag to check

#### Returns

* <type>boolean</type>: `true` if the given flag is set

**Notes**

If the parent <type>[Map](Map.md)</type>'s format is UDMF, this behaves exactly the same as <code>[MapObject.BoolProperty](MapObject.md#boolproperty)</code>. Otherwise, <arg>flagName</arg> needs to be one of the following:

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
### SetAnglePoint

Sets the <prop>angle</prop> property so that the thing is facing towards <arg>position</arg>.

#### Parameters

* <arg>position</arg> (<type>[Point](../Point.md)</type>): Position in map coordinates
