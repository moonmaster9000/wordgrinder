-- © 2022 David Given.
-- WordGrinder is licensed under the MIT open source license. See the COPYING
-- file in this distribution for the full text.

local table_insert = table.insert
local table_remove = table.remove
local string_char = string.char
local SetColour = wg.setcolour
local DefineColour = wg.definecolour

local colours = {
	bg           = {0.000, 0.000, 0.000},
	fg           = {1.000, 1.000, 1.000},
	desk_bg      = {0.135, 0.135, 0.135},
	paper_bg     = {0.200, 0.200, 0.200},
	statusbar_fg = {0.140, 0.220, 0.400},
	statusbar_bg = {0.800, 0.700, 0.200},
}

local styles = {
	normal =    { colours.fg,           colours.bg },
	desktop =   { colours.fg,           colours.desk_bg },
	body =      { colours.fg,           colours.paper_bg },
	statusbar = { colours.statusbar_fg, colours.statusbar_bg }, -- reversed
	message =   { colours.fg,           colours.bg },
}

-----------------------------------------------------------------------------
-- Addon registration. Create the default global settings.

do
	local function cb()
		GlobalSettings.palette = GlobalSettings.palette or {}
	end

	AddEventListener(Event.RegisterAddons, cb)
end

-----------------------------------------------------------------------------
-- Actually sets a style for drawing.

function SetStyle(name)
	local sp = styles[name]
	if not sp then
		sp = styles.normal
	end
	SetColour(sp[1].id, sp[2].id)
end

-----------------------------------------------------------------------------
-- Programs the colours into the backend.

function UpdateColours()
	local id = 0
	for name, c in pairs(colours) do
		c.id = id
		id = id + 1
		DefineColour(c.id, c[1], c[2], c[3])
	end
end

AddEventListener(Event.ScreenInitialised, UpdateColours)

