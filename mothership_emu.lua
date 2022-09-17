dofile "queue.lua"

-- MAME Memory access object
local cpu1mem = manager.machine.devices[":maincpu"].spaces["program"]
local cpu2mem = manager.machine.devices[":sub"].spaces["program"]
local memory_taps = {}                      -- table to keep taps from becoming orphaned and garbage collected

-- Memory locations
local game_started_addr     = 0x9012        -- is a game currently in progress?
local shot_addr             = 0x9846        -- lowest byte of player shots
local hit_addr              = 0x9844        -- lowest byte of number of hits
local score_start_addr      = 0x83F8
local score_end_addr        = 0x83FF
local score_addrs =                         -- VRAM locations of the player 1 score
    {"0x83FF", "0x83FE", "0x83FD", "0x83FC","0x83FB","0x83FA","0x83F9","0x83F8"}
local credits_addr          = 0x99B8
local lives_addr            = 0x9820
local stage_addr            = 0x9821


-- Mechanics and statistics
local queues = {
    ["shot_count"]          = Queue.new(),  -- queue tracking the number of shots fired in the current game, calculated via: (sc_rollover * 256) + sc_low_byte
    ["hit_count"]           = Queue.new(),
    ["accuracy"]            = Queue.new(),
    ["credits"]             = Queue.new(),
    ["lives"]               = Queue.new(),
    ["stage"]               = Queue.new(),
    ["score"]               = Queue.new()
}
local rom_check_cleared     = false         -- boolean indicating if the rom check has been cleared
local game_active           = false         -- boolean indicating whether a game is active
local game_started          = -1            -- (u8:from mem) value indicating if a game has been started
local shot_count            = 0
local sc_low_byte           = 0             -- (u8:from mem) low-byte of the number of shots fired
local sc_rollover           = 0             -- number tracking the number of times during the current game the low-byte of the shots fired has wrapped from 255 to 0.
local hit_low_byte          = 0             -- (u8:from mem) low-byte of the number of hits
local hit_rollover          = 0             -- number tracking the number of times during the current game the low-byte of the hit counter has wrapped from 255 to 0.
local prev_stage            = 255
local inf_credits           = true
local frame                 = 0

-- Socket for writing data and reading commands from Mothership app
local socket = emu.file("rw")
socket:open("socket.127.0.0.1:2159")

