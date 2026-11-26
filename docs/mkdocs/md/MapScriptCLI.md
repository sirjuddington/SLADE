<header>Map Script CLI</header>

`slade-map-script` runs a Lua map script without opening the SLADE user interface. It is intended for repeatable map-generation and transformation workflows, and is the validation path for an external script library.

## Build and run

Configure with Lua and the runner enabled (both are enabled by default), then build the explicit target:

```sh
cmake -S . -B build
cmake --build build --target slade-map-script
```

Run it with an input archive, a map name, a Lua file, and a distinct output WAD:

```sh
slade-map-script \
  --input path/to/source.wad \
  --map MAP01 \
  --script path/to/generate.lua \
  --output path/to/generated.wad \
  --validate
```

The input archive is opened read-only and is never modified. The result is a standalone WAD containing the selected map; an existing output file may be replaced. `--validate` reopens and reads the generated WAD after it is written. It verifies serialisation and map readability, not gameplay or node-builder validity.

`--input`, `--map`, `--script`, and `--output` are required. Map-name matching is case-insensitive. The output must not resolve to the same path as the input. Exit status is `0` on success, `1` for an execution, I/O, script, or validation error, and `2` for invalid command-line arguments.

## Script contract

The Lua file must evaluate to a function. The runner calls that function once, passing the loaded [Map](Types/Map/Map.md):

```lua
return function(map)
    local line = map:CreateLine(Point.new(-64, 0), Point.new(64, 0))
    assert(line ~= nil, "could not create line")
end
```

Use `map` directly. The headless runner has no `App.MapEditor()` UI context, so scripts designed around selections, dialogs, or an open editor tab are not suitable unchanged. The `Map`, map-object, `Point`, math, string, table, and other non-I/O Lua APIs are available; the Lua `io` and `os` libraries remain disabled.

For a repeatable regression check, configure with `-DBUILD_MAP_SCRIPT_CLI_TESTS=ON` and run:

```sh
cmake --build build --target check-map-script-cli
```

This creates an empty UDMF test map, runs a script that creates one line and its two vertices, validates the output WAD, and checks the generated geometry.
