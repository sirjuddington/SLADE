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
