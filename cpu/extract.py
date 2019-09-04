# *******************************************************************************************
# *******************************************************************************************
#
#		File:		extract.py
#		Date:		3rd September 2019
#		Purpose:	Extract opcode descriptions from tables.h.org
#		Author:		Paul Robson (paul@robson.org.uk)
#
# *******************************************************************************************
# *******************************************************************************************

import re
#
#							Get one element from one table.
#
def extract(opc,n):
	line = opc[n >> 4]
	m = re.match("^\\/\\*\\s+[0-9A-Fa-f]\\s+\\*\\/(.*)\\s*\\/\\*\\s*[0-9A-Fa-f]\\s*\\*\\/$",line)
	assert m is not None,line
	return m.group(1).replace(" ","").split(",")[n % 16]

#
#						Convert a tables.h type file to a list of opcodes
#
def convert(src,tgt):
	src = [x.strip() for x in open(src).readlines()]
	#
	#		Pull out data.
	#
	eac = src[3:3+16]
	opcodes = src[23:23+16]
	cycles = src[43:43+16]
	h = open(tgt,"w")
	for n in range(0,256):
		action = extract(opcodes,n)
		if action != "nop":
			amode = extract(eac,n)
			icycles = extract(cycles,n)
			h.write("{1:3} {2:4} {3} ${0:02x}\n".format(n,action,amode,icycles))
	h.close()
#
#		Stop it being overwritten accidentally. Should be no reason to regenerate 6502.opcodes
#		The pair is so you can compare the two sets of opcodes. (dis-include the 65c02.opcodes
#		extensions first.)
#
#convert("tables.h.org","6502.opcodes")
#convert("tables.h","/tmp/6502.opcodes")	