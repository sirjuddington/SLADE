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

```lua
function Colour.new()
```

Creates a new colour with all components set to `0`.

---
```lua
function Colour.new(r, g, b)
```

Creates a new colour with the given <arg>r</arg>, <arg>g</arg> and <arg>b</arg> components. The <prop>a</prop> component is set to `255`.

<listhead>Parameters</listhead>

* <type>number</type> <arg>r</arg>: Red component (`0`-`255`)
* <type>number</type> <arg>g</arg>: Green component (`0`-`255`)
* <type>number</type> <arg>b</arg>: Blue component (`0`-`255`)

---
```lua
function Colour.new(r, g, b, a)
```

Creates a new colour with the given <arg>r</arg>, <arg>g</arg>, <arg>b</arg> and <arg>a</arg> components.

<listhead>Parameters</listhead>

* <type>number</type> <arg>r</arg>: Red component (`0`-`255`)
* <type>number</type> <arg>g</arg>: Green component (`0`-`255`)
* <type>number</type> <arg>b</arg>: Blue component (`0`-`255`)
* <type>number</type> <arg>a</arg>: Alpha component (`0`-`255`)
