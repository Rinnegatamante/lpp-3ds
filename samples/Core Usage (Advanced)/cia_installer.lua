-- Opening CIA file to install
handle = io.open("/file.cia",FREAD)
file_size = io.size(handle)

-- Initializing AM service
ret = Core.execCall("amInit")
if not ret == 0 then
	error("amInit returned an error: 0x" .. string.format("%X",ret))
end

-- Allocating memory for cia handle
cia_handle = Core.alloc(0x04)

-- Launching StartCiaInstall syscall with mediatype SDMC (0x01)
ret = Core.execCall("AM_StartCiaInstall", 0x01, cia_handle)
if not ret == 0 then
	error("AM_StartCiaInstall returned an error: 0x" .. string.format("%X",ret))
end

-- Reading memory block for Handle value
tmp = Core.readWord(cia_handle)

-- Generating a fileStream starting from Handle value
content = Core.getHandle(tmp)

-- Starting installation
i = 0
BUF_SIZE = 5242880
while i < file_size do
	block_size = BUF_SIZE
	if i + BUF_SIZE > file_size then
		block_size = file_size - i
	end
	io.write(content,i,io.read(handle, i, block_size),block_size)
	i = i + block_size
end

-- Launching FinishCiaInstall syscall with mediatype SDMC (0x01)
ret = Core.execCall("AM_FinishCiaInstall", cia_handle)
if not ret == 0 then
	error("AM_FinishCiaInstall returned an error: 0x" .. string.format("%X",ret))
end

-- Closing AM service
Core.execCall("amExit")

-- Freeing memory and exiting
io.close(handle)
io.close(content)
Core.free(cia_handle)
System.exit()