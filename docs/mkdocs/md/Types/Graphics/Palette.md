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

<code><type>Palette</type>.<func>new</func>()</code>

Creates a new palette with 256 colours in a greyscale gradient (from black to white).

---

<code><type>Palette</type>.<func>new</func>(<arg>count</arg>)</code>

Creates a new palette with <arg>count</arg> colours in a greyscale gradient.

#### Parameters

* <arg>count</arg> (<type>number</type>): The number of colours in the palette


## Functions

### Overview

#### General

<fdef>[Colour](#colour)(<arg>index</arg>) -> <type>[Colour](../Colour.md)</type></fdef>
<fdef>[CopyColours](#copycolours)(<arg>other</arg>)</fdef>
<fdef>[CountUniqueColours](#countuniquecolours)() -> <type>number</type></fdef>
<fdef>[FindColour](#findcolour)(<arg>colour</arg>) -> <type>number</type></fdef>
<fdef>[NearestColour](#nearestcolour)(<arg>colour</arg>, <arg>[matchMode]</arg>) -> <type>number</type></fdef>

#### Load/Save

<fdef>[LoadData](#loaddata)(<arg>data</arg>, <arg>[format]</arg>) -> <type>boolean</type></fdef>
<fdef>[LoadFile](#loadfile)(<arg>path</arg>, <arg>[format]</arg>) -> <type>boolean</type></fdef>
<fdef>[SaveFile](#savefile)(<arg>path</arg>, <arg>[format]</arg>)</fdef>

#### Colour Modification

<fdef>[SetColour](#setcolour)(<arg>index</arg>, <arg>colour</arg>)</fdef>
<fdef>[SetColourR](#setcolourr)(<arg>index</arg>, <arg>r</arg>)</fdef>
<fdef>[SetColourG](#setcolourg)(<arg>index</arg>, <arg>g</arg>)</fdef>
<fdef>[SetColourB](#setcolourb)(<arg>index</arg>, <arg>b</arg>)</fdef>
<fdef>[SetColourA](#setcoloura)(<arg>index</arg>, <arg>a</arg>)</fdef>

#### Colour Range Modification

<fdef>[ApplyTranslation](#applytranslation)(<arg>translation</arg>)</fdef>
<fdef>[Colourise](#colourise)(<arg>colour</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>
<fdef>[Tint](#tint)(<arg>colour</arg>, <arg>amount</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>
<fdef>[Saturate](#saturate)(<arg>amount</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>
<fdef>[Illuminate](#illuminate)(<arg>amount</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>
<fdef>[Shift](#shift)(<arg>amount</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>
<fdef>[Invert](#invert)(<arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>
<fdef>[Gradient](#invert)(<arg>startColour</arg>, <arg>endColour</arg>, <arg>firstIndex</arg>, <arg>lastIndex</arg>)</fdef>

---
### Colour

Gets the colour at the given (`0`-based) <arg>index</arg> in the palette.

#### Parameters

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour to get

#### Returns

* <type>[Colour](../Colour.md)</type>: The colour at the given <arg>index</arg>

---
### CopyColours

Copies all colours from another palette.

#### Parameters

* <arg>other</arg> (<type>Palette</type>): The palette to copy from

#### Notes

This will not change the <prop>colourCount</prop> of the palette. If the <arg>other</arg> palette has less colours than this, the remaining colours in this palette will be left the same.

---
### CountUniqueColours

Counts the number of unique colours in the palette.

#### Returns

* <type>number</type>: The number of unique colours present in the palette

---
### FindColour

Finds the index of the first colour in the palette that *exactly* matches the given <arg>colour</arg>.

#### Parameters

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to find in the palette

#### Returns

* <type>number</type>: The (`0`-based) index of the first matching colour in the palette, or `-1` if no match was found

---
### NearestColour

Finds the index of the colour in the palette that most closely matches the given <arg>colour</arg>.

#### Parameters

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to find in the palette
* <arg>[matchMode]</arg> (<type>number</type>): The colour matching algorithm to use (see `MATCH_` constants). Default is `MATCH_DEFAULT`, which means the colour matching algorithm currently selected in the SLADE preferences will be used

#### Returns

* <type>number</type>: The (`0`-based) index of the closest matching colour

---
### LoadData

Loads the given <arg>data</arg> into the palette.

#### Parameters

* <arg>data</arg> (<type>string</type>): The binary data to load
* <arg>[format]</arg> (<type>number</type>): The format of the data. See `FORMAT_` constants. Default is `FORMAT_RAW`

#### Returns

* <type>boolean</type>: `true` on success

---
### LoadFile

Loads the file at <arg>path</arg> on disk into the palette.

#### Parameters

* <arg>path</arg> (<type>string</type>): The full path to the file to load
* <arg>[format]</arg> (<type>number</type>): The format of the file. See `FORMAT_` constants. Default is `FORMAT_RAW`

#### Returns

* <type>boolean</type>: `true` on success

---
### SaveFile

Saves the palette to a file at <arg>path</arg> on disk in the specified <arg>format</arg>.

#### Parameters

* <arg>path</arg> (<type>string</type>): The full path to the file
* <arg>[format]</arg> (<type>number</type>): The format of the file. See `FORMAT_` constants. Default is `FORMAT_RAW`

#### Returns

* <type>boolean</type>: `true` on success

---
### SetColour

Sets the colour at <arg>index</arg> in the palette.

#### Parameters

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour to set
* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to set it to

---
### SetColourR

Sets red component of the colour at <arg>index</arg> in the palette.

#### Parameters

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour in the palette
* <arg>r</arg> (<type>number</type>): The new red component value

---
### SetColourG

Sets green component of the colour at <arg>index</arg> in the palette.

#### Parameters

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour in the palette
* <arg>g</arg> (<type>number</type>): The new green component value

---
### SetColourB

Sets blue component of the colour at <arg>index</arg> in the palette.

#### Parameters

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour in the palette
* <arg>b</arg> (<type>number</type>): The new blue component value

---
### SetColourA

Sets alpha component of the colour at <arg>index</arg> in the palette.

#### Parameters

* <arg>index</arg> (<type>number</type>): The (`0`-based) index of the colour in the palette
* <arg>a</arg> (<type>number</type>): The new green component value

---
### ApplyTranslation

Applies a <type>[Translation](../Translation/Translation.md)</type> to the palette.

#### Parameters

* <arg>translation</arg> (<type>[Translation](../Translation/Translation.md)</type>): The translation to apply

---
### Colourise

Colourises a range of colours in the palette.

#### Parameters

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to use
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Tint

Tints a range of colours in the palette.

#### Parameters

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to use
* <arg>amount</arg> (<type>number</type>): The amount to tint (`0.0` - `1.0`)
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Saturate

Adjusts the saturation on a range of colours in the palette.

#### Parameters

* <arg>amount</arg> (<type>number</type>): The amount to adjust by (`0.0` - `2.0`)
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Illuminate

Adjusts the brightness on a range of colours in the palette.

#### Parameters

* <arg>amount</arg> (<type>number</type>): The amount to adjust by (`0.0` - `2.0`)
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Shift

Shifts the hue on a range of colours in the palette.

#### Parameters

* <arg>amount</arg> (<type>number</type>): The amount to shift by (`0.0` - `1.0`)
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Invert

Inverts a range of colours in the palette.

#### Parameters

* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to

---
### Gradient

Converts a range of colours in the palette to a colour gradient from <arg>startColour</arg> to <arg>endColour</arg>.

#### Parameters

* <arg>startColour</arg> (<type>[Colour](../Colour.md)</type>): The starting colour of the gradient
* <arg>endColour</arg> (<type>[Colour](../Colour.md)</type>): The ending colour of the gradient
* <arg>firstIndex</arg> (<type>number</type>): The (`0`-based) index of the first colour to apply to
* <arg>lastIndex</arg> (<type>number</type>): The (`0`-based) index of the last colour to apply to
