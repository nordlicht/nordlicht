-- nordlicht integration for mpv. You need mpv >= 0.3.6 to correctly support
-- lua scripting.

function init()
    is_on = false
    mode = 1 -- 1: single, 2: double
    -- size of the display:
    screen_width = mp.get_property("osd-width")

    -- size of the barcode:
    width = mp.get_property("osd-width")
    height = math.floor(width/30)
    width2 = width*5
    height2 = math.floor(width2/30)

    -- size of the progress marker:
    mh = math.floor(height/10)*2+1
    mw = mh*2-1

    -- styles:
    styles = "horizontal"
    styles2 = "slitscan"

    mp.register_event("start-file", new_file)
    mp.register_event("shutdown", shutdown)
    mp.add_key_binding("n", "nordlicht-toggle", toggle)

    -- yeah, I know. But it's flexible :-P
    os.execute("convert -depth 8 -size "..mw.."x"..mh.." xc:none -fill white -stroke black -strokewidth 0.5 +antialias -draw \"path 'M "..((mw-1)/2)..",0 L "..(mw-1)..","..(mh-1).." L 0,"..(mh-1).." Z'\" bgra:/tmp/arrow_up.bgra")
    os.execute("convert -depth 8 -size "..mw.."x"..mh.." bgra:/tmp/arrow_up.bgra -flip bgra:/tmp/arrow_down.bgra")

    new_file()
    -- wait for the convert commands to finish
    mp.add_timeout(0.5, on)
end

function shutdown()
    off()
    kill()
end

function kill()
    os.execute("killall -9 nordlicht")
end

function new_file()
    local was_on = is_on
    if was_on then
        off()
    end

    kill()

    local nordlicht_cmd = "nordlicht -s "..styles
    local nordlicht_cmd2 = "nordlicht -s "..styles2
    local path = mp.get_property("path")
    local cmd = "nice "..nordlicht_cmd.." \""..path.."\" -o /tmp/nordlicht.bgra -w "..width.." -h "..height.." &"
    os.execute(cmd)
    local cmd2 = "nice "..nordlicht_cmd2.." \""..path.."\" -o /tmp/nordlicht2.bgra -w "..width2.." -h "..height2.." &"
    os.execute(cmd2)

    if was_on then
        -- wait for the file to be opened and truncated
        mp.add_timeout(0.5, on)
    end
end

function update()
    local pos = mp.get_property("percent-pos")

    if pos ~= nil then
        mp.command("overlay_add 0 0 "..mh.." /tmp/nordlicht.bgra 0 bgra "..width.." "..height.." "..width*4)
        mp.command("overlay_add 1 "..(math.floor(pos/100*width)-(mw-1)/2).." "..(0).." /tmp/arrow_down.bgra 0 bgra "..mw.." "..mh.." "..mw*4)
        mp.command("overlay_add 2 "..(math.floor(pos/100*width)-(mw-1)/2).." "..(height+mh).." /tmp/arrow_up.bgra 0 bgra "..mw.." "..mh.." "..mw*4)
    end

    if mode == 2 then
        local offset = height+3*mh

        if pos ~= nil then
            mp.command("overlay_add 3 "..math.floor(screen_width/2-width2*pos/100).." "..(mh+offset).." /tmp/nordlicht2.bgra 0 bgra "..width2.." "..height2.." "..width2*4)
            mp.command("overlay_add 4 "..(math.floor(screen_width/2)-(mw-1)/2).." "..(offset).." /tmp/arrow_down.bgra 0 bgra "..mw.." "..mh.." "..mw*4)
            mp.command("overlay_add 5 "..(math.floor(screen_width/2)-(mw-1)/2).." "..(height2+mh+offset).." /tmp/arrow_up.bgra 0 bgra "..mw.." "..mh.." "..mw*4)
        end
    end
end

function on()
    if not is_on then
        timer = mp.add_periodic_timer(1.0/30, update)
        is_on = true
    end
end

function off()
    if is_on then
        mp.cancel_timer(timer)
        mp.command("overlay_remove 0")
        mp.command("overlay_remove 1")
        mp.command("overlay_remove 2")
        mp.command("overlay_remove 3")
        mp.command("overlay_remove 4")
        mp.command("overlay_remove 5")
        is_on = false
    end
end

function toggle()
    if is_on then
        off()
        if mode == 1 then
            mode = 2
            on()
        end
    else
        mode = 1
        on()
    end
end

function fullscreen()
    mp.set_property("fullscreen", "yes")
end

-- not an optimal solution, but it seems to work:
mp.add_timeout(0.5, fullscreen)
mp.add_timeout(2, init)
