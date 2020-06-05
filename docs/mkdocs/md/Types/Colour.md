<subhead>Type</subhead>
<header>Colour</header>

Represents an RGBA colour. Note that colour each component is an 8-bit integer (ie. must be between `0` and `255`).

## Constants

| Name | Value |
|:-----|:------|
`FORMAT_RGB` | 0
`FORMAT_RGBA` | 1
`FORMAT_HEX` | 2
`FORMAT_ZDOOM` | 3

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">r</prop> | <type>integer</type> | Red component
<prop class="rw">g</prop> | <type>integer</type> | Green component
<prop class="rw">b</prop> | <type>integer</type> | Blue component
<prop class="rw">a</prop> | <type>integer</type> | Alpha component (transparency)
<prop class="ro">fr</prop> | <type>float</type> | Red component as a float (`0.0`-`1.0`)
<prop class="ro">fg</prop> | <type>float</type> | Green component as a float (`0.0`-`1.0`)
<prop class="ro">fb</prop> | <type>float</type> | Blue component as a float (`0.0`-`1.0`)
<prop class="ro">fa</prop> | <type>float</type> | Alpha component as a float (`0.0`-`1.0`)

## Constructors

<code><type>Colour</type>.<func>new</func>()</code>

Creates a new colour with all components set to `0`.

---
<code><type>Colour</type>.<func>new</func>(<arg>r</arg>, <arg>g</arg>, <arg>b</arg>)</code>

Creates a new colour with the given <arg>r</arg>, <arg>g</arg> and <arg>b</arg> components. The <prop>a</prop> component is set to `255`.

#### Parameters

* <arg>r</arg> (<type>integer</type>): Red component (`0` - `255`)
* <arg>g</arg> (<type>integer</type>): Green component (`0` - `255`)
* <arg>b</arg> (<type>integer</type>): Blue component (`0` - `255`)

---
<code><type>Colour</type>.<func>new</func>(<arg>r</arg>, <arg>g</arg>, <arg>b</arg>, <arg>a</arg>)</code>

Creates a new colour with the given <arg>r</arg>, <arg>g</arg>, <arg>b</arg> and <arg>a</arg> components.

#### Parameters

* <arg>r</arg> (<type>integer</type>): Red component (`0` - `255`)
* <arg>g</arg> (<type>integer</type>): Green component (`0` - `255`)
* <arg>b</arg> (<type>integer</type>): Blue component (`0` - `255`)
* <arg>a</arg> (<type>integer</type>): Alpha component (`0` - `255`)

## Functions

### Overview

#### Conversion

<fdef>[AsHSL](#ashsl)() -> <type>float</type>, <type>float</type>, <type>float</type></fdef>
<fdef>[AsLAB](#aslab)() -> <type>float</type>, <type>float</type>, <type>float</type></fdef>
<fdef>[AsString](#aslab)(<arg>format</arg>) -> <type>string</type></fdef>
<fdef>[FromHSL](#fromhsl)(<arg>hue</arg>, <arg>saturation</arg>, <arg>lightness</arg>)</fdef>

---
### AsHSL

Gets the colour in [HSL](https://en.wikipedia.org/wiki/HSL_and_HSV) colourspace.

#### Returns

* <type>float</type>: The hue value (`0.0` - `1.0`)
* <type>float</type>: The saturation value (`0.0` - `1.0`)
* <type>float</type>: The lightness value (`0.0` - `1.0`)

---
### AsLAB

Gets the colour in [CIELAB](https://en.wikipedia.org/wiki/CIELAB_color_space) colourspace.

#### Returns

* <type>float</type>: The lightness value (`0.0` - `1.0`)
* <type>float</type>: The green-red value (`0.0` - `1.0`)
* <type>float</type>: The blue-yellow value (`0.0` - `1.0`)

---
### AsString

Gets a string representation of the colour in the specified <arg>format</arg>.

#### Parameters

* <arg>format</arg> (<type>integer</type>): The format of the string representation. See `FORMAT_` constants above

#### Returns

* <type>string</type>: The colour as a string of the specified format

#### Example

```lua
local colour = Colour.new(100, 150, 200, 150)

App.LogMessage(colour:AsString(Colour.FORMAT_RGB))   -- rgb(100, 150, 200)
App.LogMessage(colour:AsString(Colour.FORMAT_RGBA))  -- rgba(100, 150, 200, 150)
App.LogMessage(colour:AsString(Colour.FORMAT_HEX))   -- #6496C8
App.LogMessage(colour:AsString(Colour.FORMAT_ZDOOM)) -- "64 96 C8"
```

---
### FromHSL

Sets the colour from the given HSL colourspace values.

#### Parameters

* <arg>hue</arg> (<type>float</type>): The hue value to set (`0.0` - `1.0`)
* <arg>saturation</arg> (<type>float</type>): The saturation value to set (`0.0` - `1.0`)
* <arg>lightness</arg> (<type>float</type>): The lightness value to set (`0.0` - `1.0`)
