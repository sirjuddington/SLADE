!!! note
    description here

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop>name</prop> | <type>string</type> | Thing type name
<prop>group</prop> | <type>string</type> | Thing type group, can be a path for nested groups eg. `Monsters/Stealth`
<prop>radius</prop> | <type>number</type> | Thing type radius in map units
<prop>height</prop> | <type>number</type> | Thing type height in map units
<prop>scaleY</prop> | <type>number</type> | X scaling factor for the sprite
<prop>scaleX</prop> | <type>number</type> | Y scaling factor for the sprite
<prop>angled</prop> | <type>boolean</type> | True if things of this type should show a direction arrow in 2d mode
<prop>hanging</prop> | <type>boolean</type> | True if things of this type should hang from the ceiling in 3d mode
<prop>fullbright</prop> | <type>boolean</type> | True if things of this type aren't affected by sector brightness
<prop>decoration</prop> | <type>boolean</type> | True if things of this type should still always be displayed in 3d mode (unless all things are hidden)
<prop>solid</prop> | <type>boolean</type> | True if things of this type block the player
<prop>sprite</prop> | <type>string</type> | The sprite to display for this thing type
<prop>icon</prop> | <type>string</type> | The icon to display for this thing type
<prop>translation</prop> | <type>string</type> | The palette translation to apply to the sprite for this thing type
<prop>palette</prop> | <type>string</type> | Palette for this thing type
