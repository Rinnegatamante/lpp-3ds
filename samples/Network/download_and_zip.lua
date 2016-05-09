-- Check if WiFi is enabled
if Network.isWifiEnabled() then
	
	-- Download a file
	Network.downloadFile("http://rinnegatamante.netsons.org/ORG/0.7/TEST.zip","/Downloaded.zip")
	
	-- Extract the file
	System.extractZIP("/Downloaded.zip",System.currentDirectory())
	
	-- Delete the temp file
	System.deleteFile("/Downloaded.zip")
	
end

-- Return HBL
System.exit()
