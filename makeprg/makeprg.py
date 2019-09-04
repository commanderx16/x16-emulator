# *******************************************************************************************
# *******************************************************************************************
#
#		Name : 		makeprg.py
#		Purpose :	Convert ASCII code to .PRG file.
#		Date :		15th August 2019
#		Author : 	Paul Robson (paul@robsons.org.uk)
#
# *******************************************************************************************
# *******************************************************************************************

import re,sys,os

# *******************************************************************************************
#
#		This is the output from gentokenlist.py. It's done this way so you don't have
#		to have the ROM file present.
#
# *******************************************************************************************

def getTokenRaw():
	return """	
	80.END::81.FOR::82.NEXT::83.DATA::84.INPUT#::85.INPUT::86.DIM::87.READ::88.LET::
	89.GOTO::8a.RUN::8b.IF::8c.RESTORE::8d.GOSUB::8e.RETURN::8f.REM::90.STOP::
	91.ON::92.WAIT::93.LOAD::94.SAVE::95.VERIFY::96.DEF::97.POKE::98.PRINT#::
	99.PRINT::9a.CONT::9b.LIST::9c.CLR::9d.CMD::9e.SYS::9f.OPEN::a0.CLOSE::
	a1.GET::a2.NEW::a3.TAB(::a4.TO::a5.FN::a6.SPC(::a7.THEN::a8.NOT::a9.
	STEP::aa.+::ab.-::ac.*::ad./::ae.^::af.AND::b0.OR::b1.>::b2.=::b3.<::b4.SGN::
	b5.INT::b6.ABS::b7.USR::b8.FRE::b9.POS::ba.SQR::bb.RND::bc.LOG::bd.EXP::
	be.COS::bf.SIN::c0.TAN::c1.ATN::c2.PEEK::c3.LEN::c4.STR$::c5.VAL::
	c6.ASC::c7.CHR$::c8.LEFT$::c9.RIGHT$::ca.MID$::cb.GO::
	ce80.MON::ce81.DOS::ce82.VPOKE::ce83.VPEEK
	"""
# *******************************************************************************************
#
#									Tokeniser Class
#
# *******************************************************************************************

class Tokeniser(object):
	def __init__(self):
		self.tokens = {}										# tokens name => token
		self.loadRawTokens()									# parse the token data
		self.program = []										# code goes here
	#
	#								Load Raw tokens
	#
	def loadRawTokens(self):
		src = getTokenRaw().replace("\t","")					# preprocess
		src = src.replace("\n","").replace(" ","")
		self.longest = 0
		for t in [x for x in src.split("::") if x != ""]:		# split by ::
			t2 = t.split(".")									# split by .
			self.tokens[t2[1]] = int(t2[0],16) 					# set up dictionary
			self.longest = max(self.longest,len(t2[1]))			# get longest token.
	#
	#								Add a code line.
	#
	def addLine(self,lineNumber,tokenisedLine):
		nextLine = len(self.program)+len(tokenisedLine)+0x806
		self.program.append(nextLine & 0xFF)	
		self.program.append(nextLine >> 8)	
		self.program.append(lineNumber & 0xFF)
		self.program.append(lineNumber >> 8)
		self.program += tokenisedLine
		self.program.append(0)
	#
	#				Tokenise line.
	#
	def tokeniseLine(self,code):
		self.line = []
		code = code.strip().upper()								# remove l/t spaces.
		while code != "": 										# & process everything.
			code = self.tokeniseOne(code)
		return self.line
	#
	#					Tokenise one element
	#		
	def tokeniseOne(self,c):
		if c.startswith('"'):									# quoted string.
			c = c[1:]											# chuck quote.
			p = c.find('"') if c.find('"') >= 0 else len(c)		# get split point
			self.line.append(34)
			for ch in c[:p]:
				self.line.append(ord(ch))
			self.line.append(34)
			return c[p+1:]

		for size in range(self.longest,0,-1):					# work back from longest
			if c[:size] in self.tokens:							# keyword found.
				tokenID = self.tokens[c[:size]]					# get token ID.
				if tokenID > 0xFF:								# output token.
					self.line.append(tokenID >> 8)
				self.line.append(tokenID & 0xFF)
				return c[size:]

		self.line.append(ord(c[0]))								# straight character.
		return c[1:]

	#
	#				Add header and end, and save.
	#
	def save(self,fileName):
		self.program.append(0)									# 0 link => end.
		self.program.append(0)				
		self.program.insert(0,0x01) 							# insert load address.
		self.program.insert(1,0x08)
		h = open(fileName,"wb") 								# Open file
		h.write(bytes(self.program))							# Python 3
		h.close()
		print("Written {0} ({1} bytes)".format(fileName,len(self.program)))
	#
	#				Parse a file.
	#
	def parse(self,source):
		if not os.path.isfile(source):
			print("File '{0}' not available".format(source))
			sys.exit(1)
		for l in [x.strip() for x in open(source).readlines()]:	# read program
			if not l.startswith(";") and l != "":				# not blank or comment
				m = re.match("^(\\d+)\\s+(.*)$",l)				# bad line ?
				if m is None:
					print("Bad code line ["+l+"]")
					sys.exit(2)
				self.addLine(int(m.group(1)),self.tokeniseLine(m.group(2)))

if len(sys.argv) != 3:
	print("python makeprg.py [source file] [prg.file]")				
	print("\tWritten by Paul Robson 2019 v1.0")
	print("\t\tTo run produced PRG file ./x16emu -prg [prg.file]")
	sys.exit(3)
t = Tokeniser()
t.parse(sys.argv[1])
t.save(sys.argv[2])
