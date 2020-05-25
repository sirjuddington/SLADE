<subhead>Type</subhead>
<header>Image</header>

An <type>Image</type> contains pixel data making up an image. The data can be in multiple formats (see the 'Image Pixel Formats' section below).

The type also has various functions for manipulating image data and reading the various formats supported by SLADE (see <type>[ImageFormat](ImageFormat.md)</type>).

### Image Pixel Formats

An image can have one of three pixel formats, which determines how data for each pixel is stored:

* `PIXELFORMAT_INDEXED`: The image is 8-bit with a mask. Each pixel is 2 bytes - one (`0`-based) palette index and one alpha value. The palette can be stored in the image (see <prop>palette</prop>/<prop>hasPalette</prop> properties). If it doesn't have an internal palette, an external one needs to be supplied in any functions dealing with colours in the image
* `PIXELFORMAT_RGBA`: The image is 32-bit. Each pixel is 4 bytes - red, green, blue, alpha
* `PIXELFORMAT_ALPHA`: The image is purely an 8-bit alpha map. Each pixel is 1 byte - the alpha value

## Constants

| Name | Value |
|:-----|:------|
`PIXELFORMAT_INDEXED` | 0
`PIXELFORMAT_RGBA` | 1
`PIXELFORMAT_ALPHA` | 2
`SOURCE_BRIGHTNESS` | 0
`SOURCE_ALPHA` | 1

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">pixelFormat</prop> | <type>integer</type> | The format of the pixels in the image (see [Image Pixel Formats](#_top))
<prop class="ro">width</prop> | <type>integer</type> | The width of the image
<prop class="ro">height</prop> | <type>integer</type> | The height of the image
<prop class="ro">hasPalette</prop> | <type>boolean</type> | Whether this image has an internal palette
<prop class="rw">palette</prop> | <type>[Palette](Palette.md)</type> | The image's internal palette. If <prop>hasPalette</prop> is `false` this will be greyscale
<prop class="rw">offsetX</prop> | <type>integer</type> | The X offset of the image
<prop class="rw">offsetY</prop> | <type>integer</type> | The Y offset of the image
<prop class="ro">stride</prop> | <type>integer</type> | The number of bytes per row of image data
<prop class="ro">bpp</prop> | <type>integer</type> | The number of bytes per pixel in the image data

## Constructors

<code><type>Image</type>.new()</code>

Creates a new, empty image with <prop>pixelFormat</prop> `PIXELFORMAT_RGBA`.

---

<code><type>Image</type>.new(<arg>pixelFormat</arg>)</code>

Creates a new, empty image of <arg>pixelFormat</arg>.

#### Parameters

* <arg>pixelFormat</arg> (<type>integer</type>): The pixel format of the image (see `PIXELFORMAT_` constants)


## Functions

### Overview

#### General

