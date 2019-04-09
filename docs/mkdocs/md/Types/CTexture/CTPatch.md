<article-head>CTPatch</article-head>

A single patch in a composite texture. See [TEXTURE1 and TEXTURE2](https://doomwiki.org/wiki/TEXTURE1_and_TEXTURE2) for more information.

### Derived Types

The following types inherit all <type>CTPatch</type> properties and functions:

* <type>[CTPatchEx](CTPatchEx.md)</type>

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">name</prop> | <type>string</type> | The name of the patch
<prop class="rw">offsetX</prop> | <type>number</type> | The horizontal offset of the patch (from the left of the texture)
<prop class="rw">offsetY</prop> | <type>number</type> | The vertical offset of the patch (from the top of the texture)

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

<listhead>See:</listhead>

* <code>[CTexture.AddPatch](CTexture.md#addpatch)</code>

## Functions

### PatchEntry

<fdef>function <type>CTPatch</type>.<func>PatchEntry</func>(<arg>*self*</arg>, <arg>parent</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>parent</arg> (<type>[Archive](../Archive/Archive.md)</type>): The parent archive (can be `nil`)

<listhead>Returns</listhead>

* <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type>: The associated <type>[ArchiveEntry](../Archive/ArchiveEntry.md)</type> for the patch, or `nil` if none found

#### Notes

Entries in the given <arg>parent</arg> archive will be prioritised when searching if it is not `nil`.

---
### AsExtended

<fdef>function <type>CTPatch</type>.<func>AsExtended</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>[CTPatchEx](CTPatchEx.md)</type>: The patch as a <type>[CTPatchEx](CTPatchEx.md)</type>, or `nil` if it is not that type
