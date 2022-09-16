

local mem = manager.machine.devices[":maincpu"].spaces["program"]

local game_started_addrs = {"0x9012"}
local game_started = 0

--function ms_get_game_started()
--    val = mem:read_u8(game_started_addrs[1])
--    if (val ~= nil) then
--        game_started = val
--    end
--end
function ms_game_started_write_listener(offset, data, mask)
    game_started = data
    print("Game started: " .. tostring(data))
    if (game_started == 0) then
        print("Stable?")
    end
    
end

function ms_register_game_started_listener()
    game_started_tap = mem:install_write_tap(0x9012, 0x9012, "ms_game_started_write_listener", ms_game_started_write_listener)
    emu.register_stop(
        function() 
            game_started_tap:remove() 
        end
    )
end
ms_register_game_started_listener()

local score_addrs = {"0x83FF","0x83FE","0x83FD","0x83FC","0x83FB","0x83FA","0x83F9","0x83F8" }

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


local credits_addr = {"0x99B8"}

function ms_get_credits()
    val = mem:read_u8(credits_addr[1])
    if (val ~= nil) then
        credits = tonumber(tostring(val // 16) .. tostring(val % 16))
        --print("Credits: " .. credits)
        return credits
    end
    return 0
end

local lives_addr = {"0x9820"}

function ms_get_lives()
    val = mem:read_u8(lives_addr[1])
    if (val ~= nil) then
        return val
    end
    return 0
end

local stage_addr = {"0x9821"}
local prev_stage = 255;

function ms_get_stage()
    val = mem:read_u8(stage_addr[1])
    if (val ~= nil) then
        return val
    end
    return 0
end

-- prepare a socket for writing data and reading commands
local socket = emu.file("rw")
socket:open("socket.127.0.0.1:2159")
local frame = 0

function ms_on_frame_done()
    score = ms_get_score()
    credits = ms_get_credits()
    lives = ms_get_lives()
    stage = ms_get_stage()
    frame = frame + 1

    socket:write(frame .. ',' .. score .. ',' .. credits .. ',' .. lives .. ',' .. stage)
end

emu.register_frame_done(ms_on_frame_done, "ms_on_frame_done")