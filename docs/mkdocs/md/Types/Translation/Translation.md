<subhead>Type</subhead>
<header>Translation</header>

A colour translation definition. Can either be a built in translation or a custom translation made up of one or more <type>[TransRange](TransRange.md)</type>s.

See the [Translation](https://zdoom.org/wiki/Translation) page on the ZDoom wiki for more information.


## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">ranges</prop> | <type>[TransRange](TransRange.md)\[\]</type> | An array of all custom ranges in the translation
<prop class="ro">rangeCount</prop> | <type>integer</type> | The number of custom ranges in the translation
<prop class="rw">standardName</prop> | <type>string</type> | The name of the 'built in' translation to use (for TEXTURES). If this is set the <prop>ranges</prop> above will be ignored. See [TEXTURES](https://zdoom.org/wiki/TEXTURES) for information on the available options.
<prop class="rw">desatAmount</prop> | <type>integer</type> | The desaturation amount (`1` - `31`) if <prop>standardName</prop> is `desaturate`


## Constructors

<code><type>Translation</type>.<func>new</func>()</code>

Creates a new, empty translation.


## Functions

### Overview

#### General

<fdef>[AsText](#astext)() -> <type>string</type></fdef>
<fdef>[Clear](#clear)()</fdef>
<fdef>[Copy](#copy)(<arg>other</arg>)</fdef>
<fdef>[IsEmpty](#isempty)() -> <type>boolean</type></fdef>
<fdef>[Parse](#parse)(<arg>definition</arg>)</fdef>
<fdef>[Translate](#translate)(<arg>colour</arg>, <arg>[palette]</arg>) -> <type>[Colour](../Colour.md)</type></fdef>

#### Custom Ranges

<fdef>[Range](#range)(<arg>index</arg>) -> <type>[TransRange](TransRange.md)</type></fdef>
<fdef>[AddRange](#addrange)(<arg>definition</arg>) -> <type>[TransRange](TransRange.md)</type></fdef>
<fdef>[AddPaletteRange](#addpaletterange)(<arg>rangeStart</arg>, <arg>rangeEnd</arg>) -> <type>[TransRangePalette](TransRangePalette.md)</type></fdef>
<fdef>[AddColourRange](#addcolourrange)(<arg>rangeStart</arg>, <arg>rangeEnd</arg>) -> <type>[TransRangeColour](TransRangeColour.md)</type></fdef>
<fdef>[AddDesatRange](#adddesatrange)(<arg>rangeStart</arg>, <arg>rangeEnd</arg>) -> <type>[TransRangeDesat](TransRangeDesat.md)</type></fdef>
<fdef>[AddBlendRange](#addblendrange)(<arg>rangeStart</arg>, <arg>rangeEnd</arg>) -> <type>[TransRangeBlend](TransRangeBlend.md)</type></fdef>
<fdef>[AddTintRange](#addtintrange)(<arg>rangeStart</arg>, <arg>rangeEnd</arg>) -> <type>[TransRangeTint](TransRangeTint.md)</type></fdef>
<fdef>[AddSpecialRange](#addspecialrange)(<arg>rangeStart</arg>, <arg>rangeEnd</arg>) -> <type>[TransRangeSpecial](TransRangeSpecial.md)</type></fdef>
<fdef>[ReadTable](#readtable)(<arg>data</arg>)</fdef>
<fdef>[RemoveRange](#removerange)(<arg>index</arg>)</fdef>
<fdef>[SwapRanges](#swapranges)(<arg>index1</arg>, <arg>index2</arg>)</fdef>

---
### AsText

Gets the translation as a text string (in ZDoom format).

#### Returns

* <type>string</type>: A text representation of the translation (in ZDoom format)

#### Example

```lua
local translation = Translation.new()

-- Add palette range
local palRange = translation:AddPaletteRange(0, 10)
palRange.destStart = 100
palRange.destEnd = 110

-- Add colour range
local colRange = translation:AddColourRange(40, 50)
colRange.startColour = Colour.new(100, 0, 0)
colRange.endColour = Colour.new(255, 100, 100)

App.LogMessage(translation:AsText()) -- "0:10=100:110", "40:50=[100,0,0]:[255,100,100]"
```

---
### Clear

Clears all custom <prop>ranges</prop> in the translation as well as <prop>standardName</prop> if it is set.

---
### Copy

Copies all translation info from another <type>Translation</type>

#### Parameters

* <arg>other</arg> (<type>Translation</type>): The translation to copy from

---
### IsEmpty

Returns `true` if the translation is 'empty' (ie. the <prop>ranges</prop> and <prop>standardName</prop> properties are both empty).

#### Returns

* <type>boolean</type>: `true` if the translation is 'empty'

---
### Parse

Clears the current translation and loads new translation information from the given text <arg>definition</arg>.

#### Parameters

* <arg>definition</arg> (<type>string</type>) : A full (comma-separated) [translation](https://zdoom.org/wiki/Translation) definition in text format

---
### Translate

Applies the translation to a colour.

#### Parameters

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to apply the translation to
* <arg>[palette]</arg> (<type>[Palette](../Graphics/Palette.md)</type>): The palette to use for the translation. Default is `nil`, which means the currently selected global palette will be used

#### Returns

* <type>[Colour](../Colour.md)</type>: The translated colour

---
### Range

Gets a single part (<type>[TransRange](TransRange.md)</type>) of the translation.

#### Parameters

* <arg>index</arg> (<type>integer</type>) : The index of the range to get

#### Returns

* <type>[TransRange](TransRange.md)</type>: The range at the specified <arg>index</arg> in this translation, or `nil` if <arg>index</arg> was out of bounds

---
### AddRange

Adds a new custom translation range to the translation, parsed from the given text <arg>definition</arg>.

#### Parameters

* <arg>definition</arg> (<type>string</type>): A single [translation](https://zdoom.org/wiki/Translation) range definition in text format

#### Returns

* <type>[TransRange](TransRange.md)</type>: The created translation range, or `nil` if the given <arg>definition</arg> was invalid

---
### AddPaletteRange

Adds a new custom translation range of type <type>[TransRangePalette](TransRangePalette.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

#### Parameters

* <arg>rangeStart</arg> (<type>integer</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>integer</type>): The last palette index of the range (`0` - `255`)

#### Returns

* <type>[TransRangePalette](TransRangePalette.md)</type>: The new range that was added

#### Example

```lua
local translation = Translation.new()
local palRange = translation:AddPaletteRange(0, 20)
palRange.destStart = 50
palRange.destEnd = 60
App.LogMessage(translation:AsText()) -- '0:20=50:60'
```

---
### AddColourRange

Adds a new custom translation range of type <type>[TransRangeColour](TransRangeColour.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

#### Parameters

* <arg>rangeStart</arg> (<type>integer</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>integer</type>): The last palette index of the range (`0` - `255`)

#### Returns

* <type>[TransRangeColour](TransRangeColour.md)</type>: The new range that was added

---
### AddDesatRange

Adds a new custom translation range of type <type>[TransRangeDesat](TransRangeDesat.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

#### Parameters

* <arg>rangeStart</arg> (<type>integer</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>integer</type>): The last palette index of the range (`0` - `255`)

#### Returns

* <type>[TransRangeDesat](TransRangeDesat.md)</type>: The new range that was added

---
### AddBlendRange

Adds a new custom translation range of type <type>[TransRangeBlend](TransRangeBlend.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

#### Parameters

* <arg>rangeStart</arg> (<type>integer</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>integer</type>): The last palette index of the range (`0` - `255`)

#### Returns

* <type>[TransRangeBlend](TransRangeBlend.md)</type>: The new range that was added

---
### AddTintRange

Adds a new custom translation range of type <type>[TransRangeTint](TransRangeTint.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

#### Parameters

* <arg>rangeStart</arg> (<type>integer</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>integer</type>): The last palette index of the range (`0` - `255`)

#### Returns

* <type>[TransRangeTint](TransRangeTint.md)</type>: The new range that was added

---
### AddSpecialRange

Adds a new custom translation range of type <type>[TransRangeSpecial](TransRangeSpecial.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

#### Parameters

* <arg>rangeStart</arg> (<type>integer</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>integer</type>): The last palette index of the range (`0` - `255`)

#### Returns

* <type>[TransRangeSpecial](TransRangeSpecial.md)</type>: The new range that was added

---
### ReadTable

Adds translation ranges from the given translation table <arg>data</arg>.

#### Parameters

* <arg>data</arg> (<type>string</type>): The translation table binary data to read

---
### RemoveRange

Removes the custom translation range at <arg>index</arg> in <prop>ranges</prop>

#### Parameters

* <arg>index</arg> (<type>integer</type>): The index of the range to remove

---
### SwapRanges

Swaps the custom translation ranges at <arg>index1</arg> and <arg>index2</arg> in <prop>ranges</prop>

#### Parameters

* <arg>index1</arg> (<type>integer</type>): The index of the first range to swap
* <arg>index2</arg> (<type>integer</type>): The index of the second range to swap
