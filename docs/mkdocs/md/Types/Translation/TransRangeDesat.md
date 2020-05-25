A custom translation range that maps to a desaturated colour gradient. See [Translation](https://zdoom.org/wiki/Translation) for more info.

### Inherits <type>[TransRange](TransRange.md)</type>  
All properties and functions of <type>[TransRange](TransRange.md)</type> can be used in addition to those below.

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>startR</prop> | <type>float</type> | The red component of the starting colour (`0.0` - `2.0`)
<prop>startG</prop> | <type>float</type> | The green component of the starting colour (`0.0` - `2.0`)
<prop>startB</prop> | <type>float</type> | The blue component of the starting colour (`0.0` - `2.0`)
<prop>endR</prop> | <type>float</type> | The red component of the ending colour (`0.0` - `2.0`)
<prop>endG</prop> | <type>float</type> | The green component of the ending colour (`0.0` - `2.0`)
<prop>endB</prop> | <type>float</type> | The blue component of the ending colour (`0.0` - `2.0`)

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Translation.AddDesatRange](Translation.md#adddesatrange)</code>

## Functions

### Overview

<fdef>[SetStartRGB](#setstartrgb)(<arg>r</arg>, <arg>g</arg>, <arg>b</arg>)</fdef>
<fdef>[SetEndRGB](#setendrgb)(<arg>r</arg>, <arg>g</arg>, <arg>b</arg>)</fdef>

---
### SetStartRGB

Sets the starting colour of the desaturated gradient.

#### Parameters

* <arg>r</arg> (<type>float</type>): Red component (`0.0` - `2.0`)
* <arg>g</arg> (<type>float</type>): Green component (`0.0` - `2.0`)
* <arg>b</arg> (<type>float</type>): Blue component (`0.0` - `2.0`)

---
### SetEndRGB

Sets the ending colour of the desaturated gradient.

#### Parameters

* <arg>r</arg> (<type>float</type>): Red component (`0.0` - `2.0`)
* <arg>g</arg> (<type>float</type>): Green component (`0.0` - `2.0`)
* <arg>b</arg> (<type>float</type>): Blue component (`0.0` - `2.0`)
