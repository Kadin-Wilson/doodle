width = 1000
height = 1000
background = BLUE

function border(segment_size)
    local w_segments = width / segment_size
    local h_segments = height / segment_size

    local corners = {
        point { 0, 0 },
        point { 0, height - segment_size },
        point { width - segment_size, 0 },
        point { width - segment_size, height - segment_size },
    }
    for _, origin in ipairs(corners) do
        rectangle { origin, segment_size, segment_size, WHITE }
    end

    local radius = segment_size / 2

    local origin = point { segment_size + radius, 0 }
    for _ = 1, w_segments - 2 do
        circle { origin, radius, WHITE, }
        circle { origin + point { 0, height }, radius, WHITE, }
        origin = origin + point { segment_size, 0 }
    end

    origin = point { 0, segment_size + radius }
    for _ = 1, h_segments - 2 do
        circle { origin, radius, WHITE, }
        circle { origin + point { width, 0 }, radius, WHITE, }
        origin = origin + point { 0, segment_size }
    end
end

function circle_circle(origin, offset, radius, count, color)
    local angle = math.pi * 2 / count

    for i = 0, count - 1 do
        circle { origin:polar_offset(angle * i, offset), radius, color }
    end
end

function star(origin, radius)
    local angle = -math.pi / 2
    for _ = 1, 5 do
        local nangle = angle + math.pi * 4 / 5
        line {
            origin:polar_offset(angle, radius),
            origin:polar_offset(nangle, radius),
            height / 100,
            RED,
        }
        angle = nangle
    end
end

border(height / 10)

rectangle { point { 10, 10 }, 20, 20, RED }

center = point { width / 2, height / 2 }
circle { center, height / 8, GREEN }
circle_circle(center, height / 4, height / 20, 10, BLACK)
circle_circle(center, height / 4, height / 40, 10, color { 255, 127 })

for i = 1, 12 do
    star(center:polar_offset(i * 2 * math.pi / 12, height / 2.5), height / 20)
end
