Represents a map sector.

### Inherits <type>[MapObject](MapObject.md)</type>  
All properties and functions of <type>[MapObject](MapObject.md)</type> can be used in addition to those below.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>textureFloor</prop> | <type>string</type> | The floor texture of the sector
<prop>textureCeiling</prop> | <type>string</type> | The ceiling texture of the sector
<prop>heightFloor</prop> | <type>number</type> | The height of the sector's floor
<prop>heightCeiling</prop> | <type>number</type> | The height of the sector's ceiling
<prop>lightLevel</prop> | <type>number</type> | The light level of the sector
<prop>special</prop> | <type>number</type> | The sector's special
<prop>id</prop> | <type>number</type> | The sector's id (tag)
<prop>connectedSides</prop> | <type>[MapSide](MapSide.md)\[\]</type> | An array of all sides that make up this sector
<prop>colour</prop> | <type>[Colour](Colour.md)</type> | The light colour of the sector
<prop>fogColour</prop> | <type>[Colour](Colour.md)</type> | The fog colour of the sector
<prop>planeFloor</prop> | <type>[Plane](Plane.md)</type> | The floor plane of the sector
<prop>planeCeiling</prop> | <type>[Plane](Plane.md)</type> | The ceiling plane of the sector

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

## Functions

### `containsPoint`

<listhead>Parameters</listhead>

* <type>[Point](Point.md)</type> <arg>position</arg>: Point coordinates in map units

**Returns** <type>boolean</type>

Returns `true` if <arg>position</arg> is inside the sector.
