<article-head>Graphics</article-head>

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

### ImageFormat

<fdef>function Graphics.<func>ImageFormat</func>(<arg>formatId</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>formatId</arg> (<type>string</type>): The id of the format to get

<listhead>Returns</listhead>

* <type>[ImageFormat](../Types/Graphics/ImageFormat.md)</type>: The format matching the given <arg>formatId</arg>. Will be the 'unknown' format if no format with a matching id was found

---
### AllImageFormats

<fdef>function Graphics.<func>AllImageFormats</func>()</fdef>

<listhead>Returns</listhead>

* <type>[ImageFormat](../Types/Graphics/ImageFormat.md)\[\]</type>: An array of all supported image formats

---
### DetectImageFormat

<fdef>function Graphics.<func>DetectImageFormat</func>(<arg>data</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>data</arg> (<type>[DataBlock](../Types/DataBlock.md)</type>): The image data to check

<listhead>Returns</listhead>

* <type>[ImageFormat](../Types/Graphics/ImageFormat.md)</type>: The format <arg>data</arg> was detected as, or the 'unknown' format if the data couldn't be detected as a known format
