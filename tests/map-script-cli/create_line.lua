-- The map-script runner invokes the returned function with the loaded map.
-- Point.new is used deliberately: generator libraries use it to create map
-- geometry without a UI map editor context.
return function(map)
    local line = map:CreateLine(Point.new(-64, 0), Point.new(64, 0))
    assert(line ~= nil, "CreateLine failed")
end
