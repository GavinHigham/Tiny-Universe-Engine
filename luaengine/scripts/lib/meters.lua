-- meter.lua

local min = math.min
local max = math.max
local function fclamp(value, lower, upper)
	return min(max(value, lower), upper)
end

local function meters()
	local default_label = "%s: %.2f"

	local meters = {
		meters = {},
		screen_width = 0,
		screen_height = 0,
		renderer = nil,
		total_label_chars = 0,
		state = 'click ended',
		clicked_meter = nil
	}

	local function find_enclosing(x, y)
		for name, m in pairs(meters.meters) do
			if m.check_enclosing(x, y) then
				return m
			end
		end
		return nil
	end

	function meters.add(name, width, height, min, value, max)
		if meters.meters[name] then
			error('Meter already exists', 2)
		end

		local m = {
			name = name,
			fmt = default_label,
			x = 0, --CHANGE THIS
			y = 0, --CHANGE THIS
			style = {
				width = width,
				height = height,
				padding = 3.0
			},
			min = min,
			max = max,
			value = value,
			snap_increment = 1.0,
			target = nil,
			callback = nil
		}

		function m.label(fmt)
			m.fmt = fmt
		end

		function m.style(fill_color, border_color, font_color, padding, flags)
			m.style.fill_color = fill_color
			m.style.border_color = border_color
			m.style.font_color = font_color
			m.style.padding = padding
			m.style.flags = flags
		end

		function m.snap_increment(snap_increment)
			m.snap_increment = snap_increment
		end

		function m.always_snap(always_snap)
			m.always_snap = always_snap
		end

		function m.value()
			return m.value
		end

		function m.set(value)
			m.value = value
			if m.target then
				m.target = m.value
			end
			if m.callback then
				m.callback(m.name, meters.state, value)
			end
		end

		function m.raw_set(value)
			m.value = value
		end

		function m.check_enclosing(x, y)
			return x >= m.x and y >= m.y and x <= (m.x + m.style.width) and y <= (m.y + m.style.height)
		end

		meters.meters[name] = m
		meters.total_label_chars = meters.total_label_chars + #name
		return m
	end

	function meters.delete(name)
		if not meters.meters[name] then
			return nil, "Meter not found"
		end
		meters.meters[name] = nil
		return 0
	end

	--[[
	--ChatGPT did some weird stuff to my functions...
	function meters.mouse(x, y, mouse_down)
		local enclosing = find_enclosing(x, y)
		if enclosing then
			if mouse_down then
				meters.clicked_meter = enclosing
				return 1
			else
				meters.clicked_meter = nil
			end
		end
		return 0
	end

	function meters.mouse_relative(x, y, mouse_down, shift_down, ctrl_down)
		local action = mouse(x, y, mouse_down)
		if action == 1 and meters.clicked_meter then
			local m = meters.meters[meters.clicked_meter]
			if mouse_down then
				local increment = m.snap_increment
				if shift_down then
					increment = increment * 0.1
				end
				if ctrl_down or m.always_snap then
					m.value = math.floor((x - m.x) / increment + 0.5) * increment
				else
					m.value = x - m.x
				end
				m.set(m.value)
				return 2
			end
		end
		return 0
	end
	]]

	function meters.mouse(x, y, mouse_down, shift_down, ctrl_down)
		if meters.state == 'METER_CLICK_STARTED' or meters.state == 'METER_DRAGGED' then
			local m = meters.clicked_meter
			if not m then
				--Got into a weird state, correct it.
				meters.state = 'METER_CLICK_ENDED'
				meters.clicked_meter = nil
				return 0
			end

			if mouse_down then
				meters.state = 'METER_DRAGGED'
			else
				meters.state = 'METER_CLICK_ENDED'
			end

			local left_edge = m.x + m.style.padding
			local right_edge = m.x + m.style.width - 2 * m.style.padding
			local value_range = m.max - m.min
			local fill_width = right_edge - left_edge
			local clicked_value = (x - left_edge) * value_range / fill_width + m.min
			ctrl_down = ctrl_down or m.always_snap

			if shift_down and not ctrl_down then
				--When shift is held, right-edge of filled region slowly drifts towards mouse cursor,
				--with velocity proportional to cursor distance from right-edge of filled region
				m.set(fclamp(0.999 * m.value + 0.001 * clicked_value, m.min, m.max))
			else
				--With no modifiers, clicking the meter just directly sets the value.
				--Ex. Clicking 25% between the left and right edge gives a value 25% between the configured min and max value.
				local value = fclamp(clicked_value, m.min, m.max)
				--If the ctrl modifier is held, snap to the "snap increment", which defaults to 1.0.
				if ctrl_down then
					local increment = m.snap_increment
					--If both ctrl and shift are held, snap to 1/10th the "snap increment".
					if shift_down then
						increment = increment / 10.0
					end
					value = value // increment * increment
				end
				m.set(value)
			end
			--print(("Meter dragged, set to %f\n"):format(meter_value(m)))
			if 'METER_DRAGGED' then
				return 2
			end
		elseif meters.state == 'METER_CLICK_ENDED' then
			if mouse_down then
				local m = find_enclosing(x, y)
				if not m then
					meters.state = 'METER_CLICK_STARTED_OUTSIDE'
					return 0
				end

				meters.state = 'METER_CLICK_STARTED'
				meters.clicked_meter = m
				meters.clicked_x = x
				meters.clicked_y = y

				--Pass a different value if I want absolute instead of relative sliding.
				m.set(m.value)
				return 1
			end
		elseif meters.state == 'METER_CLICK_STARTED_OUTSIDE' then
			if not mouse_down then
				meters.state = 'METER_CLICK_ENDED'
			end
		end
		return 0
	end

	function meters.draw_all()
		-- Drawing logic here (this is context dependent and will vary based on the rendering system)
	end

	function meters.init(screen_width, screen_height, renderer)
		meters.screen_width = screen_width
		meters.screen_height = screen_height
		meters.renderer = renderer
		meters.meters = {}
		meters.total_label_chars = 0
	end

	function meters.deinit()
		meters.meters = nil
	end

	function meters.resize_screen(screen_width, screen_height)
		meters.screen_width = screen_width
		meters.screen_height = screen_height
	end

	return setmetatable(meters, {
		__call = function(_, name)
			return meters.meters[name]
		end
	})
end

return meters
