<article-head>MapThing</article-head>

Represents a map thing (ie. an item/actor on the map).

### Inherits <type>[MapObject](MapObject.md)</type>  
All properties and functions of <type>[MapObject](MapObject.md)</type> can be used in addition to those below.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">x</prop> | <type>number</type> | The X coordinate of the thing
<prop class="ro">y</prop> | <type>number</type> | The Y coordinate of the thing
<prop class="ro">type</prop> | <type>number</type> | The thing type
<prop class="ro">angle</prop> | <type>number</type> | The direction the thing is facing in degrees, with `0` being east.

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

## Functions

### Flag

<fdef>function <type>MapThing</type>.<func>Flag</func>(<arg>*self*</arg>, <arg>flagName</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>flagName</arg> (<type>string</type>): The name of the flag to check

<listhead>Returns</listhead>

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

<fdef>function <type>MapThing</type>.<func>SetAnglePoint</func>(<arg>*self*</arg>, <arg>position</arg>)</fdef>

Sets the <prop>angle</prop> property so that the thing is facing towards <arg>position</arg>.

<listhead>Parameters</listhead>

* <arg>position</arg> (<type>[Point](../Point.md)</type>): Position in map coordinates
