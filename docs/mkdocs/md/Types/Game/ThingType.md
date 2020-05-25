Represents a thing type definition from the current game configuration (or parsed from DECORATE/ZScript etc.)

## Properties

| Property | Type | Description |
|:---------|:-----|:------------|
<prop class="ro">name</prop> | <type>string</type> | Thing type name
<prop class="ro">group</prop> | <type>string</type> | Thing type group, can be a path for nested groups eg. `Monsters/Stealth`
<prop class="ro">radius</prop> | <type>number</type> | Thing type radius in map units
<prop class="ro">height</prop> | <type>number</type> | Thing type height in map units
<prop class="ro">scaleY</prop> | <type>number</type> | X scaling factor for the sprite
<prop class="ro">scaleX</prop> | <type>number</type> | Y scaling factor for the sprite
<prop class="ro">angled</prop> | <type>boolean</type> | True if things of this type should show a direction arrow in 2d mode
<prop class="ro">hanging</prop> | <type>boolean</type> | True if things of this type should hang from the ceiling in 3d mode
<prop class="ro">fullbright</prop> | <type>boolean</type> | True if things of this type aren't affected by sector brightness
<prop class="ro">decoration</prop> | <type>boolean</type> | True if things of this type should still always be displayed in 3d mode (unless all things are hidden)
<prop class="ro">solid</prop> | <type>boolean</type> | True if things of this type block the player
<prop class="ro">sprite</prop> | <type>string</type> | The sprite to display for this thing type
<prop class="ro">icon</prop> | <type>string</type> | The icon to display for this thing type
<prop class="ro">translation</prop> | <type>string</type> | The palette translation to apply to the sprite for this thing type
<prop class="ro">palette</prop> | <type>string</type> | Palette for this thing type

## Constructors

!!! attention "No Constructors"
    This type can not be created directly in scripts.

**See:**

* <code>[Game.ThingType](../../Namespaces/Game.md#thingtype)</code>
