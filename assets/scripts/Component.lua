local Component = {}

Component.__index = Component

function Component:new(o)
	o = o or {}
	setmetatable(o, self)
	return o
end

return Component