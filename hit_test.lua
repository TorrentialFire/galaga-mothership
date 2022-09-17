local cpu2mem = manager.machine.devices[":sub"].spaces["program"]

local hit_addr      = 0x9844
local hit_low_byte  = 0 
local hit_rollover  = 0
local hit_count     = 0

function ms_hits_write_tap(offset, data, mask)
    if (hit_low_byte == 255) then
        hit_rollover = hit_rollover + 1
    end
    if (data ~= nil) then
        hit_low_byte = data
        hit_count = (256 * hit_rollover) + hit_low_byte
        print("Hit: " .. tostring(hit_count))
    end
end

local tap = nil

function ms_register_hits_tap()
    print("Registering tap")
    tap =
        cpu2mem:install_write_tap(
            hit_addr,
            hit_addr,
            "ms_hits_write_tap",
            ms_hits_write_tap)
    emu.register_stop(
        function()
            tap:remove()
        end
    )
end

ms_register_hits_tap()