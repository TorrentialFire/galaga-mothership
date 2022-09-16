-- MAME Memory access object
local mem = manager.machine.devices[":maincpu"].spaces["program"]
local cpu2mem = manager.machine.devices[":sub"].spaces["program"]
local memory_taps = {}                  -- table to keep taps from becoming orphaned and garbage collected

-- Memory locations
local game_started_addr     = 0x9012    -- is a game currently in progress?
local shot_addr             = 0x9846    -- lowest byte of player shots
local hit_addr              = 0x9844    -- lowest byte of number of hits
local score_addrs =                     -- VRAM locations of the player 1 score
    {"0x83FF", "0x83FE", "0x83FD", "0x83FC","0x83FB","0x83FA","0x83F9","0x83F8"}
local credits_addr          = 0x99B8
local lives_addr = {"0x9820"}
local stage_addr = {"0x9821"}


-- Mechanics and statistics
local game_active           = false     -- boolean indicating whether a game is active
local game_started          = -1        -- (u8:from mem) value indicating if a game has been started
local shot_count            = 0         -- number tracking the number of shots fired in the current game, calculated via: (sc_rollover * 256) + sc_low_byte
local sc_low_byte           = 0         -- (u8:from mem) low-byte of the number of shots fired
local sc_rollover           = 0         -- number tracking the number of times during the current game the low-byte of the shots fired has wrapped from 255 to 0.
local hit_count             = 0         -- number tracking the number of hits scored during the current game, calculated via: (hit_rollover * 256) + hit_low_byte
local hit_low_byte          = 0         -- (u8:from mem) low-byte of the number of hits
local hit_rollover          = 0         -- number tracking the number of times during the current game the low-byte of the hit counter has wrapped from 255 to 0.
local accuracy              = 0
local prev_stage            = 255
local credits               = 0
local inf_credits           = true
local frame = 0

-- Socket for writing data and reading commands from Mothership app
local socket = emu.file("rw")
socket:open("socket.127.0.0.1:2159")

-- Useful for debugging when taps get garbage collected
-- (i.e. they weren't properly registered with or they were removed from the memory_taps table)
local sub = cpu2mem:add_change_notifier(
    function(handler_type)
        print(handler_type .. " has changed! Was something GC'd!?")
   end
)
emu:register_stop(function() sub:unsubscribe() end)

function ms_register_tap(addr_start, addr_end, callback, tap_name)
    if (memory_taps[tap_name] == nil) then
        print("Tap \"" .. tap_name .. "\" not found, installing...")
        tap =
            mem:install_write_tap(
                addr_start,
                addr_end,
                tap_name,
                callback)
        memory_taps[tap_name] = tap
        emu.register_stop(
            function()
                tap:remove()
            end
        )
    else
        print("Tap \"" .. tap_name .. "\" already present, reinstalling...")
        memory_taps[tap_name]:reinstall()
    end
end

-- When a game is started, reset variables that track game statistics.
function ms_on_game_started_reset()
    game_active = true
    shot_count = 0
    sc_rollover = 0
    hit_count = 0
    hit_rollover = 0
end

-- Listener for writes to 'game_started_addr' to determine whether the player
-- has started a new game or when a game has ended.
function ms_game_started_write_listener(offset, data, mask)
    game_started = data
    if (game_started == 0) then
        print("Game ended")
        game_active = false
    end
    if (game_started == 1) then
        print("Game started")
        ms_on_game_started_reset()
    end
end

function ms_register_game_started_listener()
    ms_register_tap(
        game_started_addr, 
        game_started_addr, 
        ms_game_started_write_listener,
        "ms_game_started_write_listener")
end
ms_register_game_started_listener()


function ms_shot_fired_write_listener(offset, data, mask)
    if (sc_low_byte == 255) then
        sc_rollover = sc_rollover + 1
    end
    if (data ~= nil) then
        sc_low_byte = data
        shot_count = (256 * sc_rollover) + sc_low_byte
        print("Shot: " .. tostring(shot_count))
    end
end

function ms_register_shot_fired_listener()
    ms_register_tap(
        shot_addr, 
        shot_addr, 
        ms_shot_fired_write_listener,
        "ms_shot_fired_write_listener")
end
ms_register_shot_fired_listener()

-- something goes wrong around 80 - 88 hits, and this stops working
function ms_hits_write_listener(offset, data, mask)
    print("Hits write!")
    if (hit_low_byte == 255) then
        hit_rollover = hit_rollover + 1
    end
    if (data ~= nil) then
        hit_low_byte = data
        hit_count = (256 * hit_rollover) + hit_low_byte
        if (shot_count > 0) then
            accuracy = hit_count / shot_count * 100
            print("Hit: " .. tostring(hit_count))
            print("Acc: " .. tostring(accuracy))
        end
    end
end

function ms_register_hits_listener()
    --ms_register_tap(hit_addr, hit_addr, ms_hits_write_listener, "ms_hits_write_listener")
    tap =
        cpu2mem:install_write_tap(
            hit_addr,
            hit_addr,
            "ms_hits_write_listener",
            ms_hits_write_listener)
    memory_taps["ms_hits_write_listener"] = tap
    emu.register_stop(
        function()
            tap:remove()
        end
    )
end
ms_register_hits_listener()

function ms_credits_write_listener(offset, data, mask)
    credits = tonumber(tostring(data // 16) .. tostring(data % 16))
    print("Credits: " .. credits)
    if (game_started == 0 and inf_credits) then
        print("INFINITE POWER")
        --return 0x99
    end
end

function ms_register_credits_listener()
    ms_register_tap(
        credits_addr, 
        credits_addr, 
        ms_credits_write_listener,
        "ms_credits_write_listener")
end
ms_register_credits_listener()


function ms_get_score()
    score = ""
    for k, addr in pairs(score_addrs) do
    mem_val = mem:read_u8(addr)
        if (mem_val < 0x0A) then
                score = score .. tostring(mem_val)
        end
    end

    ret_score = 0
    num_score = tonumber(score)
    if (num_score ~= nil) then
        ret_score = num_score
    end
    
    return ret_score
end

function ms_get_credits()
    val = mem:read_u8(credits_addr[1])
    if (val ~= nil) then
        credits = tonumber(tostring(val // 16) .. tostring(val % 16))
        --print("Credits: " .. credits)
        return credits
    end
    return 0
end

function ms_get_lives()
    val = mem:read_u8(lives_addr[1])
    if (val ~= nil) then
        return val
    end
    return 0
end

function ms_get_stage()
    val = mem:read_u8(stage_addr[1])
    if (val ~= nil) then
        return val
    end
    return 0
end

function ms_on_frame_done()
    score = ms_get_score()
    --credits = ms_get_credits()
    lives = ms_get_lives()
    stage = ms_get_stage()
    frame = frame + 1

    socket:write(frame .. ',' .. score .. ',' .. credits .. ',' .. lives .. ',' .. stage)
end

emu.register_frame_done(ms_on_frame_done, "ms_on_frame_done")