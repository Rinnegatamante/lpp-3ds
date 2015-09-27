-----------------------------------------------------------------------------
--           This sample works fine with PC utility, CHMM2 sender          --
--              You can find the source of this utility here:              --
-- https://github.com/Rinnegatamante/CHMM2/blob/master/tools/sender/main.c --
-----------------------------------------------------------------------------

-- Init socketing and create a server socket on Port 5000
Socket.init()
server = Socket.createServerSocket(5000)

-- Create a debug console
cns = Console.new(TOP_SCREEN)
state = 0

while true do

	-- Clear screen
	Screen.refresh()
	Screen.clear(TOP_SCREEN)
	
	-- State 0: Looking for connections
	if state == 0 then
	
		-- Writing info on screen
		Console.append(cns, "Looking for connections...")
		Console.append(cns, "\nIP: "..Network.getIPAddress())
		Console.show(cns)
		Screen.flip()
		Screen.waitVblankStart()
		Console.clear(cns)
		
		-- Searching for clients
		client = Socket.accept(server)
		if client == nil then
			state = 0
		else
			state = 1
		end
		
	-- State 1: Connection estabilished, sending response to client
	elseif state == 1 then
	
		-- Writing info on screen
		Console.append(cns, "Sending magic...\n")
		Console.show(cns)
		Screen.flip()
		Screen.waitVblankStart()
		Console.clear(cns)
		
		-- Sending magic word expected from client to communicate
		Socket.send(client, "YATA SENDER")
		state = 2
		
		-- Opening file to be received
		new_theme = io.open("/tmp.zip",FCREATE)
		offs = 0
		
	-- State 2: File receiver
	elseif state == 2 then
	
		-- Writing info on screen
		Console.append(cns, "Receiving file...\n")
		Console.show(cns)
		Screen.flip()
		Screen.waitVblankStart()
		
		-- File receiver on work with 256 bytes packets
		block = Socket.receive(client, 256)
		while string.len(block) > 0 do
			io.write(new_theme, offs, block, string.len(block))
			offs = offs + string.len(block)
			block = Socket.receive(client, 256)
		end
		
		-- Closing sockets instances and file
		io.close(new_theme)
		Socket.close(server)
		Socket.close(client)
		Socket.term()
		
		-- Writing info on screen
		Console.clear(cns)
		Console.append(cns, "File received\nPress START to exit.")
		state = 3
		
	else
	
		-- Script finished
		Console.show(cns)
		Screen.flip()
		Screen.waitVblankStart()
		if Controls.check(Controls.read(), KEY_START) then
			System.exit()
		end
		
	end

end