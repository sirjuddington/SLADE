<article-head>Palette</article-head>

A palette containing (up to) 256 <type>[Colour](../Colour.md)</type>s

## Constants

| Name | Value |
|:-----|:------|
`FORMAT_RAW` | 0
`FORMAT_IMAGE` | 1
`FORMAT_CSV` | 2
`FORMAT_JASC` | 3
`FORMAT_GIMP` | 4
`MATCH_DEFAULT` | 0
`MATCH_OLD` | 1
`MATCH_RGB` | 2
`MATCH_HSL` | 3
`MATCH_C76` | 4
`MATCH_C94` | 5
`MATCH_C2K` | 6


## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">colourCount</prop> | <type>number</type> | The number of colours in the palette


## Constructors

<fdef>function <type>Palette</type>.<func>new</func>()</fdef>

Creates a new palette with 256 colours in a greyscale gradient (from black to white).

---

<fdef>function <type>Palette</type>.<func>new</func>(<arg>count</arg>)</fdef>

Creates a new palette with <arg>count</arg> colours in a greyscale gradient.

<listhead>Parameters</listhead>

* <arg>count</arg> (<type>number</type>): The number of colours in the palette


## Functions - General

### Colour

<fdef>function <type>Palette</type>.<func>Colour</func>(<arg>*self*</arg>, <arg>index</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour to get

<listhead>Returns</listhead>

* <type>[Colour](../Colour.md)</type>: The colour at the given <arg>index</arg>

---
### CopyColours

<fdef>function <type>Palette</type>.<func>CopyColours</func>(<arg>*self*</arg>, <arg>other</arg>)</fdef>

Copies all colours from another palette.

<listhead>Parameters</listhead>

* <arg>other</arg> (<type>Palette</type>): The palette to copy from

**Notes**

This will not change the <prop>colourCount</prop> of the palette. If the <arg>other</arg> palette has less colours than this, some colours in this palette will be left the same.

---
### CountUniqueColours

<fdef>function <type>Palette</type>.<func>CountUniqueColours</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>number</type>: The number of unique colours present in the palette

---
### FindColour

<fdef>function <type>Palette</type>.<func>FindColour</func>(<arg>*self*</arg>, <arg>colour</arg>)</fdef>

Finds the index of the first colour in the palette that *exactly* matches the given <arg>colour</arg>.

<listhead>Parameters</listhead>

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to find in the palette

<listhead>Returns</listhead>

* <type>number</type>: The (`0`-based) index of the first matching colour in the palette, or `-1` if no match was found

---
### NearestColour

<fdef>function <type>Palette</type>.<func>NearestColour</func>(<arg>*self*</arg>, <arg>colour</arg>, <arg>matchMode</arg>)</fdef>

Finds the index of the colour in the palette that most closely matches the given <arg>colour</arg>.

<listhead>Parameters</listhead>

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to find in the palette
* <arg>matchMode</arg> (<type>number</type>, default `MATCH_DEFAULT`): The colour matching algorithm to use (see `MATCH_` constants). If this is `MATCH_DEFAULT`, the colour matching algorithm currently selected in the SLADE preferences will be used

<listhead>Returns</listhead>

* <type>number</type>: The (`0`-based) index of the closest matching colour


## Functions - Load/Save

### LoadData

<fdef>function <typr>Palette</typr>.<func>LoadData</func>(<arg>*self*</arg>, <arg>data</arg>, <arg>format</arg>)</fdef>

Loads the given <arg>data</arg> into the palette.

<listhead>Parameters</listhead>

* <arg>data</arg> (<type>string</type>): The binary data to load
* <arg>[format]</arg> (<type>number</type>, default `FORMAT_RAW`): The format of the data. See `FORMAT_` constants

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### LoadFile

<fdef>function <typr>Palette</typr>.<func>LoadFile</func>(<arg>*self*</arg>, <arg>path</arg>, <arg>format</arg>)</fdef>

Loads the file at <arg>path</arg> on disk into the palette.

<listhead>Parameters</listhead>

* <arg>path</arg> (<type>string</type>): The full path to the file to load
* <arg>[format]</arg> (<type>number</type>, default `FORMAT_RAW`): The format of the file. See `FORMAT_` constants

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### SaveFile

<fdef>function <typr>Palette</typr>.<func>SaveFile</func>(<arg>*self*</arg>, <arg>path</arg>, <arg>format</arg>)</fdef>

Saves the palette to a file at <arg>path</arg> on disk in the specified <arg>format</arg>.

<listhead>Parameters</listhead>

* <arg>path</arg> (<type>string</type>): The full path to the file
* <arg>[format]</arg> (<type>number</type>, default `FORMAT_RAW`): The format of the file. See `FORMAT_` constants

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success


## Functions - Colour Modification

### SetColour

<fdef>function <type>Palette</type>.<func>SetColour</func>(<arg>*self*</arg>, <arg>index</arg>, <arg>colour</arg>)</fdef>

Sets the colour at <arg>index</arg> in the palette.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour to set
* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to set it to

---
### SetColourR

<fdef>function <type>Palette</type>.<func>SetColourR</func>(<arg>*self*</arg>, <arg>index</arg>, <arg>r</arg>)</fdef>

Sets red component of the colour at <arg>index</arg> in the palette.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour in the palette
* <arg>r</arg> (<type>number</type>): The new red component value

---
### SetColourG

<fdef>function <type>Palette</type>.<func>SetColourG</func>(<arg>*self*</arg>, <arg>index</arg>, <arg>g</arg>)</fdef>

Sets green component of the colour at <arg>index</arg> in the palette.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour in the palette
* <arg>g</arg> (<type>number</type>): The new green component value

---
### SetColourB

<fdef>function <type>Palette</type>.<func>SetColourB</func>(<arg>*self*</arg>, <arg>index</arg>, <arg>b</arg>)</fdef>

Sets blue component of the colour at <arg>index</arg> in the palette.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour in the palette
* <arg>b</arg> (<type>number</type>): The new blue component value

---
### SetColourA

<fdef>function <type>Palette</type>.<func>SetColourA</func>(<arg>*self*</arg>, <arg>index</arg>, <arg>a</arg>)</fdef>

Sets alpha component of the colour at <arg>index</arg> in the palette.

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour in the palette
* <arg>a</arg> (<type>number</type>): The new green component value


## Functions - Colour Range Modification

### ApplyTranslation

<fdef>function <type>Palette</type>.<func>ApplyTranslation</func>(<arg>*self*</arg>, <arg>translation</arg>)</fdef>

Applies a <type>[Translation](../Translation/Translation.md)</type> to the palette.

<listhead>Parameters</listhead>

* <arg>translation</arg> (<type>[Translation](../Translation/Translation.md)</type>): The translation to apply

---
### Colourise

<fdef>function <type>Palette</type>.<func>Colourise</func>(<arg>*self*</arg>, <arg>colour</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>

Colourises a range of colours in the palette.

<listhead>Parameters</listhead>

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to use
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Tint

<fdef>function <type>Palette</type>.<func>Tint</func>(<arg>*self*</arg>, <arg>colour</arg>, <arg>amount</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>

Tints a range of colours in the palette.

<listhead>Parameters</listhead>

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to use
* <arg>amount</arg> (<type>number</type>): The amount to tint (`0.0` - `1.0`)
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Saturate

<fdef>function <type>Palette</type>.<func>Saturate</func>(<arg>*self*</arg>, <arg>amount</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>

Adjusts the saturation on a range of colours in the palette.

<listhead>Parameters</listhead>

* <arg>amount</arg> (<type>number</type>): The amount to adjust by (`0.0` - `2.0`)
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Illuminate

<fdef>function <type>Palette</type>.<func>Illuminate</func>(<arg>*self*</arg>, <arg>amount</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>

Adjusts the brightness on a range of colours in the palette.

<listhead>Parameters</listhead>

* <arg>amount</arg> (<type>number</type>): The amount to adjust by (`0.0` - `2.0`)
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Shift

<fdef>function <type>Palette</type>.<func>Shift</func>(<arg>*self*</arg>, <arg>amount</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>

Shifts the hue on a range of colours in the palette.

<listhead>Parameters</listhead>

* <arg>amount</arg> (<type>number</type>): The amount to shift by (`0.0` - `1.0`)
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Invert

<fdef>function <type>Palette</type>.<func>Invert</func>(<arg>*self*</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>

Inverts a range of colours in the palette.

<listhead>Parameters</listhead>

* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Gradient

<fdef>function <type>Palette</type>.<func>Invert</func>(<arg>*self*</arg>, <arg>startColour</arg>, <arg>endColour</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>

Converts a range of colours in the palette to a colour gradient from <arg>startColour</arg> to <arg>endColour</arg>.

<listhead>Parameters</listhead>

* <arg>startColour</arg> (<type>[Colour](../Colour.md)</type>): The starting colour of the gradient
* <arg>endColour</arg> (<type>[Colour](../Colour.md)</type>): The ending colour of the gradient
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to
