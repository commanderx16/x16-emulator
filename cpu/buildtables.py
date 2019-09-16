# *******************************************************************************************
# *******************************************************************************************
#
#		File:			buildtables.py
#		Date:			3rd September 2019
#		Purpose:		Creates files tables.h from the .opcodes descriptors
#						Creates disassembly include file.
#		Author:			Paul Robson (paul@robson.org.uk)
#		Formatted By: 	Jeries Abedrabbo (jabedrabbo@asaltech.com)
#
# *******************************************************************************************
# *******************************************************************************************

import re

#
#							Load in a source file with opcode descriptors
#
############# CONSTANTS ##############
######################################
########### KEY CONSTANTS ############
ACTN_KEY_STR = "action"
CYCLES_KEY_STR = "cycles"
MODE_KEY_STR = "mode"
OPCODE_KEY_STR = "opcode"
######################################
########### REGEX CONSTANTS ##########
OPCODE_REGEX_STR = "^(?P<{actnCode}>\\w+)\\s+(?P<{addrMode}>\\w+)\\s+(?P<{mchnCycles}>\\d)\\s+\\$(?P<{opCode}>[0-9a-fA-F]+)$".format(
    actnCode=ACTN_KEY_STR,
    addrMode=MODE_KEY_STR,
    mchnCycles=CYCLES_KEY_STR,
    opCode=OPCODE_KEY_STR
)
#####################################
########## HEADER CONSTANTS #########
ADDR_MODE_HEADER = "static void (*addrtable[256])() = {"
ACTN_CODE_HEADER = "static void (*optable[256])() = {"
MCHN_CYCLES_HEADER = "static const uint32_t ticktable[256] = {"
#####################################
#####################################

def loadSource(srcFile):
    #########################
    #		Load file		#
    #########################
    src = [opcodeLine.strip() for opcodeLine in open(srcFile).readlines() if opcodeLine.strip() != ""]
    src = [opcodeLine for opcodeLine in src if not opcodeLine.startswith(";")]
    ##############################
    #		Import opcodes		 #
    ##############################
    for l in src:
        m = re.match(OPCODE_REGEX_STR, l)
        assert m is not None, "Format " + l
        opcode = int(m.group(OPCODE_KEY_STR), 16)
        assert opcodesList[opcode] is None, "Duplicate {0:02x}".format(opcode)
        opcodesList[opcode] = {
            MODE_KEY_STR: m.group(MODE_KEY_STR),
            ACTN_KEY_STR: m.group(ACTN_KEY_STR),
            CYCLES_KEY_STR: m.group(CYCLES_KEY_STR),
            OPCODE_KEY_STR: opcode
        }


#################################################################################################
#									Fill unused slots with NOPs									#
#################################################################################################
def fillNop():
    for i in range(0, 256):
        if opcodesList[i] is None:
            opcodesList[i] = {
                MODE_KEY_STR: "imp",
                ACTN_KEY_STR: "nop",
                CYCLES_KEY_STR: "2",
                OPCODE_KEY_STR: i
            }


#
#										  Output a table
#
def generateTable(h, header, element):
    spacing = 5
    h.write("{}{}{}".format("\n", header, "\n"))
    h.write(
        "/*{0:8}|  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |{0:5}*/\n".format(
            "", ""))
    for row in range(0, 16):
        elements = ",".join([("         " + opcodesList[opcodeItem][element])[-spacing:] for opcodeItem in
                             range(row * 16, row * 16 + 16)])
        h.write("/* {0:X} */ {3}  {1}{2} /* {0:X} */\n".format(row, elements, " " if row == 15 else ",",
                                                               " " if element == ACTN_KEY_STR else ""))
    h.write("};\n")


#
#							Convert opcode structure to mnemonic
#	
def convertMnemonic(opInfo):
    modeStr = {
        "imp": "",
        "imm": "#$%02x",
        "zp": "$%02x",
        "rel": "$%02x",
        "zpx": "$%02x,x",
        "zpy": "$%02x,y",
        "abso": "$%04x",
        "absx": "$%04x,x",
        "absy": "$%04x,y",
        "ainx": "($%04x,x)",
        "indy": "($%02x),y",
        "indx": "($%02x,x)",
        "ind": "($%04x)",
        "ind0": "($%02x)",
        "acc": "a"
    }
    return opInfo[ACTN_KEY_STR] + " " + modeStr[opInfo[MODE_KEY_STR]]


#
#										Load in opcodes
#
if __name__ == '__main__':
    opcodesList = [None] * 256
    loadSource("6502.opcodes")
    loadSource("65c02.opcodes")
    fillNop()
    #
    #										Create tables.h
    #
    with open("tables.h", "w") as output_h_file:
        output_h_file.write("/* Generated by buildtables.py */\n")
        generateTable(output_h_file, ADDR_MODE_HEADER, MODE_KEY_STR)
        generateTable(output_h_file, ACTN_CODE_HEADER, ACTN_KEY_STR)
        generateTable(output_h_file, MCHN_CYCLES_HEADER, CYCLES_KEY_STR)
    #
    #										Create disassembly file.
    #
    mnemonics = [convertMnemonic(opcodesList[x]) for x in range(0, 256)]
    mnemonics = ",".join(['"' + x + '"' for x in mnemonics])
    with open("mnemonics.h", "w") as output_h_file:
        # output_h_file = open("mnemonics.h", "w")
        output_h_file.write("/* Generated by buildtables.py */\n")
        output_h_file.write("static char *mnemonics[256] = {{ {0} }};\n".format(mnemonics))
