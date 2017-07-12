!!! note
    description here

## Properties

| Name | Type | Description |
|:-----|:-----|:------------|
`name` | `string` | Thing type name
`group` | `string` | Thing type group, can be a path for nested groups eg. `Monsters/Stealth`
`radius` | `number` | Thing type radius in map units
`height` | `number` | Thing type height in map units
`scaleY` | `number` | X scaling factor for the sprite
`scaleX` | `number` | Y scaling factor for the sprite
`angled` | `boolean` | True if things of this type should show a direction arrow in 2d mode
`hanging` | `boolean` | True if things of this type should hang from the ceiling in 3d mode
`fullbright` | `boolean` | True if things of this type aren't affected by sector brightness
`decoration` | `boolean` | True if things of this type should still always be displayed in 3d mode (unless all things are hidden)
`solid` | `boolean` | True if things of this type block the player
`sprite` | `string` | The sprite to display for this thing type
`icon` | `string` | The icon to display for this thing type
`translation` | `string` | The palette translation to apply to the sprite for this thing type
`palette` | `string` | Palette for this thing type
