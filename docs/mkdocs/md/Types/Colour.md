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
