-- nordlicht integration for mpv. You need mpv >= 0.3.6 to correctly support
-- lua scripting.

utils = require "mp.utils"

function init()
    styles = os.getenv("NORDLICHT_STYLE") or "horizontal"

    mp.register_event("file-loaded", new_file)
    mp.register_event("shutdown", shutdown)
    mp.register_event("seek", seek)
    mp.add_key_binding("n", "nordlicht-toggle", toggle)
    mp.add_key_binding("N", "nordlicht-regenerate", regenerate)
    mp.add_key_binding("mouse_btn0", "jump", jump)

    -- size of the progress marker
    mh = 10
    mw = mh*2-1

    -- yeah, I know. But it's flexible :-P
    utils.subprocess({args={"convert", "-depth", "8", "-size", mw.."x"..mh, "xc:none", "-fill", "white", "-stroke", "black", "-strokewidth", "0.5", "+antialias", "-draw", "path 'M"..((mw-1)/2)..",0L"..(mw-1)..","..(mh-1).."L0,"..(mh-1).."Z'", "bgra:/tmp/arrow_up.bgra"}})
    utils.subprocess({args={"convert", "-depth", "8", "-size", mw.."x"..mh, "bgra:/tmp/arrow_up.bgra", "-flip", "bgra:/tmp/arrow_down.bgra"}})
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
    shutdown()

    -- size of the barcode
    width = mp.get_property("osd-width")
    height = math.floor(width/20)

    video = mp.get_property("path")
    nordlicht = video..".nordlicht.png"
    buffer = "/tmp/nordlicht.bgra"

    local f = io.open(nordlicht, "r")
    if f ~= nil then
        -- a suitable nordlicht already exists, use that
        io.close(f)
        local cmd = {"convert", nordlicht, "-depth", "8", "-resize", width.."x"..height.."!", buffer}
        utils.subprocess({args=cmd})

        if was_on then
            on()
        end
    else
        -- no nordlicht exists, generate one
        regenerate()

        if was_on then
            -- wait for the buffer to be opened and truncated
            mp.add_timeout(0.5, on)
        end
    end

end

function update()
    local pos = mp.get_property("percent-pos")

    if pos ~= nil then
        mp.command("overlay_add 0 0 "..mh.." "..buffer.." 0 bgra "..width.." "..height.." "..width*4)
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

function seek()
	on()

	if seek_off_timer then
		seek_off_timer:kill()
	end
	seek_off_timer = mp.add_timeout(1, off)
end

function regenerate()
    local was_on = is_on
    off()
    local cmd = "(nice nordlicht -s "..styles.." \""..video.."\" -o "..buffer.." -w "..width.." -h "..height.." && convert -depth 8 -size "..width.."x"..height.." "..buffer.." \""..nordlicht.."\") &"
    os.execute(cmd)
    if was_on then
        on()
    end
end

function jump(e)
    local mouseX, mouseY = mp.get_mouse_pos()
    local osdX, osdY = mp.get_osd_resolution()
    mouseX = 100.0*mouseX/osdX

    mp.commandv("seek", mouseX, "absolute-percent", "exact")
end

-- wait until the osd-width is > 0, then init
function maybeinit()
    if tonumber(mp.get_property("osd-width")) > 0 then
        init()
        is_on = false
        new_file()
        on()
    else
        mp.add_timeout(0.1, maybeinit)
    end
end

mp.set_property("fullscreen", "yes")
maybeinit()
