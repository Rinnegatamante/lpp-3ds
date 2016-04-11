-- Title ID of the CIA we're going to delete is 0x0004000000287000.
-- Since Lua doesn't support 64 bit integers natively, we have to pass an offset
-- pointing to our 64 bit integer value. Remember that 3DS is a LE machine!

-- Allocating our 64 bit integer
title_id = Core.alloc(0x08)
Core.storeWord(title_id, 0x00287000)
Core.storeWord(title_id + 0x04, 0x00040000)

-- Initializing AM service
Core.execCall("amInit")
if not ret == 0 then
	error("amInit returned an error: 0x" .. string.format("%X",ret))
end

-- Launching both Delete calls just to be sure
Core.execCall("AM_DeleteTitle", 0x01, title_id)
Core.execCall("AM_DeleteAppTitle", 0x01, title_id)

-- Closing AM service
Core.execCall("amExit")

-- Freeing memory and exiting
Core.free(title_id)
System.exit()