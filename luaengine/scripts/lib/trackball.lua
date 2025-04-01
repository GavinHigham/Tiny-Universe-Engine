--Ported from trackball.c
local glla = require 'glla'
local vec3 = glla.vec3
local mat3 = glla.mat3
local amat4 = glla.amat4
local function clamp(val, min, max)
    if val < min then return min end
    if val > max then return max end
    return val
end

local function trackball(target, radius_or_eye)
    local radius, x, y, eye
    if type(radius_or_eye) == 'number' then
        radius = radius_or_eye
    else
        eye = radius_or_eye - target
        radius = vec3.mag(eye)
        x = math.asin(eye.x / radius)
        y = math.asin(eye.y / radius)
    end
    local t = {
        camera = amat4(mat3.identity(), eye or vec3(0, 0, radius) + target),
        target = target,
        rotation = vec3(x or 0, y or 0, 0),
        prev_rotation = vec3(0, 0, 0),
        speed = vec3(radius / 300.0, radius / 300.0, radius / 300.0),
        radius = radius,
        min_radius = radius / 500.0, --Randomly chosen, no special meaning.
        max_radius = math.huge,
        bounds = {top = math.huge, bottom = math.huge, left = math.huge, right = math.huge},
        mouse = {button = false, x = 0, y = 0, scroll = {x = 0, y = 0}}
    }

    local function update()
        local r = t.radius
        local x = clamp(t.rotation.x, -t.bounds.left, t.bounds.right)
        local y = clamp(t.rotation.y, -t.bounds.bottom, t.bounds.top)
        --A is a position on the horizontal ring around the target.
        local a = vec3(r * math.sin(x), 0, r * math.cos(x))
        local b = vec3.normalized(a) * r * math.cos(y)
        b.y = r * math.sin(y)
        b = b + t.target
        t.camera = amat4(mat3.lookat(b, t.target, vec3(0, 1, 0)), b)
    end

    function t.set_speed(horizontal, vertical, zoom)
        t.speed = vec3(horizontal, vertical, zoom)
    end

    function t.set_target(target)
        t.target = target
        --for now, just reset the camera
        t.camera.t = vec3(0, 0, t.radius) + t.target
        t.camera.a = mat3.lookat(t.camera.t, t.target, vec3(0, 1, 0))
    end

    function t.set_radius(r, min_r, max_r)
        t.radius = r
        t.min_radius = min_r
        t.max_radius = max_r
        t.camera.t = vec3.normalized(t.camera.t) * t.radius
    end

    function t.set_bounds(top, bottom, left, right)
        t.bounds.top = top
        t.bounds.bottom = bottom
        t.bounds.left = left
        t.bounds.right = right
        t.camera = amat4(mat3.identity(), vec3(0, 0, t.radius) + t.target)
    end

    function t.step(mouse_x, mouse_y, button, scroll_x, scroll_y)
        t.radius = clamp(t.radius + scroll_y * t.speed.z, t.min_radius, t.max_radius)

        if t.mouse.button then
            if button then
--DRAG CONTINUE
                t.rotation = t.prev_rotation + vec3(t.mouse.x - mouse_x, mouse_y - t.mouse.y, 0) * t.speed
            else
--DRAG END
                t.mouse.button = false
                t.prev_rotation = t.rotation
            end
        else
--DRAG START
            if button then
                t.mouse.button = true
                t.mouse.x = mouse_x
                t.mouse.y = mouse_y
                t.mouse.scroll.x = scroll_x
                t.mouse.scroll.y = scroll_y
                t.prev_rotation = t.rotation
            else
--NO DRAG
            end
        end

        if scroll_y ~= 0 or t.mouse.button then
            update()
            return 1
        end

        return 0
    end

    update()

    return t
end

return trackball