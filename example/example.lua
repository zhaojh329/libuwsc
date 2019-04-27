#!/usr/bin/lua

local ev = require "ev"
local uwsc = require "uwsc"
local loop = ev.Loop.default

local url = "ws://localhost:8082/echo"
local PING_INTERVAL = 5
local auto_reconnect = true
local RECONNECT_INTERVAL = 5
local do_connect = nil
local sender = nil

local function stop_send()
    if sender then
        sender:stop(loop)
        sender = nil
    end
end

local function on_open(c)
	print(loop:now(), "open ok")

	sender = ev.Timer.new(function(loop, timer, revents)
	    c:send_text("Text Message   - " .. os.time())
	    c:send_binary("Binary Message - " .. os.time())
    end, 0.5, 1)

    sender:start(loop)
end

local function start_reconnect()
	if not auto_reconnect then
		loop:unloop()
		return
	end

	ev.Timer.new(function()
		do_connect()
	end, RECONNECT_INTERVAL):start(loop)
end

do_connect = function()
	local c, err = uwsc.new(url, PING_INTERVAL)
	if not c then
	    print(loop:now(), err)
	    return
	end

	c:on("open", function()
		on_open(c)
	end)

	c:on("message", function(data, is_binary)
		print(loop:now(), "Received message:", data, "is binary:", is_binary)
	end)

	c:on("close", function(code, reason)
		print(loop:now(), "Closed by peer:", code, reason)
		stop_send()
		start_reconnect()
	end)

	c:on("error", function(err, msg)
		print(loop:now(), "Error occurred:", err, msg)
		stop_send()
		start_reconnect()
	end)
end

ev.Signal.new(function()
	loop:unloop()
end, ev.SIGINT):start(loop)

do_connect()

-- major minor patch
local version = string.format("%d.%d.%d",  uwsc.version())
print(loop:now(), "Version:", version)

loop:loop()

print(loop:now(), "Normal quit")