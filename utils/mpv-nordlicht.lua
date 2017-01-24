-- nordlicht integration for mpv. You need mpv >= 0.3.6 to correctly support lua scripting.
-- You also need ImageMagick's "convert" in your PATH, as well as the luaposix >= 33.3.0 and md5 packages.

utils = require "mp.utils"
posix = require "posix"
md5 = require "md5"

-- check if a file exists
function file_exists(name)
   local f = io.open(name, "r")
   if f ~= nil then io.close(f) return true else return false end
end

-- to be called when the played file changes
function new_file()
    kill()

    video = mp.get_property("path")

    if video == nil then
        -- no path yet, try again...
        mp.add_timeout(1/30, new_file)
        return
    end

    resize()
    nordlicht = NORDLICHT_CACHE_DIR..md5.sumhexa(mp.get_property("filename"))..".png"

    if not file_exists(nordlicht) then
        livewidth = width
        liveheight = math.floor(livewidth/20)
        local styles = os.getenv("NORDLICHT_STYLE") or "horizontal"

        local pfd = posix.popen({"nordlicht", "-s", styles, video, "-o", nordlicht, "-b", livebuffer, "-w", tostring(livewidth), "-h", tostring(liveheight)}, "r")
        pid = pfd.pids[1]
    end
end

-- to be called when the nordlicht changes
function update()
    if is_on then
        if file_exists(nordlicht) then
            -- final PNG nordlicht exists
            local cmd = {"convert", nordlicht, "-depth", "8", "-resize", width.."x"..height.."!", buffer}
            utils.subprocess({args=cmd})

            -- process must have ended
            pid = nil
        elseif file_exists(livebuffer) then
            -- nordlicht is probably being generated right now
            local cmd = {"convert", "-size", livewidth.."x"..liveheight, "-depth", "8", livebuffer, "-depth", "8", "-resize", width.."x"..height.."!", buffer}
            utils.subprocess({args=cmd})
        else
            -- no nordlicht available yet
            mp.command("overlay_remove 0")
        end

        local pos = mp.get_property("percent-pos")

        if file_exists(buffer) then
            mp.command("overlay_add 0 0 0 "..buffer.." 0 bgra "..width.." "..height.." "..width*4)
        end

        if pos ~= nil then
            mp.command("overlay_add 1 "..(math.floor(pos/100*width)-(mw-1)/2).." "..(height).." /tmp/arrow_up.bgra 0 bgra "..mw.." "..mh.." "..mw*4)
        end
    end

    mp.add_timeout(1.0/30, update)
end

-- to be called when the user request a regeneration
function regenerate()
    os.remove(nordlicht)
    new_file()
end

-- start update loop, always show the nordlicht
function on()
    if not is_on then
        is_on = true
    end
end

-- hide the nordlicht as soon as the mouse leaves it
function off()
    if is_on then
        local mouse_x, mouse_y = mp.get_mouse_pos()

        if mouse_y > 2*height then
            mp.command("overlay_remove 0")
            mp.command("overlay_remove 1")
            is_on = false
        else
            if off_timer then
                off_timer:kill()
            end
            off_timer = mp.add_timeout(1/30, off)
        end
    end
end

-- to be called when the window size changes
function resize()
    width = mp.get_property_number("osd-width")
    height = math.floor(width/20)

    -- size of the progress marker
    mh = math.floor(height/5)
    mw = mh*2-1

    -- create the little marker triangle using ImageMagick
    utils.subprocess({args={"convert", "-depth", "8", "-size", mw.."x"..mh, "xc:none", "-fill", "white", "-stroke", "black", "-strokewidth", "0.5", "+antialias", "-draw", "path 'M"..((mw-1)/2)..",0L"..(mw-1)..","..(mh-1).."L0,"..(mh-1).."Z'", "bgra:/tmp/arrow_up.bgra"}})
    utils.subprocess({args={"chmod", "a+r", "/tmp/arrow_up.bgra"}})
    --utils.subprocess({args={"convert", "-depth", "8", "-size", mw.."x"..mh, "bgra:/tmp/arrow_up.bgra", "-flip", "bgra:/tmp/arrow_down.bgra"}})
end

-- kill the current nordlicht process, if there is one
function kill()
    if pid then
        posix.kill(pid)
        pid = nil
    end
end

-- to be called when the left mouse button is pressed
function click()
    if not is_on then
        return
    end

    local mouse_x, mouse_y = mp.get_mouse_pos()

    if mouse_y <= 2*height then
        mp.commandv("seek", 100.0*mouse_x/width, "absolute-percent", "exact")
    end
end

-- poke the nordlicht awake for 1 second
function poke()
    if not is_on then
        on()
        if off_timer then
            off_timer:kill()
        end
        off_timer = mp.add_timeout(1, off)
    end
end

-- poke the nordlicht awake when moving the mouse over it
function mouse_move()
    if not is_on then
        local mouse_x, mouse_y = mp.get_mouse_pos()

        if mouse_y <= 2*height then
            poke()
        end
    end
end

-- set up buffer filenames and hooks
function init()
    NORDLICHT_CACHE_DIR = os.getenv("XDG_CACHE_HOME") or os.getenv("HOME").."/.cache"
    NORDLICHT_CACHE_DIR = NORDLICHT_CACHE_DIR.."/nordlicht/"
    posix.mkdir(NORDLICHT_CACHE_DIR)
    -- nordlicht's internal bgra buffer will be mmapped to this file
    local tmpbuffer = os.tmpname()
    livebuffer = tmpbuffer .. ".nordlicht.mpv.bgra"

    -- buffer will be used for the bgra data that is displayed on screen
    local tmpbuffer = os.tmpname()
    buffer = tmpbuffer .. ".nordlicht.mpv.bgra"
    os.rename(tmpbuffer, buffer)

    mp.register_event("file-loaded", new_file)
    mp.register_event("shutdown", shutdown)
    mp.register_event("seek", poke)
    mp.observe_property("osd-width", "number", resize)
    mp.add_key_binding("N", "nordlicht-regenerate", regenerate)
    mp.add_key_binding("mouse_btn0", "nordlicht-click", click)
    mp.add_key_binding("mouse_move", mouse_move)

    new_file()
    is_on = false
    update()
end

-- wait until the osd-width is > 0, then init
function maybeinit()
    if mp.get_property_number("osd-width") > 0 and mp.get_property("path") ~= nil then
        init()
    else
        mp.add_timeout(0.1, maybeinit)
    end
end

maybeinit()
