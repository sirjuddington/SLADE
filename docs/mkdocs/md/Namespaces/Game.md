<article-head>Game</article-head>

The `Game` scripting namespace contains functions related to the currently loaded game configuration in SLADE.

## Functions

### ThingType

```lua
function Game.ThingType(type)
```

<listhead>Parameters</listhead>

  * <type>number</type> <arg>type</arg>: The editor number (DoomEdNum) of the type

<listhead>Returns</listhead>

* <type>[ThingType](../Types/ThingType.md)</type>: Thing type info for the given editor number