<fdef>[Clear](#clear)()</fdef>
<fdef>[Copy](#copy)(<arg>other</arg>)</fdef>
<fdef>[Create](#create)(<arg>width</arg>, <arg>height</arg>, <arg>type</arg>, <arg>palette</arg>)</fdef>

#### Image Info

<fdef>[CountUniqueColours](#countuniquecolours)() -> <type>integer</type></fdef>
<fdef>[FindUnusedColour](#findunusedcolour)() -> <type>integer</type></fdef>
<fdef>[IsValid](#isvalid)() -> <type>boolean</type></fdef>
<fdef>[PixelAt](#pixelat)(<arg>x</arg>, <arg>y</arg>, <arg>[palette]</arg>) -> <type>[Colour](../Colour.md)</type></fdef>
<fdef>[PixelIndexAt](#pixelindexat)(<arg>x</arg>, <arg>y</arg>) -> <type>integer</type></fdef>

#### Read Data

<fdef>[LoadData](#loaddata)(<arg>data</arg>, <arg>[index]</arg>, <arg>[typeHint]</arg>) -> <type>boolean</type></fdef>
<fdef>[LoadEntry](#loadentry)(<arg>entry</arg>, <arg>[index]</arg>) -> <type>boolean</type>, <type>string</type></fdef>

#### Write Data

<fdef>[WriteIndexedData](#writeindexeddata)(<arg>block</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteRGBAData](#writergbadata)(<arg>block</arg>, <arg>[palette]</arg>) -> <type>boolean</type></fdef>
<fdef>[WriteRGBAData](#writergbadata)(<arg>block</arg>, <arg>[palette]</arg>) -> <type>boolean</type></fdef>

#### Pixel Format Conversion

<fdef>[ConvertAlphaMap](#convertalphamap)(<arg>alphaSource</arg>, <arg>[palette]</arg>) -> <type>boolean</type></fdef>
<fdef>[ConvertIndexed](#convertindexed)(<arg>newPalette</arg>, <arg>currentPalette</arg>) -> <type>boolean</type></fdef>
<fdef>[ConvertRGBA](#convertrgba)(<arg>[palette]</arg>) -> <type>boolean</type></fdef>

#### General Modification

<fdef>[Crop](#crop)(<arg>left</arg>, <arg>top</arg>, <arg>right</arg>, <arg>bottom</arg>) -> <type>boolean</type></fdef>
<fdef>[MirrorHorizontal](#mirrorhorizontal)() -> <type>boolean</type></fdef>
<fdef>[MirrorVertical](#mirrorvertical)() -> <type>boolean</type></fdef>
<fdef>[Resize](#resize)(<arg>newWidth</arg>, <arg>newHeight</arg>) -> <type>boolean</type></fdef>
<fdef>[Rotate](#rotate)(<arg>angle</arg>) -> <type>boolean</type></fdef>
<fdef>[Trim](#trim)()</fdef>

#### Colour Modification

<fdef>[ApplyTranslation](#applytranslation)(<arg>translation</arg>, <arg>[palette]</arg>, <arg>[trueColour]</arg>) -> <type>boolean</type></fdef>
<fdef>[Colourise](#colourise)(<arg>colour</arg>, <arg>[palette]</arg>) -> <type>boolean</type></fdef>
<fdef>[SetPixelColour](#setpixelcolour)(<arg>x</arg>, <arg>y</arg>, <arg>colour</arg>, <arg>[palette]</arg>) -> <type>boolean</type></fdef>
<fdef>[SetPixelIndex](#setpixelindex)(<arg>x</arg>, <arg>y</arg>, <arg>index</arg>, <arg>[alpha]</arg>) -> <type>boolean</type></fdef>
<fdef>[Tint](#tint)(<arg>colour</arg>, <arg>amount</arg>, <arg>[palette]</arg>) -> <type>boolean</type></fdef>

#### Alpha Modification

<fdef>[FillAlpha](#fillalpha)(<arg>alpha</arg>)</fdef>
<fdef>[MaskFromBrightness](#maskfrombrightness)(<arg>[palette]</arg>) -> <type>boolean</type></fdef>
<fdef>[MaskFromColour](#maskfromcolour)(<arg>colour</arg>, <arg>[palette]</arg>) -> <type>boolean</type></fdef>

#### Drawing

<fdef>[DrawImage](#drawimage)(<arg>x</arg>, <arg>y</arg>, <arg>srcImage</arg>, <arg>drawOptions</arg>, <arg>[srcPalette]</arg>, <arg>[destPalette]</arg>)</fdef>
<fdef>[DrawPixel](#drawpixel)(<arg>x</arg>, <arg>y</arg>, <arg>colour</arg>, <arg>drawOptions</arg>, <arg>[palette]</arg>) -> <type>boolean</type></fdef>


---
### Clear

Clears all image data, resetting size and offsets to `0`.

---
### Copy

Copies all data and properties from another <type>Image</type>.

#### Parameters

* <arg>other</arg> (<type>Image</type>): The image to copy from

---
### Create

(Re)Creates the image with the given properties.

#### Parameters

* <arg>width</arg> (<type>integer</type>): The width of the image
* <arg>height</arg> (<type>integer</type>): The height of the image
* <arg>type</arg> (<type>integer</type>): The type of image to create (see `PIXELFORMAT_` constants)
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use as the internal image palette

---
### CountUniqueColours

Counts the number of unique colours present in the image.

#### Returns

* <type>integer</type>: The number of unique colours present in the image, or `0` if the image isn't `PIXELFORMAT_INDEXED`

---
### FindUnusedColour

Gets the index of the first unused colour in the palette. Will return `-1` if the image isn't `PIXELFORMAT_INDEXED` or if all colours are used.

#### Returns

* <type>integer</type>: The index of the first colour in the palette not used in the image.

---
### IsValid

Checks if the image is valid (ie. has any data).

#### Returns

* <type>boolean</type>: `true` if the image has any data

---
### PixelAt

#### Parameters

* <arg>x</arg> (<type>integer</type>): The X position of the pixel
* <arg>y</arg> (<type>integer</type>): The Y position of the pixel
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` and has no internal palette. Default is `nil`

#### Returns

* <type>[Colour](../Colour.md)</type>: The colour of the pixel at (<arg>x</arg>,<arg>y</arg>) in the image, or black+transparent if the given coordinates were out of range

---
### PixelIndexAt

Gets the palette index of the pixel at (<arg>x</arg>,<arg>y</arg>). Will be `0` if the given coordinates were out of range, or if the image isn't `PIXELFORMAT_INDEXED`.

#### Parameters

* <arg>x</arg> (<type>integer</type>): The X position of the pixel
* <arg>y</arg> (<type>integer</type>): The Y position of the pixel

#### Returns

* <type>integer</type>: The palette index of the pixel at (<arg>x</arg>,<arg>y</arg>)

---
### LoadData

Reads the given <arg>data</arg> to the image. The data can be in any supported image format.

#### Parameters

* <arg>data</arg> (<type>[DataBlock](../DataBlock.md)</type>): The data to read
* <arg>[index]</arg> (<type>integer</type>): The index of the image to load, for cases where the image format contains multiple images. Default is `0`
* <arg>[typeHint]</arg> (<type>string</type>): The image format id to try loading first, before auto-detecting. Default is `""`

#### Returns

* <type>boolean</type>: `true` on success

---
### LoadEntry

Reads the given <arg>entry</arg>'s data to the image. The data can be in any supported image format.

#### Parameters

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to read
* <arg>[index]</arg> (<type>integer</type>): The index of the image to load, for cases where the image format contains multiple images. Default is `0`

#### Returns

* <type>boolean</type>: `true` on success
* <type>string</type>: An error message if loading failed

#### Notes

This function can additionally load various 'font' formats to the image, which are as yet unsupported by <type>[ImageFormat](ImageFormat.md)</type> and therefore <func>[LoadData](#loaddata)</func>. It will also use the extra information available from the entry to better detect the correct image format.

---
### WriteIndexedData

Writes the raw image data into <arg>block</arg> in 8-bit indexed format.

#### Parameters

* <arg>block</arg> (<type>[DataBlock](../DataBlock.md)</type>): The <type>[DataBlock](../DataBlock.md)</type> to write the data to (will be resized to fit the data)

#### Returns

* <type>boolean</type>: `true` on success

---
### WriteRGBData

Writes the raw image data into <arg>block</arg> in 24-bit RGB format.

#### Parameters

* <arg>block</arg> (<type>[DataBlock](../DataBlock.md)</type>): The <type>[DataBlock](../DataBlock.md)</type> to write the data to (will be resized to fit the data)
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `true` on success

---
### WriteRGBAData

Writes the raw image data into <arg>block</arg> in 32-bit RGBA format.

#### Parameters

* <arg>block</arg> (<type>[DataBlock](../DataBlock.md)</type>): The <type>[DataBlock](../DataBlock.md)</type> to write the data to (will be resized to fit the data)
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `true` on success

---
### ConvertAlphaMap

Converts the image to `PIXELFORMAT_ALPHA`.

#### Parameters

* <arg>alphaSource</arg> (<type>integer</type>): The source to generate the alpha values from:
    * `SOURCE_ALPHA`: Use existing alpha values
    * `SOURCE_BRIGHTNESS`: Determine alpha from the brightness of each pixel's colour
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `true` if the image was converted

---
### ConvertIndexed

Converts the image to `PIXELFORMAT_INDEXED`.

#### Parameters

* <arg>newPalette</arg> (<type>[Palette](Palette.md)</type>): The new palette to convert to (the image's <prop>palette</prop> will also be set to this)
* <arg>currentPalette</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The 'current' palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

#### Returns

* <type>boolean</type>: `true` if the image was converted

---
### ConvertRGBA

Converts the image to `PIXELFORMAT_RGBA`.

#### Parameters

* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `true` if the image was converted

---
### Crop

Crops the image to a new rectangle between (<arg>left</arg>, <arg>top</arg>) -> (<arg>right</arg>, <arg>bottom</arg>).

#### Parameters

* <arg>left</arg> (<type>integer</type>): The new left of the image (anything to the left of this will be cut)
* <arg>top</arg> (<type>integer</type>): The new top of the image (anything above this will be cut)
* <arg>right</arg> (<type>integer</type>): The new right of the image (anything to the right of this will be cut)
* <arg>bottom</arg> (<type>integer</type>): The new bottom of the image (anything below this will be cut)

#### Returns

* <type>boolean</type>: `false` if the given parameters were invalid (must be within the existing dimensions)

---
### MirrorHorizontal

Mirrors the image horizontally.

#### Returns

* <type>boolean</type>: `true` on success

---
### MirrorVertical

Mirrors the image vertically.

#### Returns

* <type>boolean</type>: `true` on success

---
### Resize

Resizes the image to <arg>newWidth</arg> x <arg>newHeight</arg>, conserving any existing image data.

#### Parameters

* <arg>newWidth</arg> (<type>integer</type>): The new image width
* <arg>newHeight</arg> (<type>integer</type>): The new image height

#### Returns

* <type>boolean</type>: `true` on success

---
### Rotate

Rotates the image by <arg>angle</arg>.

#### Parameters

* <arg>angle</arg> (<type>integer</type>): The amount (in degrees) to rotate the image

#### Returns

* <type>boolean</type>: `true` if the image was rotated successfully

#### Notes

The <arg>angle</arg> must be a multiple of 90, other rotations are not currently supported.

---
### Trim

Automatically crops the image to remove fully transparent rows/columns from the edges.

---
### ApplyTranslation

Applies the given <arg>translation</arg> to the image.

#### Parameters

* <arg>translation</arg> (<type>[Translation](../Translation/Translation.md)</type>): The translation to apply
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`
* <arg>[trueColour]</arg> (<type>boolean</type>): If `true`, the translation will be performed in 'truecolour' (32-bit) and the image converted to `PIXELFORMAT_RGBA`. Default is `false`

#### Returns

* <type>boolean</type>: `true` on success

---
### Colourise

Colourises the image to <arg>colour</arg>.

#### Parameters

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to apply
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `true` on success

---
### SetPixelColour

Sets the pixel at (<arg>x</arg>,<arg>y</arg>) to <arg>colour</arg>.

#### Parameters

* <arg>x</arg> (<type>integer</type>): The X position of the pixel
* <arg>y</arg> (<type>integer</type>): The Y position of the pixel
* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to set the pixel to
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `false` if the given position was out of range

---
### SetPixelIndex

Ses the pixel at (<arg>x</arg>,<arg>y</arg>) to the palette colour <arg>index</arg>, and the pixel's <arg>alpha</arg> if possible.

#### Parameters

* <arg>x</arg> (<type>integer</type>): The X position of the pixel
* <arg>y</arg> (<type>integer</type>): The Y position of the pixel
* <arg>index</arg> (<type>integer</type>): The (`0`-based) palette colour index
* <arg>[alpha]</arg> (<type>integer</type>): The alpha value for the pixel. Default is `255` (fully opaque)

#### Returns

* <type>boolean</type>: `false` if the given position was out of range

---
### Tint

Tints the image to <arg>colour</arg> by <arg>amount</arg>.

#### Parameters

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to apply
* <arg>amount</arg> (<type>float</type>): The amount to tint (`0.0` - `1.0`)
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `true` on success

---
### FillAlpha

Sets all alpha values in the image to <arg>alpha</arg>.

#### Parameters

* <arg>alpha</arg> (<type>integer</type>): The alpha value to set

---
### MaskFromBrightness

Changes the mask/alpha channel so that each pixel's transparency matches its brigntness level (where black is fully transparent).

#### Parameters

* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `true` on success

---
### MaskFromColour

Changes the mask/alpha channel so that pixels that match <arg>colour</arg> are fully transparent, and all other pixels fully opaque.

#### Parameters

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to set as transparent
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `true` on success

---
### DrawImage

Draws <arg>srcImage</arg> at (<arg>x</arg>,<arg>y</arg>) on the image.

#### Parameters

* <arg>x</arg> (<type>integer</type>): The X position to draw <arg>srcImage</arg>. Can be negative
* <arg>y</arg> (<type>integer</type>): The Y position to draw <arg>srcImage</arg>. Can be negative
* <arg>srcImage</arg> (<type>Image</type>): The image to draw
* <arg>drawOptions</arg> (<type>[ImageDrawOptions](ImageDrawOptions.md)</type>): Options for blending <arg>srcImage</arg>'s pixels with the existing image
* <arg>[srcPalette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use for <arg>srcImage</arg> if it is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`
* <arg>[destPalette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if this image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Notes

<arg>srcImage</arg> is drawn with its top-left at the given coordinates.

This will not resize the current image - any part of the drawn <arg>srcImage</arg> that goes past the edge of the image's bounds will be cut off.

The pixel colour calculations will always be done in 32-bit, and if the image is `PIXELFORMAT_INDEXED`, the resulting pixel colours will be converted to their nearest match in the image's <prop>palette</prop> (or the <arg>destPalette</arg> parameter if the image doesn't have one).

---
### DrawPixel

Draws a pixel of <arg>colour</arg> at (<arg>x</arg>,<arg>y</arg>) on the image.

#### Parameters

* <arg>x</arg> (<type>integer</type>): The X position to draw the pixel
* <arg>y</arg> (<type>integer</type>): The Y position to draw the pixel
* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour of the pixel to draw
* <arg>drawOptions</arg> (<type>[ImageDrawOptions](ImageDrawOptions.md)</type>): Options for blending the pixel with the existing image
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>. Default is `nil`

#### Returns

* <type>boolean</type>: `true` on success

#### Notes

The pixel colour calculations will always be done in 32-bit, and if the image is `PIXELFORMAT_INDEXED`, the resulting pixel colour will be converted to its nearest match in the image's <prop>palette</prop> (or the <arg>palette</arg> parameter if the image doesn't have one).

#### Example

This will draw a red pixel at (`10`,`20`) on `image` blended additively at 50% opacity.

```lua
local drawOpt = ImageDrawOptions.new()
drawOpt.blend = Graphics.BLEND_ADD
drawOpt.alpha = 0.5
image:DrawPixel(10, 20, Colour.new(255, 0, 0), drawOpt)
```
