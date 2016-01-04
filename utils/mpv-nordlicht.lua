-- nordlicht integration for mpv. You need mpv >= 0.3.6 to correctly support
-- lua scripting.

function init()
    is_on = false

    -- size of the barcode:
    width = mp.get_property("osd-width")
    height = math.floor(width/20)

    -- size of the progress marker:
    mh = math.floor(height/15)*2+1
    mw = mh*2-1

    -- styles:
    styles = os.getenv("NORDLICHT_STYLE") or "horizontal"

    mp.register_event("file-loaded", new_file)
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
    local path = mp.get_property("path")
    local cmd = "nice "..nordlicht_cmd.." \""..path.."\" -o /tmp/nordlicht.bgra -w "..width.." -h "..height.." &"
    os.execute(cmd)

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
        is_on = false
    end
end

function toggle()
    if is_on then
        off()
    else
        on()
    end
end

mp.add_key_binding("mouse_btn0", "jump",
    function(e)
        local mouseX, mouseY = mp.get_mouse_pos()
        local osdX, osdY = mp.get_osd_resolution()
        mouseX = 100.0*mouseX/osdX

        mp.commandv("seek", mouseX, "absolute-percent", "exact")
    end
)

function maybeinit()
    if tonumber(mp.get_property("osd-width")) > 0 then
        init()
    else
        mp.add_timeout(0.1, maybeinit)
    end
end

mp.set_property("fullscreen", "yes")
maybeinit()
