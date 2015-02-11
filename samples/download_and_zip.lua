if Network.isWifiEnabled() then
	Network.downloadFile("http://rinnegatamante.netsons.org/ORG/0.7/TEST.zip","/Downloaded.zip")
	System.extractZIP("/Downloaded.zip",System.currentDirectory().."/")
	System.deleteFile("/Downloaded.zip")
end
System.exit()
