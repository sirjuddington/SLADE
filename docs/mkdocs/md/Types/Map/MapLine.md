<article-head>MapLine</article-head>

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

## Functions

### Flag

<fdef>function <type>MapLine</type>.<func>Flag</func>(<arg>*self*</arg>, <arg>flagName</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>flagName</arg> (<type>string</type>): The name of the flag to check

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the given flag is set

#### Notes

If the parent <type>[Map](Map.md)</type>'s format is UDMF, this behaves exactly the same as <code>[MapObject.BoolProperty](MapObject.md#boolproperty)</code>. Otherwise, <arg>flagName</arg> needs to be one of the following:

* `blocking`
* `twosided`
* `dontpegtop`
* `dontpegbottom`

---
### Flip

<fdef>function <type>MapLine</type>.<func>Flip</func>(<arg>*self*</arg>, <arg>swapSides</arg>)</fdef>

Flips the line so that it faces the opposite direction. If <arg>swapSides</arg> is `true`, <prop>side1</prop> and <prop>side2</prop> will also be swapped so that they stay on the same spatial "side" of the line.

<listhead>Parameters</listhead>

* <arg>[swapSides]</arg> (<type>boolean</type>, default `true`): Whether to swap the sides

---
### VisibleTextures

<fdef>function <type>MapLine</type>.<func>VisibleTextures</func>(<arg>*self*</arg>)</fdef>

Determines what textures (parts) of the line are showing.

<listhead>Returns</listhead>

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
