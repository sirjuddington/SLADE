<header>SLADE Scripting Documentation</header>

Here is the documentation for the SLADE Lua scripting system. To start with it is probably best to take a look at the examples to get an idea of how things work, since currently there aren't any real tutorials or basic 'how-to' articles (something like that may come later).

## Documentation Conventions

### Type Properties

Type properties can either be read-only or read+write. Trying to modify a property that is read only will result in a script error.

In this documentation, a symbol is displayed to the left of each property indicating whether the property can be modified:

* <prop class="ro">property</prop>: Read-only, can not be modified
* <prop class="rw">property</prop>: Can be modified

## Lua Language

Since the SLADE scripting functionality uses Lua as its language, it is recommended to read up on Lua ([Programming in Lua](https://www.lua.org/pil/contents.html)).

Below are some general notes about some things that may trip you up if you're used to other languages such as C or ZScript.

### Array and List Indices

Lua differs from most other languages in the way array indices work - usually, the first element of an array is index `0`, but in Lua the first element will be index `1`.

SLADE scripting functions and types use the 'standard' (`0`-based) convention for things like index parameters. This means you may need to be careful when using an index from a Lua array in a SLADE scripting function.

Also note that functions that return an array will return a Lua array, which means it starts at index `1`.

```lua
local archive = Archives.OpenFile("c:/games/doom/somewad.wad")

-- The first element in entries is entries[1], because it is a Lua array
-- (entries[0] is invalid)
local entries = archive.entries

-- Will print "0" because entries in SLADE begin at index 0
App.LogMessage(entries[1].index)
```

### Type Member Functions

Any function that is a member of a type has `self` as the first parameter, which is how 'member functions' work in lua.

Lua provides the `:` shortcut to call member functions with an automatic `self` parameter:

```lua
local object = SomeType.new()

-- The following lines are equivalent
object:SomeFunction(param1, param2)
object.SomeFunction(object, param1, param2)
SomeType.SomeFunction(object, param1, param2)
```

It is generally recommended to use the `:` format in these cases, as it is shorter and less error-prone.

### Multiple Return Values

Lua allows functions to return multiple values, which is something some SLADE scripting functions take advantage of.

An example of this is <code>[Colour.AsHSL](md/Types/Colour.md#ashsl)</code>. The values returned can be retrieved as per the example below:

```lua
local red = Colour.new(255, 0, 0)
local h, s, l = red:AsHSL()

-- Prints "Red HSL: 0.0, 1.0, 0.5"
App.LogMessage(string.format("Red HSL: %1.1f, %1.1f, %1.1f", h, s, l))
```

## Notes

### Running Scripts

State is not presereved after running a script - eg. a global variable defined in a script will not be available next time a script is run.
