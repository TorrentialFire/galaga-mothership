-- A simple double-ended queue implementation
-- Slightly modified from https://www.lua.org/pil/11.4.html
-- Usage:
--
-- require "queue"
--
-- -- Initialize a new queue "object"
-- local q = Queue.new()
--
-- Queue.push_back(q, 8)
--
-- print("Size: " .. tostring(Queue.size(q)) .. ", Empty: " .. tostring(Queue.is_empty(q))) 
--
-- val = Queue.pop_front(q)
-- print(tostring(val)) -- should print "8"
--
-- print("Size: " .. tostring(Queue.size(q)) .. ", Empty: " .. tostring(Queue.is_empty(q))) 
--
-- -- If the queue has a limited lifetime, free it for GC
-- q = nil
--
-- NOT A THREAD SAFE QUEUE
Queue = {}

function Queue.new()
    return {back = -1, front = 0}
end

function Queue.push_back(list, value)
    local back = list.back + 1          -- first increment the back cursor
    list.back = back                    -- set the back cursor
    list[back] = value                  -- set the value at the back cursor
end

function Queue.push_front(list, value)
    local front = list.front - 1        -- first decrement the front cursor
    list.front = front                  -- set the front cursor
    list[front] = value                 -- set the value at the front cursor
end

function Queue.pop_back(list)
    local back = list.back              -- record the current back cursor position
    if back < list.first then error("queue is empty") end
    local value = list[back]            -- get the value in the current back cursor position
    list[back] = nil                    -- allow GC
    list.back = back - 1                -- update the back of the queue cursor 
    return value
end

function Queue.pop_front(list)
    local front = list.front            -- record the current front cursor position
    if list.back < front then error("queue is empty") end
    local value = list[front]           -- get the value in the current front cursor position
    list[front] = nil                   -- allow GC
    list.front = front + 1              -- update the front of the queue cursor
    return value
end

function Queue.is_empty(list)
    return (list.back < list.front)
end

function Queue.size(list)
    return (list.back - list.front) + 1
end

--function Queue.peek_back(list)
--    local back = list.back
--    if list.back < front then error("queue is empty") end
--    return list[back]
--end
--
--function Queue.peek_front(list)
--    local front = list.front
--    if list.back < front then error("queue is empty") end
--    return list[front]
--end