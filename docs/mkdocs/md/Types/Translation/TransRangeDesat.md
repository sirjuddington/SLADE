<article-head>TransRangeDesat</article-head>

A custom translation range that maps to a desaturated colour gradient. See [Translation](https://zdoom.org/wiki/Translation) for more info.

### Inherits <type>[TransRange](TransRange.md)</type>  
All properties and functions of <type>[TransRange](TransRange.md)</type> can be used in addition to those below.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>startR</prop> | <type>number</type> | The red component of the starting colour (`0.0` - `2.0`)
<prop>startG</prop> | <type>number</type> | The green component of the starting colour (`0.0` - `2.0`)
<prop>startB</prop> | <type>number</type> | The blue component of the starting colour (`0.0` - `2.0`)
<prop>endR</prop> | <type>number</type> | The red component of the ending colour (`0.0` - `2.0`)
<prop>endG</prop> | <type>number</type> | The green component of the ending colour (`0.0` - `2.0`)
<prop>endB</prop> | <type>number</type> | The blue component of the ending colour (`0.0` - `2.0`)

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

<listhead>See:</listhead>

* <code>[Translation.AddDesatRange](Translation.md#adddesatrange)</code>

## Functions

### SetStartRGB

<fdef>function <type>TransRangeDesat</type>.<func>SetStartRGB</func>(<arg>*self*</arg>, <arg>r</arg>, <arg>g</arg>, <arg>b</arg>)</fdef>

Sets the starting colour of the desaturated gradient.

<listhead>Parameters</listhead>

* <arg>r</arg> (<type>number</type>): Red component (`0.0` - `2.0`)
* <arg>g</arg> (<type>number</type>): Green component (`0.0` - `2.0`)
* <arg>b</arg> (<type>number</type>): Blue component (`0.0` - `2.0`)

---
### SetEndRGB

<fdef>function <type>TransRangeDesat</type>.<func>SetEndRGB</func>(<arg>*self*</arg>, <arg>r</arg>, <arg>g</arg>, <arg>b</arg>)</fdef>

Sets the ending colour of the desaturated gradient.

<listhead>Parameters</listhead>

* <arg>r</arg> (<type>number</type>): Red component (`0.0` - `2.0`)
* <arg>g</arg> (<type>number</type>): Green component (`0.0` - `2.0`)
* <arg>b</arg> (<type>number</type>): Blue component (`0.0` - `2.0`)
