<article-head>Colour</article-head>

Represents an RGBA colour. Note that colour each component is an 8-bit integer (ie. must be between `0` and `255`).

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">r</prop> | <type>number</type> | Red component
<prop class="rw">g</prop> | <type>number</type> | Green component
<prop class="rw">b</prop> | <type>number</type> | Blue component
<prop class="rw">a</prop> | <type>number</type> | Alpha component (transparency)

## Constructors

<fdef>function <type>Colour</type>.<func>new</func>()</fdef>

Creates a new colour with all components set to `0`.

---
<fdef>function <type>Colour</type>.<func>new</func>(<arg>r</arg>, <arg>g</arg>, <arg>b</arg>)</fdef>

Creates a new colour with the given <arg>r</arg>, <arg>g</arg> and <arg>b</arg> components. The <prop>a</prop> component is set to `255`.

<listhead>Parameters</listhead>

* <arg>r</arg> (<type>number</type>): Red component (`0` - `255`)
* <arg>g</arg> (<type>number</type>): Green component (`0` - `255`)
* <arg>b</arg> (<type>number</type>): Blue component (`0` - `255`)

---
<fdef>function <type>Colour</type>.<func>new</func>(<arg>r</arg>, <arg>g</arg>, <arg>b</arg>, <arg>a</arg>)</fdef>

Creates a new colour with the given <arg>r</arg>, <arg>g</arg>, <arg>b</arg> and <arg>a</arg> components.

<listhead>Parameters</listhead>

* <arg>r</arg> (<type>number</type>): Red component (`0` - `255`)
* <arg>g</arg> (<type>number</type>): Green component (`0` - `255`)
* <arg>b</arg> (<type>number</type>): Blue component (`0` - `255`)
* <arg>a</arg> (<type>number</type>): Alpha component (`0` - `255`)

## Functions

### AsHSL

<fdef>function <type>Colour</type>.<func>AsHSL</func>(<arg>*self*</arg>)</fdef>

Gets the colour in [HSL](https://en.wikipedia.org/wiki/HSL_and_HSV) colourspace.

<listhead>Returns</listhead>

* <type>number</type>: The hue value (`0.0` - `1.0`)
* <type>number</type>: The saturation value (`0.0` - `1.0`)
* <type>number</type>: The lightness value (`0.0` - `1.0`)

---
### AsLAB

<fdef>function <type>Colour</type>.<func>AsLAB</func>(<arg>*self*</arg>)</fdef>

Gets the colour in [CIELAB](https://en.wikipedia.org/wiki/CIELAB_color_space) colourspace.

<listhead>Returns</listhead>

* <type>number</type>: The lightness value (`0.0` - `1.0`)
* <type>number</type>: The green-red value (`0.0` - `1.0`)
* <type>number</type>: The blue-yellow value (`0.0` - `1.0`)

---
### FromHSL

<fdef>function <type>Colour</type>.<func>FromHSL</func>(<arg>*self*</arg>, <arg>hue</arg>, <arg>saturation</arg>, <arg>lightness</arg>)</fdef>

Sets the colour from the given HSL colourspace values.

<listhead>Parameters</listhead>

* <arg>hue</arg> (<type>number</type>): The hue value to set
* <arg>saturation</arg> (<type>number</type>): The saturation value to set
* <arg>lightness</arg> (<type>number</type>): The lightness value to set
