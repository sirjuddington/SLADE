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
<prop class="ro">visibleTextures</prop> | <type>table</type> | The textures of the line that are visible.<br/>The <type>table</type> consists of the following <type>boolean</type> values: `frontUpper`, `frontMiddle`, `frontLower`, `backUpper`, `backMiddle`, `backLower`.

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

* `blocking`
* `twosided`
* `dontpegtop`
* `dontpegbottom`

---
### `flip`

<listhead>Parameters</listhead>

* `[`<type>boolean</type> <arg>swap_sides</arg> : `true]`: Whether to swap the sides

Flips the line so that it faces the opposite direction. If <arg>swap_sides</arg> is `true`, the line's <type>[MapSide](MapSide.md)</type>s will also be swapped so that they stay on the same spatial "side" of the line.

## Examples

### The <prop>visibleTextures</prop> Property

The <prop>visibleTextures</prop> property can be used to determine what textures (parts) of the line are showing. Below is an example that checks if a line's front upper texture is missing:

```lua
-- Check if front upper texture is visible
if line.visibleTextures.frontUpper == true then
    -- Check if texture is missing
    if line.side1.textureTop == '-' then
        App.logMessage('Line front upper texture is missing')
    end
end
```

!!! Tip
    The <prop>visibleTextures</prop> property is re-calculated each time it is accessed, so for performance reasons it is best to store it in a variable if it needs to be accessed multiple times.
