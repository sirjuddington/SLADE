The <type>ImageDrawOptions</type> type contains colour blending options for use in <type>[Image](Image.md)</type>'s drawing functions.

**See:**

* <code>[Image.DrawPixel](Image.md#drawpixel)</code>
* <code>[Image.DrawImage](Image.md#drawimage)</code>

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="rw">blend</prop> | <type>number</type> | The blending mode to use (see `BLEND_` constants in the <code>[Graphics](../../Namespaces/Graphics.md#constants)</code> namespace)
<prop class="rw">alpha</prop> | <type>number</type> | The alpha value to draw with (`0.0` - `1.0`)
<prop class="rw">keepSourceAlpha</prop> | <type>boolean</type> | If `true`, the alpha value(s) of the source pixel(s) is respected when drawing, otherwise they will be ignored (ie. considered to be all fully opaque)

## Constructors

<code><type>ImageDrawOptions</type>.<func>new</func>()</code>

Creates a new <type>ImageDrawOptions</type> with the following default properties:

* <prop>blend</prop>: `Graphics.BLEND_NORMAL`
* <prop>alpha</prop>: `1.0`
* <prop>keepSourceAlpha</prop>: `true`
