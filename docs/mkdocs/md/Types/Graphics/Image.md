<article-head>Image</article-head>

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
<prop class="ro">pixelFormat</prop> | <type>number</type> | The format of the pixels in the image (see [Image Pixel Formats](#_top))
<prop class="ro">width</prop> | <type>number</type> | The width of the image
<prop class="ro">height</prop> | <type>number</type> | The height of the image
<prop class="ro">hasPalette</prop> | <type>boolean</type> | Whether this image has an internal palette
<prop class="rw">palette</prop> | <type>[Palette](Palette.md)</type> | The image's internal palette. If <prop>hasPalette</prop> is `false` this will be greyscale
<prop class="rw">offsetX</prop> | <type>number</type> | The X offset of the image
<prop class="rw">offsetY</prop> | <type>number</type> | The Y offset of the image
<prop class="ro">stride</prop> | <type>number</type> | The number of bytes per row of image data
<prop class="ro">bpp</prop> | <type>number</type> | The number of bytes per pixel in the image data

## Constructors

<fdef>function <type>Image</type>.<func>new</func>()</fdef>

Creates a new, empty image with <prop>pixelFormat</prop> `PIXELFORMAT_RGBA`.

---

<fdef>function <type>Image</type>.<func>new</func>(<arg>pixelFormat</arg>)</fdef>

Creates a new, empty image of <arg>pixelFormat</arg>.

<listhead>Parameters</listhead>

* <arg>pixelFormat</arg> (<type>number</type>): The pixel format of the image (see `PIXELFORMAT_` constants)


## Functions - General

### Clear

<fdef>function <type>Image</type>.<func>Clear</func>(<arg>*self*</arg>)</fdef>

Clears all image data, resetting size and offsets to `0`.

---
### Copy

<fdef>function <type>Image</type>.<func>Copy</func>(<arg>*self*</arg>, <arg>other</arg>)</fdef>

Copies all data and properties from another <type>Image</type>.

<listhead>Parameters</listhead>

* <arg>other</arg> (<type>Image</type>): The image to copy from

---
### Create

<fdef>function <type>Image</type>.<func>Create</func>(<arg>*self*</arg>, <arg>width</arg>, <arg>height</arg>, <arg>type</arg>, <arg>palette</arg>)</fdef>

(Re)Creates the image with the given properties.

<listhead>Parameters</listhead>

* <arg>width</arg> (<type>number</type>): The width of the image
* <arg>height</arg> (<type>number</type>): The height of the image
* <arg>type</arg> (<type>number</type>): The type of image to create (see `PIXELFORMAT_` constants)
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use as the internal image palette


## Functions - Image Info

### CountUniqueColours

<fdef>function <type>Image</type>.<func>CountUniqueColours</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>number</type>: The number of unique colours present in the image, or `0` if the image isn't `PIXELFORMAT_INDEXED`

---
### FindUnusedColour

<fdef>function <type>Image</type>.<func>FindUnusedColour</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>number</type>: The index of the first colour in the palette not used in the image. Will return `-1` if the image isn't `PIXELFORMAT_INDEXED` or all colours are used

---
### IsValid

<fdef>function <type>Image</type>.<func>IsValid</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the image has any data

---
### PixelAt

<fdef>function <type>Image</type>.<func>PixelAt</func>(<arg>*self*</arg>, <arg>x</arg>, <arg>y</arg>, <arg>palette</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>x</arg> (<type>number</type>): The X position of the pixel
* <arg>y</arg> (<type>number</type>): The Y position of the pixel
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` and has no internal palette

<listhead>Returns</listhead>

* <type>[Colour](../Colour.md)</type>: The colour of the pixel at (<arg>x</arg>,<arg>y</arg>) in the image, or black+transparent if the given coordinates were out of range

---
### PixelIndexAt

<fdef>function <type>Image</type>.<func>PixelIndexAt</func>(<arg>*self*</arg>, <arg>x</arg>, <arg>y</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>x</arg> (<type>number</type>): The X position of the pixel
* <arg>y</arg> (<type>number</type>): The Y position of the pixel

<listhead>Returns</listhead>

* <type>number</type>: The palette index of the pixel at (<arg>x</arg>,<arg>y</arg>), or 0 if the given coordinates were out of range, or if the image isn't `PIXELFORMAT_INDEXED`


## Functions - Read Data

### LoadData

<fdef>function <type>Image</type>.<func>LoadData</func>(<arg>*self*</arg>, <arg>data</arg>, <arg>index</arg>, <arg>typeHint</arg>)</fdef>

Reads the given <arg>data</arg> to the image. The data can be in any supported image format.

<listhead>Parameters</listhead>

* <arg>data</arg> (<type>[DataBlock](../DataBlock.md)</type>): The data to read
* <arg>[index]</arg> (<type>number</type>, default `0`): The index of the image to load, for cases where the image format contains multiple images
* <arg>[typeHint]</arg> (<type>string</type>, default `""`): The image format id to try loading first, before auto-detecting

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### LoadEntry

<fdef>function <type>Image</type>.<func>LoadEntry</func>(<arg>*self*</arg>, <arg>entry</arg>, <arg>index</arg>)</fdef>

Reads the given <arg>entry</arg>'s data to the image. The data can be in any supported image format.

<listhead>Parameters</listhead>

* <arg>entry</arg> (<type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>): The entry to read
* <arg>[index]</arg> (<type>number</type>, default `0`): The index of the image to load, for cases where the image format contains multiple images

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success
* <type>string</type>: An error message if loading failed

#### Notes

This function can additionally load various 'font' formats to the image, which are as yet unsupported by <type>[ImageFormat](ImageFormat.md)</type> and therefore <func>[LoadData](#loaddata)</func>. It will also use the extra information available from the entry to better detect the correct image format.

## Functions - Write Data

### WriteIndexedData

<fdef>function <type>Image</type>.<func>WriteRGBAData</func>(<arg>*self*</arg>, <arg>block</arg>)</fdef>

Writes the raw image data into <arg>block</arg> in 8-bit indexed format.

<listhead>Parameters</listhead>

* <arg>block</arg> (<type>[DataBlock](../DataBlock.md)</type>): The <type>[DataBlock](../DataBlock.md)</type> to write the data to (will be resized to fit the data)

<listhead>Returns</listhead>

* <type>boolean</type> : `true` on success

---
### WriteRGBData

<fdef>function <type>Image</type>.<func>WriteRGBAData</func>(<arg>*self*</arg>, <arg>block</arg>, <arg>palette</arg>)</fdef>

Writes the raw image data into <arg>block</arg> in 24-bit RGB format.

<listhead>Parameters</listhead>

* <arg>block</arg> (<type>[DataBlock](../DataBlock.md)</type>): The <type>[DataBlock](../DataBlock.md)</type> to write the data to (will be resized to fit the data)
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type> : `true` on success

---
### WriteRGBAData

<fdef>function <type>Image</type>.<func>WriteRGBAData</func>(<arg>*self*</arg>, <arg>block</arg>, <arg>palette</arg>)</fdef>

Writes the raw image data into <arg>block</arg> in 32-bit RGBA format.

<listhead>Parameters</listhead>

* <arg>block</arg> (<type>[DataBlock](../DataBlock.md)</type>): The <type>[DataBlock](../DataBlock.md)</type> to write the data to (will be resized to fit the data)
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type> : `true` on success


## Functions - Pixel Format Conversion

### ConvertAlphaMap

<fdef>function <type>Image</type>.<func>ConvertAlphaMap</func>(<arg>*self*</arg>, <arg>alphaSource</arg>, <arg>palette</arg>)</fdef>

Converts the image to `PIXELFORMAT_ALPHA`.

<listhead>Parameters</listhead>

* <arg>alphaSource</arg> (<type>number</type>): The source to generate the alpha values from:
    * `SOURCE_ALPHA`: Use existing alpha values
    * `SOURCE_BRIGHTNESS`: Determine alpha from the brightness of each pixel's colour
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the image was converted

---
### ConvertIndexed

<fdef>function <type>Image</type>.<func>ConvertIndexed</func>(<arg>*self*</arg>, <arg>newPalette</arg>, <arg>currentPalette</arg>)</fdef>

Converts the image to `PIXELFORMAT_INDEXED`.

<listhead>Parameters</listhead>

* <arg>newPalette</arg> (<type>[Palette](Palette.md)</type>): The new palette to convert to (the image's <prop>palette</prop> will also be set to this)
* <arg>currentPalette</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The 'current' palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the image was converted

---
### ConvertRGBA

<fdef>function <type>Image</type>.<func>ConvertRGBA</func>(<arg>*self*</arg>, <arg>palette</arg>)</fdef>

Converts the image to `PIXELFORMAT_RGBA`.

<listhead>Parameters</listhead>

* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the image was converted


## Functions - General Modification

### Crop

<fdef>function <type>Image</type>.<func>Crop</func>(<arg>*self*</arg>, <arg>left</arg>, <arg>top</arg>, <arg>right</arg>, <arg>bottom</arg>)</fdef>

Crops the image to a new rectangle between (<arg>left</arg>, <arg>top</arg>) -> (<arg>right</arg>, <arg>bottom</arg>).

<listhead>Parameters</listhead>

* <arg>left</arg> (<type>number</type>): The new left of the image (anything to the left of this will be cut)
* <arg>top</arg> (<type>number</type>): The new top of the image (anything above this will be cut)
* <arg>right</arg> (<type>number</type>): The new right of the image (anything to the right of this will be cut)
* <arg>bottom</arg> (<type>number</type>): The new bottom of the image (anything below this will be cut)

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given parameters were invalid (must be within the existing dimensions)

---
### MirrorHorizontal

<fdef>function <type>Image</type>.<func>MirrorHorizontal</func>(<arg>*self*</arg>)</fdef>

Mirrors the image horizontally.

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### MirrorVertical

<fdef>function <type>Image</type>.<func>MirrorVertical</func>(<arg>*self*</arg>)</fdef>

Mirrors the image vertically.

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### Resize

<fdef>function <type>Image</type>.<func>Resize</func>(<arg>*self*</arg>, <arg>newWidth</arg>, <arg>newHeight</arg>)</fdef>

Resizes the image to <arg>newWidth</arg> x <arg>newHeight</arg>, conserving any existing image data.

<listhead>Parameters</listhead>

* <arg>newWidth</arg> (<type>number</type>): The new image width
* <arg>newHeight</arg> (<type>number</type>): The new image height

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### Rotate

<fdef>function <type>Image</type>.<func>Rotate</func>(<arg>*self*</arg>, <arg>angle</arg>)</fdef>

Rotates the image by <arg>angle</arg>.

<listhead>Parameters</listhead>

* <arg>angle</arg> (<type>number</type>): The amount (in degrees) to rotate the image, must be a multiple of 90

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the image was rotated successfully

---
### Trim

<fdef>function <type>Image</type>.<func>Trim</func>(<arg>*self*</arg>)</fdef>

Automatically crops the image to remove fully transparent rows/columns from the edges.


## Functions - Colour Modification

### ApplyTranslation

<fdef>function <type>Image</type>.<func>ApplyTranslation</func>(<arg>*self*</arg>, <arg>translation</arg>, <arg>palette</arg>, <arg>trueColour</arg>)</fdef>

Applies the given <arg>translation</arg> to the image.

<listhead>Parameters</listhead>

* <arg>translation</arg> (<type>[Translation](../Translation/Translation.md)</type>): The translation to apply
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>
* <arg>[trueColour]</arg> (<type>boolean</type>, default `false`): If `true`, the translation will be performed in 'truecolour' (32-bit) and the image converted to `PIXELFORMAT_RGBA`

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### Colourise

<fdef>function <type>Image</type>.<func>Colourise</func>(<arg>*self*</arg>, <arg>colour</arg>, <arg>palette</arg>)</fdef>

Colourises the image to <arg>colour</arg>.

<listhead>Parameters</listhead>

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to apply
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### SetPixelColour

<fdef>function <type>Image</type>.<func>SetPixelColour</func>(<arg>*self*</arg>, <arg>x</arg>, <arg>y</arg>, <arg>colour</arg>, <arg>palette</arg>)</fdef>

Sets the pixel at (<arg>x</arg>,<arg>y</arg>) to <arg>colour</arg>.

<listhead>Parameters</listhead>

* <arg>x</arg> (<type>number</type>): The X position of the pixel
* <arg>y</arg> (<type>number</type>): The Y position of the pixel
* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to set the pixel to
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given position was out of range

---
### SetPixelIndex

<fdef>function <type>Image</type>.<func>SetPixelIndex</func>(<arg>*self*</arg>, <arg>x</arg>, <arg>y</arg>, <arg>index</arg>, <arg>alpha</arg>)</fdef>

Ses the pixel at (<arg>x</arg>,<arg>y</arg>) to the palette colour <arg>index</arg>, and the pixel's <arg>alpha</arg> if possible.

<listhead>Parameters</listhead>

* <arg>x</arg> (<type>number</type>): The X position of the pixel
* <arg>y</arg> (<type>number</type>): The Y position of the pixel
* <arg>index</arg> (<type>number</type>): The (`0`-based) palette colour index
* <arg>[alpha]</arg> (<type>number</type>, default `255`): The alpha value for the pixel

<listhead>Returns</listhead>

* <type>boolean</type>: `false` if the given position was out of range

---
### Tint

<fdef>function <type>Image</type>.<func>Tint</func>(<arg>*self*</arg>, <arg>colour</arg>, <arg>amount</arg>, <arg>palette</arg>)</fdef>

Tints the image to <arg>colour</arg> by <arg>amount</arg>.

<listhead>Parameters</listhead>

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to apply
* <arg>amount</arg> (<type>number</type>): The amount to tint (`0.0` - `1.0`)
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success


## Functions - Alpha Modification

### FillAlpha

<fdef>function <type>Image</type>.<func>FillAlpha</func>(<arg>*self*</arg>, <arg>alpha</arg>)</fdef>

Sets all alpha values in the image to <arg>alpha</arg>.

<listhead>Parameters</listhead>

* <arg>alpha</arg> (<type>number</type>): The alpha value to set

---
### MaskFromBrightness

<fdef>function <type>Image</type>.<func>MaskFromBrightness</func>(<arg>*self*</arg>, <arg>palette</arg>)</fdef>

Changes the mask/alpha channel so that each pixel's transparency matches its brigntness level (where black is fully transparent).

<listhead>Parameters</listhead>

* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success

---
### MaskFromColour

<fdef>function <type>Image</type>.<func>MaskFromColour</func>(<arg>*self*</arg>, <arg>colour</arg>, <arg>palette</arg>)</fdef>

Changes the mask/alpha channel so that pixels that match <arg>colour</arg> are fully transparent, and all other pixels fully opaque.

<listhead>Parameters</listhead>

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to set as transparent
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

* <type>boolean</type>: `true` on success


## Functions - Drawing

### DrawImage

<fdef>function <type>Image</type>.<func>DrawImage</func>(<arg>*self*</arg>, <arg>x</arg>, <arg>y</arg>, <arg>srcImage</arg>, <arg>drawOptions</arg>, <arg>srcPalette</arg>, <arg>destPalette</arg>)</fdef>

Draws <arg>srcImage</arg> at (<arg>x</arg>,<arg>y</arg>) on the image.

<listhead>Parameters</listhead>

* <arg>x</arg> (<type>number</type>): The X position to draw <arg>srcImage</arg>. Can be negative
* <arg>y</arg> (<type>number</type>): The Y position to draw <arg>srcImage</arg>. Can be negative
* <arg>srcImage</arg> (<type>Image</type>): The image to draw
* <arg>drawOptions</arg> (<type>[ImageDrawOptions](ImageDrawOptions.md)</type>): Options for blending <arg>srcImage</arg>'s pixels with the existing image
* <arg>[srcPalette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use for <arg>srcImage</arg> if it is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>
* <arg>[destPalette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if this image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

#### Notes

<arg>srcImage</arg> is drawn with its top-left at the given coordinates.

This will not resize the current image - any part of the drawn <arg>srcImage</arg> that goes past the edge of the image's bounds will be cut off.

The pixel colour calculations will always be done in 32-bit, and if the image is `PIXELFORMAT_INDEXED`, the resulting pixel colours will be converted to their nearest match in the image's <prop>palette</prop> (or the <arg>destPalette</arg> parameter if the image doesn't have one).

---
### DrawPixel

<fdef>function <type>Image</type>.<func>DrawPixel</func>(<arg>*self*</arg>, <arg>x</arg>, <arg>y</arg>, <arg>colour</arg>, <arg>drawOptions</arg>, <arg>palette</arg>)</fdef>

Draws a pixel of <arg>colour</arg> at (<arg>x</arg>,<arg>y</arg>) on the image.

<listhead>Parameters</listhead>

* <arg>x</arg> (<type>number</type>): The X position to draw the pixel
* <arg>y</arg> (<type>number</type>): The Y position to draw the pixel
* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour of the pixel to draw
* <arg>drawOptions</arg> (<type>[ImageDrawOptions](ImageDrawOptions.md)</type>): Options for blending the pixel with the existing image
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>, default `nil`): The palette to use if the image is `PIXELFORMAT_INDEXED` but doesn't have an internal <prop>palette</prop>

<listhead>Returns</listhead>

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
