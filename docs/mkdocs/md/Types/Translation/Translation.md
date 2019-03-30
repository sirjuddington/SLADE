<article-head>Translation</article-head>

A colour translation definition. Can either be a built in translation or a custom translation made up of one or more <type>[TransRange](TransRange.md)</type>s.

See the [Translation](https://zdoom.org/wiki/Translation) page on the ZDoom wiki for more information.


## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">ranges</prop> | <type>[TransRange](TransRange.md)\[\]</type> | An array of all custom ranges in the translation
<prop class="ro">rangeCount</prop> | <type>number</type> | The number of custom ranges in the translation
<prop class="rw">standardName</prop> | <type>string</type> | The name of the 'built in' translation to use (for TEXTURES). If this is set the <prop>ranges</prop> above will be ignored. See [TEXTURES](https://zdoom.org/wiki/TEXTURES) for information on the available options.
<prop class="rw">desatAmount</prop> | <type>number</type> | The desaturation amount (`1` - `31`) if <prop>standardName</prop> is `desaturate`


## Constructors

<fdef>function <type>Translation</type>.<func>new</func>()</fdef>

Creates a new, empty translation.


## Functions - General

### AsText

<fdef>function <type>Translation</type>.<func>AsText</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>string</type>: A text representation of the translation (in ZDoom format)

**Example**

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

<fdef>function <type>Translation</type>.<func>Clear</func>(<arg>*self*</arg>)</fdef>

Clears all custom <prop>ranges</prop> in the translation as well as <prop>standardName</prop> if it is set.

---
### Copy

<fdef>function <type>Translation</type>.<func>Copy</func>(<arg>*self*</arg>, <arg>other</arg>)</fdef>

Copies all translation info from another <type>Translation</type>

<listhead>Parameters</listhead>

* <arg>other</arg> (<type>Translation</type>): The translation to copy from

---
### IsEmpty

<fdef>function <type>Translation</type>.<func>IsEmpty</func>(<arg>*self*</arg>)</fdef>

<listhead>Returns</listhead>

* <type>boolean</type>: `true` if the <prop>ranges</prop> and <prop>standardName</prop> properties are both empty (ie. the translation will do nothing)

---
### Parse

<fdef>function <type>Translation</type>.<func>Parse</func>(<arg>*self*</arg>, <arg>definition</arg>)</fdef>

Clears the current translation and loads new translation information from the given text <arg>definition</arg>.

<listhead>Parameters</listhead>

* <arg>definition</arg> (<type>string</type>) : A full (comma-separated) [translation](https://zdoom.org/wiki/Translation) definition in text format

---
### Translate

<fdef>function <type>Translation</type>.<func>Translate</func>(<arg>self</arg>, <arg>colour</arg>, <arg>palette</arg>)</fdef>

Applies the translation to a colour

<listhead>Parameters</listhead>

* <arg>colour</arg> (<type>[Colour](../Colour.md)</type>): The colour to apply the translation to
* <arg>[palette]</arg> (<type>[Palette](../Graphics/Palette.md)</type>, default `nil`): The palette to use for the translation. If `nil`, the currently selected global palette will be used

<listhead>Returns</listhead>

* <type>[Colour](../Colour.md)</type>: The translated colour


## Functions - Custom Ranges

### Range

<fdef>function <type>Translation</type>.<func>Range</func>(<arg>*self*</arg>, <arg>index</arg>)</fdef>

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>) : The index of the range to get

<listhead>Returns</listhead>

* <type>[TransRange](TransRange.md)</type> : The range at the specified <arg>index</arg> in this translation, or `nil` if <arg>index</arg> was out of bounds

---
### AddRange

<fdef>function <type>Translation</type>.<func>AddRange</func>(<arg>*self*</arg>, <arg>definition</arg>)</fdef>

Adds a new custom translation range to the translation, parsed from the given text <arg>definition</arg>.

<listhead>Parameters</listhead>

* <arg>definition</arg> (<type>string</type>): A single [translation](https://zdoom.org/wiki/Translation) range definition in text format

<listhead>Returns</listhead>

* <type>[TransRange](TransRange.md)</type>: The created translation range, or `nil` if the given <arg>definition</arg> was invalid

---
### AddPaletteRange

<fdef>function <type>Translation</type>.<func>AddPaletteRange</func>(<arg>*self*</arg>, <arg>rangeStart</arg>, <arg>rangeEnd</arg>)</fdef>

Adds a new custom translation range of type <type>[TransRangePalette](TransRangePalette.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

<listhead>Parameters</listhead>

* <arg>rangeStart</arg> (<type>number</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>number</type>): The last palette index of the range (`0` - `255`)

<listhead>Returns</listhead>

* <type>[TransRangePalette](TransRangePalette.md)</type>: The new range that was added

**Example**

```lua
local translation = Translation.new()
local palRange = translation:AddPaletteRange(0, 20)
palRange.destStart = 50
palRange.destEnd = 60
App.LogMessage(translation:AsText()) -- '0:20=50:60'
```

---
### AddColourRange

<fdef>function <type>Translation</type>.<func>AddColourRange</func>(<arg>*self*</arg>, <arg>rangeStart</arg>, <arg>rangeEnd</arg>)</fdef>

Adds a new custom translation range of type <type>[TransRangeColour](TransRangeColour.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

<listhead>Parameters</listhead>

* <arg>rangeStart</arg> (<type>number</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>number</type>): The last palette index of the range (`0` - `255`)

<listhead>Returns</listhead>

* <type>[TransRangeColour](TransRangeColour.md)</type>: The new range that was added

---
### AddDesatRange

<fdef>function <type>Translation</type>.<func>AddDesatRange</func>(<arg>*self*</arg>, <arg>rangeStart</arg>, <arg>rangeEnd</arg>)</fdef>

Adds a new custom translation range of type <type>[TransRangeDesat](TransRangeDesat.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

<listhead>Parameters</listhead>

* <arg>rangeStart</arg> (<type>number</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>number</type>): The last palette index of the range (`0` - `255`)

<listhead>Returns</listhead>

* <type>[TransRangeDesat](TransRangeDesat.md)</type>: The new range that was added

---
### AddBlendRange

<fdef>function <type>Translation</type>.<func>AddBlendRange</func>(<arg>*self*</arg>, <arg>rangeStart</arg>, <arg>rangeEnd</arg>)</fdef>

Adds a new custom translation range of type <type>[TransRangeBlend](TransRangeBlend.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

<listhead>Parameters</listhead>

* <arg>rangeStart</arg> (<type>number</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>number</type>): The last palette index of the range (`0` - `255`)

<listhead>Returns</listhead>

* <type>[TransRangeBlend](TransRangeBlend.md)</type>: The new range that was added

---
### AddTintRange

<fdef>function <type>Translation</type>.<func>AddTintRange</func>(<arg>*self*</arg>, <arg>rangeStart</arg>, <arg>rangeEnd</arg>)</fdef>

Adds a new custom translation range of type <type>[TransRangeTint](TransRangeTint.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

<listhead>Parameters</listhead>

* <arg>rangeStart</arg> (<type>number</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>number</type>): The last palette index of the range (`0` - `255`)

<listhead>Returns</listhead>

* <type>[TransRangeTint](TransRangeTint.md)</type>: The new range that was added

---
### AddSpecialRange

<fdef>function <type>Translation</type>.<func>AddSpecialRange</func>(<arg>*self*</arg>, <arg>rangeStart</arg>, <arg>rangeEnd</arg>)</fdef>

Adds a new custom translation range of type <type>[TransRangeSpecial](TransRangeSpecial.md)</type> to the translation, from <arg>rangeStart</arg> to <arg>rangeEnd</arg>.

<listhead>Parameters</listhead>

* <arg>rangeStart</arg> (<type>number</type>): The first palette index of the range (`0` - `255`)
* <arg>rangeEnd</arg> (<type>number</type>): The last palette index of the range (`0` - `255`)

<listhead>Returns</listhead>

* <type>[TransRangeSpecial](TransRangeSpecial.md)</type>: The new range that was added

---
### ReadTable

<fdef>function <type>Translation</type>.<func>ReadTable</func>(<arg>*self*</arg>, <arg>data</arg>)</fdef>

Adds translation ranges from the given translation table <arg>data</arg>.

<listhead>Parameters</listhead>

* <arg>data</arg> (<type>string</type>): The translation table binary data to read

---
### RemoveRange

<fdef>function <type>Translation</type>.<func>RemoveRange</func>(<arg>*self*</arg>, <arg>index</arg>)</fdef>

Removes the custom translation range at <arg>index</arg> in <prop>ranges</prop>

<listhead>Parameters</listhead>

* <arg>index</arg> (<type>number</type>): The index of the range to remove

---
### SwapRanges

<fdef>function <type>Translation</type>.<func>SwapRanges</func>(<arg>*self*</arg>, <arg>index1</arg>, <arg>index2</arg>)</fdef>

Swaps the custom translation ranges at <arg>index1</arg> and <arg>index2</arg> in <prop>ranges</prop>

<listhead>Parameters</listhead>

* <arg>index1</arg> (<type>number</type>): The index of the first range to swap
* <arg>index2</arg> (<type>number</type>): The index of the second range to swap
