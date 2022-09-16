-- MAME Memory access object
local mem = manager.machine.devices[":maincpu"].spaces["program"]

-- Memory locations
local game_started_addr     = 0x9012    -- is a game currently in progress?
local shot_addr             = 0x9846    -- lowest byte of player shots
local score_addrs =                     -- VRAM locations of the player 1 score
    {"0x83FF", "0x83FE", "0x83FD", "0x83FC","0x83FB","0x83FA","0x83F9","0x83F8"}
local credits_addr = {"0x99B8"}
local lives_addr = {"0x9820"}
local stage_addr = {"0x9821"}


-- Mechanics and statistics
local game_active           = false     -- boolean indicating whether a game is active
local game_started          = 0         -- (u8:from mem) value indicating if a game has been started
local shot_count            = 0         -- number tracking the number of shots fired in the current game, calculated via: (sc_rollover * 256) + sc_low_byte
local sc_low_byte           = 0         -- (u8:from mem) low-byte of the number of shots fired
local sc_rollover           = 0         -- number tracking the number of times during the current game the low-byte of the shots fired has wrapped from 255 to 0.
local prev_stage            = 255;
local frame = 0

-- Socket for writing data and reading commands from Mothership app
local socket = emu.file("rw")
socket:open("socket.127.0.0.1:2159")


-- When a game is started, reset variables that track game statistics.
function ms_on_game_started_reset()
    game_active = true
    shot_count = 0
    sc_rollover = 0
end

-- Listener for writes to 'game_started_addr' to determine whether the player
-- has started a new game or when a game has ended.
function ms_game_started_write_listener(offset, data, mask)
    game_started = data
    if (game_started == 0) then
        game_active = false
    end
    if (game_started == 1) then
        print("Game started")
        ms_on_game_started_reset()
    end
end

function ms_register_game_started_listener()
    game_started_tap = 
        mem:install_write_tap(
            game_started_addr, 
            game_started_addr, 
            "ms_game_started_write_listener", 
            ms_game_started_write_listener)
    emu.register_stop(
        function() 
            game_started_tap:remove() 
        end
    )
end
ms_register_game_started_listener()


function ms_shot_fired_write_listener(offset, data, mask)
    if (sc_low_byte == 255) then
        sc_rollover = sc_rollover + 1
    end
    sc_low_byte = data
    shot_count = (256 * sc_rollover) + sc_low_byte
    print("Shot: " .. tostring(shot_count))
end

function ms_register_shot_fired_listener()
    shot_fired_tap =
        mem:install_write_tap(
            shot_addr,
            shot_addr,
            "ms_shot_fired_write_listener",
            ms_shot_fired_write_listener)
    emu.register_stop(
        function()
            shot_fired_tap:remove()
        end
    )
end
ms_register_shot_fired_listener()

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
    credits = ms_get_credits()
    lives = ms_get_lives()
    stage = ms_get_stage()
    frame = frame + 1

    socket:write(frame .. ',' .. score .. ',' .. credits .. ',' .. lives .. ',' .. stage)
end

emu.register_frame_done(ms_on_frame_done, "ms_on_frame_done")