The <type>ImageFormat</type> type is a handler for an image format that SLADE supports (eg. Doom Gfx), with the ability to read from or write to an <type>[Image](Image.md)</type>.

## Constants

| Name | Value |
|:-----|:------|
`WRITABLE_NO` | 0
`WRITABLE_YES` | 1
`WRITABLE_NEEDS_CONVERSION` | 2

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">id</prop> | <type>string</type> | The format id
<prop class="ro">name</prop> | <type>string</type> | The name of the image format
<prop class="ro">extension</prop> | <type>string</type> | The file extension to use for images of this type

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Graphics.ImageFormat](../../Namespaces/Graphics.md#imageformat)</code>
* <code>[Graphics.AllImageFormats](../../Namespaces/Graphics.md#allimageformats)</code>
* <code>[Graphics.DetectImageFormat](../../Namespaces/Graphics.md#detectimageformat)</code>

## Functions

### Overview

#### General

<fdef>[IsThisFormat](#isthisformat)(<arg>data</arg>) -> <type>boolean</type></fdef>
<fdef>[LoadImage](#loadimage)(<arg>image</arg>, <arg>data</arg>, <arg>[index]</arg>) -> <type>boolean</type></fdef>
<fdef>[SaveImage](#saveimage)(<arg>image</arg>, <arg>dataOut</arg>, <arg>[palette]</arg>, <arg>[index]</arg>) -> <type>boolean</type></fdef>
<fdef>[CanWrite](#canwrite)(<arg>image</arg>) -> <type>number</type></fdef>
<fdef>[CanWritePixelFormat](#canwritepixelformat)(<arg>pixelFormat</arg>) -> <type>boolean</type></fdef>
<fdef>[ConvertWritable](#convertwritable)(<arg>image</arg>, <arg>convertOptions</arg>) -> <type>boolean</type></fdef>

---
### IsThisFormat

Checks if the given <arg>data</arg> is an image of this format.

#### Parameters

* <arg>data</arg> (<type>[DataBlock](../DataBlock.md)</type>): The image data to check

#### Returns

* <type>boolean</type>: `true` if the given <arg>data</arg> is an image of this format

---
### LoadImage

Loads <arg>data</arg> of this format into <arg>image</arg>.

#### Parameters

* <arg>image</arg> (<type>[Image](Image.md)</type>): The target image to load the data into
* <arg>data</arg> (<type>[DataBlock](../DataBlock.md)</type>): The data to load
* <arg>[index]</arg> (<type>number</type>): The index of the image in the data (for cases where the image format contains multiple images, eg. Duke3D ART). Default is `0`

#### Returns

* <type>boolean</type>: `true` on success

---
### SaveImage

Saves the given <arg>image</arg> to <arg>dataOut</arg> in this format.

#### Parameters

* <arg>image</arg> (<type>[Image](Image.md)</type>): The image to write
* <arg>dataOut</arg> (<type>[DataBlock](../DataBlock.md)</type>): The data block to write to
* <arg>[palette]</arg> (<type>[Palette](Palette.md)</type>): The palette to use if <arg>image</arg> is 8-bit indexed but does not contain its own internal palette. Default is `nil`
* <arg>[index]</arg> (<type>number</type>): The index to write the image to in the data (for cases where the image format contains multiple images, eg. Duke3D ART). Default is `0`

#### Returns

* <type>boolean</type>: `true` on success

---
### CanWrite

Checks if the given <arg>image</arg> can be written in this format.

#### Parameters

* <arg>image</arg> (<type>[Image](Image.md)</type>): The image to check

#### Returns

* <type>number</type>: Whether the given image can be saved as this format:
    * `WRITABLE_NO`: Can't be written
    * `WRITABLE_YES`: Can be written
    * `WRITABLE_NEEDS_CONVERSION`: Can't be written as-is, requires conversion to a supported pixel format first (see <func>[ConvertWritable](#convertwritable)</func>)

---
### CanWritePixelFormat

#### Parameters

* <arg>pixelFormat</arg> (<type>number</type>): The image pixel format to check (see <type>[Image](Image.md#constants)</type>`.PIXELFORMAT_` constants)

#### Returns

* <type>boolean</type>: `true` if this image format supports the given <arg>pixelFormat</arg>

---
### ConvertWritable

Converts the given <arg>image</arg> so that it can be written in this format.

#### Parameters

* <arg>image</arg> (<type>[Image](Image.md)</type>): The image to convert
* <arg>convertOptions</arg> (<type>[ImageConvertOptions](ImageConvertOptions.md)</type>): Options for converting the image to this format

#### Returns

* <type>boolean</type>: `true` if the image was converted successfully and can now be written in this format