-- Useful for debugging when taps get garbage collected
-- (i.e. they weren't properly registered with or they were removed from the memory_taps table)
local sub_cpu1 = cpu1mem:add_change_notifier(
    function(handler_type)
        print(handler_type .. " has changed! Was something GC'd!?")
   end
)
emu:register_stop(function() sub_cpu1:unsubscribe() end)

local sub_cpu2 = cpu2mem:add_change_notifier(
    function(handler_type)
        print(handler_type .. " has changed! Was something GC'd!?")
   end
)
emu:register_stop(function() sub_cpu2:unsubscribe() end)

function ms_register_write_tap(space, addr_start, addr_end, callback, tap_name)
    if (memory_taps[tap_name] == nil) then
        print("Tap \"" .. tap_name .. "\" not found, installing...")
        tap =
            space:install_write_tap(
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
function ms_game_started_write_cb(offset, data, mask)
    game_started = data
    if (game_started == 0) then
        print("Game ended")
        game_active = false
        rom_check_cleared = true
    end
    if (game_started == 1) then
        print("Game started")
        ms_on_game_started_reset()
    end
end

function ms_register_game_started_tap()
    ms_register_write_tap(
        cpu1mem,
        game_started_addr, 
        game_started_addr, 
        ms_game_started_write_cb,
        "ms_game_started_write_cb")
end
ms_register_game_started_tap()

function ms_shot_fired_write_cb(offset, data, mask)
    if (sc_low_byte == 255) then
        sc_rollover = sc_rollover + 1
    end
    if (data ~= nil) then
        sc_low_byte = data
        local shot_count = (256 * sc_rollover) + sc_low_byte
        Queue.push_back(queues["shot_count"], shot_count)
        --print("Shot: " .. tostring(shot_count))
    end
end

function ms_register_shot_fired_tap()
    ms_register_write_tap(
        cpu1mem,
        shot_addr, 
        shot_addr, 
        ms_shot_fired_write_cb,
        "ms_shot_fired_write_cb")
end
ms_register_shot_fired_tap()

function ms_hits_write_cb(offset, data, mask)
    if (hit_low_byte == 255) then
        hit_rollover = hit_rollover + 1
    end
    hit_low_byte = data
    local hit_count = (256 * hit_rollover) + hit_low_byte
    Queue.push_back(queues["hit_count"], hit_count)
    if (shot_count > 0) then
        local accuracy = hit_count / shot_count * 100
        Queue.push_back(queues["accuracy"], accuracy)
        --print("Hit/Acc: " .. tostring(hit_count) .. "/" .. tostring(accuracy) .. "%")
    end
end

function ms_register_hits_tap()
    -- The secondary CPU handles bullet/bug collision detection.
    -- So, this tap is installed in cpu2mem instead (which overlaps with cpu1).
    ms_register_write_tap(
        cpu2mem,
        hit_addr,
        hit_addr,
        ms_hits_write_cb,
        "ms_hits_write_cb")
end
ms_register_hits_tap()

function ms_credits_write_cb(offset, data, mask)
    local credits = tonumber(tostring(data // 16) .. tostring(data % 16))
    Queue.push_back(queues["credits"], credits)
    --print("Credits: " .. credits)
    --if (rom_check_cleared and inf_credits) then
    --    print("INFINITE POWER")
        --return 0x99
    --end
end

function ms_register_credits_tap()
    ms_register_write_tap(
        cpu1mem,
        credits_addr, 
        credits_addr, 
        ms_credits_write_cb,
        "ms_credits_write_cb")
end
ms_register_credits_tap()

function ms_score_write_cb(offset, data, mask)
    print("Score offset: " .. tostring(offset - score_start_addr) .. ", Data: " .. tostring(data))
end

function ms_register_score_tap()
    ms_register_write_tap(
        cpu1mem,
        score_start_addr,
        score_end_addr,
        ms_score_write_cb,
        "ms_score_write_cb"
    )
end
ms_register_score_tap()

--function ms_get_score()
--    score = ""
--    for k, addr in pairs(score_addrs) do
--    mem_val = cpu1mem:read_u8(addr)
--        if (mem_val < 0x0A) then
--                score = score .. tostring(mem_val)
--        end
--    end
--
--    ret_score = 0
--    num_score = tonumber(score)
--    if (num_score ~= nil) then
--        ret_score = num_score
--    end
--    
--    return ret_score
--end

function ms_lives_write_cb(offset, data, mask)
    local lives = data
    Queue.push_back(queues["lives"], lives)
end

function ms_register_lives_tap()
    ms_register_write_tap(
        cpu1mem,
        lives_addr,
        lives_addr,
        ms_lives_write_cb,
        "ms_lives_write_cb")
end
ms_register_lives_tap()

function ms_stage_write_cb(offset, data, mask)
    local stage = data
    Queue.push_back(queues["stage"], stage)
end

function ms_register_stage_tap()
    ms_register_write_tap(
        cpu1mem,
        stage_addr,
        stage_addr,
        ms_stage_write_cb,
        "ms_stage_write_cb"
    )
end
ms_register_stage_tap()

function ms_on_frame_done()
    --score = ms_get_score()
    --credits = ms_get_credits()
    --lives = ms_get_lives()
    --stage = ms_get_stage()
    frame = frame + 1

    for k, v in pairs(queues) do
        while (not Queue.is_empty(v)) do
            local val = Queue.pop_front(v)
            print(k .. ": " .. tostring(val))

            if (k == "shot_count") then
                shot_count = val
            end
        end
    end

    socket:write(frame)
end

emu.register_frame_done(ms_on_frame_done, "ms_on_frame_done")