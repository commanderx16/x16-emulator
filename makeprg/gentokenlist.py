# *******************************************************************************************
# *******************************************************************************************
#
#		Name : 		gentokenlist.py
#		Purpose :	Rip the BASIC tokenising tables from the ROM image.
#		Date :		15th August 2019
#		Author : 	Paul Robson (paul@robsons.org.uk)
#
# *******************************************************************************************
# *******************************************************************************************
#
#			Where END starts (sequence $45,$4E,$C4). This may move !
#
endToken = 0xA6 												# Where the END token starts.


rom = [x for x in open("rom.bin","rb").read(-1)] 				# read rom in
tokenList = []
p = endToken
tokenID = 0x80
while rom[p] != 0:												# end of table is $00
	size = 0 													# end of entry is bit 7 set.
	while rom[p+size] < 0x80:
		size += 1
	size += 1 													# total size
	token = "".join([chr(x & 0x7F) for x in rom[p:p+size]])
	p = p + size	
	realToken = tokenID if tokenID <= 203 else 0xCE80+(tokenID-204)
	#print("{0:4x} {1}".format(realToken,token))
	tokenList.append([realToken,token])
	if tokenID == 203: 											# read the second table starting after GO
		p += 1
	tokenID += 1

print("::".join(["{0:x}.{1}".format(t[0],t[1]) for t in tokenList]))