<subhead>Namespace</subhead>
<header>Graphics</header>

The `Graphics` namespace contains various graphics-related functions.

## Constants

| Name | Value |
|:-----|:-----:|
`BLEND_NORMAL` | 0
`BLEND_ADD` | 1
`BLEND_SUBTRACT` | 2
`BLEND_REVERSESUBTRACT` | 3
`BLEND_MODULATE` | 4

## Functions

### Overview

#### Image Format

<fdef>[ImageFormat](imageformat)</func>(<arg>formatId</arg>) -> <type>[ImageFormat](../Types/Graphics/ImageFormat.md)</type></fdef>
<fdef>[AllImageFormats](allimageformats)</func>() -> <type>[ImageFormat](../Types/Graphics/ImageFormat.md)\[\]</type></fdef>
<fdef>[DetectImageFormat](detectimageformat)</func>(<arg>data</arg>) -> <type>[ImageFormat](../Types/Graphics/ImageFormat.md)</type></fdef>

#### Images

<fdef>[GetImageInfo](#getinfo)(<arg>data</arg>, <arg>[index]</arg>) -> <type>table</type></fdef>

---
### ImageFormat

#### Parameters

* <arg>formatId</arg> (<type>string</type>): The id of the format to get

#### Returns

* <type>[ImageFormat](../Types/Graphics/ImageFormat.md)</type>: The format matching the given <arg>formatId</arg>. Will be the 'unknown' format if no format with a matching id was found

---
### AllImageFormats

#### Returns

* <type>[ImageFormat](../Types/Graphics/ImageFormat.md)\[\]</type>: An array of all supported image formats

---
### DetectImageFormat

#### Parameters

* <arg>data</arg> (<type>[DataBlock](../Types/DataBlock.md)</type>): The image data to check

#### Returns

* <type>[ImageFormat](../Types/Graphics/ImageFormat.md)</type>: The format <arg>data</arg> was detected as, or the 'unknown' format if the data couldn't be detected as a known format

---
### GetImageInfo

Returns a <type>table</type> containing basic information about the image contained in <arg>data</arg>.

#### Parameters

* <arg>data</arg> (<type>[DataBlock](../Types/DataBlock.md)</type>): The image data to read
* <arg>[index]</arg> (<type>integer</type>): The index of the image in the data (for cases where the image format contains multiple images, eg. Duke3D ART). Default is `0`

#### Returns

* <type>table</type>: A table containing information about the image (see notes below)

#### Notes

The table returned by this function has the following keys:

| Name | Type | Description |
|:-----|:-----|:------------|
<nobr>`width`</nobr> | <type>integer</type> | The pixel width of the image
<nobr>`height`</nobr> | <type>integer</type> | The pixel height of the image
<nobr>`type`</nobr> | <type>integer</type> | The pixel format of the image (see <type>[Image](../Types/Graphics/Image.md#constants)</type>`.PIXELFORMAT_` constants)
<nobr>`format`</nobr> | <type>string</type> | The format id of the image (see <type>[ImageFormat](../Types/Graphics/ImageFormat.md)</type>`.`<prop>id</prop>)
<nobr>`imageCount`</nobr> | <type>integer</type> | The number of sub-images contained in the image
<nobr>`offsetX`</nobr> | <type>integer</type> | The X-offset of the image
<nobr>`offsetY`</nobr> | <type>integer</type> | The Y-offset of the image
<nobr>`hasPalette`</nobr> | <type>boolean</type> | `true` if the image contains an internal palette
