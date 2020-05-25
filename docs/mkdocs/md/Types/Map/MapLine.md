Represents a map linedef.

### Inherits <type>[MapObject](MapObject.md)</type>  
All properties and functions of <type>[MapObject](MapObject.md)</type> can be used in addition to those below.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">x1</prop> | <type>number</type> | The X coordinate of the line's first vertex
<prop class="ro">y1</prop> | <type>number</type> | The Y coordinate of the line's first vertex
<prop class="ro">x2</prop> | <type>number</type> | The X coordinate of the line's second vertex
<prop class="ro">y2</prop> | <type>number</type> | The Y coordinate of the line's second vertex
<prop class="ro">vertex1</prop> | <type>[MapVertex](MapVertex.md)</type> | The line's first vertex
<prop class="ro">vertex2</prop> | <type>[MapVertex](MapVertex.md)</type> | The line's second vertex
<prop class="ro">side1</prop> | <type>[MapSide](MapSide.md)</type> | The line's first (front) side
<prop class="ro">side2</prop> | <type>[MapSide](MapSide.md)</type> | The line's second (back) side
<prop class="ro">special</prop> | <type>number</type> | The line's action special
<prop class="ro">length</prop> | <type>number</type> | The length of the line in map units

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Map.linedefs](Map.md#properties)</code>

## Functions

### Overview

#### Info

<fdef>[Flag](#flag)</func>(<arg>flagName</arg>) -> <type>boolean</type></fdef>
<fdef>[VisibleTextures](#visibletextures)</func>() -> <type>table</type></fdef>

#### Modification

<fdef>[Flip](#flip)</func>(<arg>[swapSides]</arg>)</fdef>

---
### Flag

#### Parameters

* <arg>flagName</arg> (<type>string</type>): The name of the flag to check

#### Returns

* <type>boolean</type>: `true` if the given flag is set

#### Notes

If the parent <type>[Map](Map.md)</type>'s format is UDMF, this behaves exactly the same as <code>[MapObject.BoolProperty](MapObject.md#boolproperty)</code>. Otherwise, <arg>flagName</arg> needs to be one of the following:

* `blocking`
* `twosided`
* `dontpegtop`
* `dontpegbottom`

---
### VisibleTextures

Determines what textures (parts) of the line are showing.

#### Returns

* <type>table</type>: A table containing the following <type>boolean</type> values:
    * `frontUpper`
    * `frontMiddle`
    * `frontLower`
    * `backUpper`
    * `backMiddle`
    * `backLower`

#### Example

```lua
local visible = line:VisibleTextures()

-- Check if front upper texture is visible
if visible.frontUpper == true then
    -- Check if texture is missing
    if line.side1.textureTop == '-' then
        App.LogMessage('Line front upper texture is missing')
    end
end
```

---
### Flip

Flips the line so that it faces the opposite direction. If <arg>swapSides</arg> is `true`, <prop>side1</prop> and <prop>side2</prop> will also be swapped so that they stay on the same spatial "side" of the line.

#### Parameters

* <arg>[swapSides]</arg> (<type>boolean</type>): Whether to swap the sides. Default is `true`
