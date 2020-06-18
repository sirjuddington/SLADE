<subhead>Type</subhead>
<header>ImageConvertOptions</header>

Contains various options for converting an image from one pixel format to another.

<listhead>See:</listhead>

* <code>[ImageFormat.ConvertWritable](ImageFormat.md#convertwritable)</code>

## Constants

| Name | Value |
|:-----|:------|
`MASK_NONE` | 0
`MASK_COLOUR` | 1
`MASK_ALPHA` | 2
`MASK_BRIGHTNESS` | 3

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">paletteCurrent</prop> | <type>[Palette](Palette.md)</type> | The palette to use for the target image if it is 8-bit indexed but has no internal palette
<prop class="rw">paletteTarget</prop> | <type>[Palette](Palette.md)</type> | The palette to convert to
<prop class="rw">maskSource</prop> | <type>integer</type> | What to use to generate the alpha channel in the converted image. See `MASK_` constants above
<prop class="rw">maskColour</prop> | <type>[Colour](../Colour.md)</type> | The colour to use as transparent if <prop>maskSource</prop> is set to `MASK_COLOUR`
<prop class="rw">alphaThreshold</prop> | <type>integer</type> | The threshold 8-bit alpha value to use in cases where the target image format does not support a full alpha channel. Anything less than this will be fully transparent
<prop class="rw">transparency</prop> | <type>boolean</type> | If `false`, all pixels in the converted image will be set to fully opaque
<prop class="rw">pixelFormat</prop> | <type>integer</type> | The pixel format to use if the target image format supports multiple (see [Image](Image.md#constants)`.PIXELFORMAT_` constants)

## Constructors

<code><type>ImageConvertOptions</type>.<func>new</func>()</code>

Creates a new <type>ImageConvertOptions</type> with the following default properties:

* <prop>maskSource</prop>: `MASK_ALPHA`
* <prop>alphaThreshold</prop>: `0`
* <prop>transparency</prop>: `true`
