<subhead>Type</subhead>
<header>Colour</header>

Represents an RGBA colour. Note that colour each component is an 8-bit integer (ie. must be between `0` and `255`).

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">r</prop> | <type>integer</type> | Red component
<prop class="rw">g</prop> | <type>integer</type> | Green component
<prop class="rw">b</prop> | <type>integer</type> | Blue component
<prop class="rw">a</prop> | <type>integer</type> | Alpha component (transparency)

## Constructors

<code><type>Colour</type>.<func>new</func>()</code>

Creates a new colour with all components set to `0`.

---
<code><type>Colour</type>.<func>new</func>(<arg>r</arg>, <arg>g</arg>, <arg>b</arg>)</code>

Creates a new colour with the given <arg>r</arg>, <arg>g</arg> and <arg>b</arg> components. The <prop>a</prop> component is set to `255`.

<listhead>Parameters</listhead>

* <arg>r</arg> (<type>integer</type>): Red component (`0` - `255`)
* <arg>g</arg> (<type>integer</type>): Green component (`0` - `255`)
* <arg>b</arg> (<type>integer</type>): Blue component (`0` - `255`)

---
<code><type>Colour</type>.<func>new</func>(<arg>r</arg>, <arg>g</arg>, <arg>b</arg>, <arg>a</arg>)</code>

Creates a new colour with the given <arg>r</arg>, <arg>g</arg>, <arg>b</arg> and <arg>a</arg> components.

<listhead>Parameters</listhead>

* <arg>r</arg> (<type>integer</type>): Red component (`0` - `255`)
* <arg>g</arg> (<type>integer</type>): Green component (`0` - `255`)
* <arg>b</arg> (<type>integer</type>): Blue component (`0` - `255`)
* <arg>a</arg> (<type>integer</type>): Alpha component (`0` - `255`)

## Functions

### Overview

#### Conversion

<fdef>[AsHSL](#ashsl)() -> <type>float</type>, <type>float</type>, <type>float</type></fdef>
<fdef>[AsLAB](#aslab)() -> <type>float</type>, <type>float</type>, <type>float</type></fdef>
<fdef>[FromHSL](#fromhsl)(<arg>hue</arg>, <arg>saturation</arg>, <arg>lightness</arg>)</fdef>

---
### AsHSL

Gets the colour in [HSL](https://en.wikipedia.org/wiki/HSL_and_HSV) colourspace.

<listhead>Returns</listhead>

* <type>float</type>: The hue value (`0.0` - `1.0`)
* <type>float</type>: The saturation value (`0.0` - `1.0`)
* <type>float</type>: The lightness value (`0.0` - `1.0`)

---
### AsLAB

Gets the colour in [CIELAB](https://en.wikipedia.org/wiki/CIELAB_color_space) colourspace.

<listhead>Returns</listhead>

* <type>float</type>: The lightness value (`0.0` - `1.0`)
* <type>float</type>: The green-red value (`0.0` - `1.0`)
* <type>float</type>: The blue-yellow value (`0.0` - `1.0`)

---
### FromHSL

Sets the colour from the given HSL colourspace values.

<listhead>Parameters</listhead>

* <arg>hue</arg> (<type>float</type>): The hue value to set (`0.0` - `1.0`)
* <arg>saturation</arg> (<type>float</type>): The saturation value to set (`0.0` - `1.0`)
* <arg>lightness</arg> (<type>float</type>): The lightness value to set (`0.0` - `1.0`)
