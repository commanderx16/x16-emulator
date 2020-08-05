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
MNEMONICS_DISASSEM_HEADER = "static const char *mnemonics[256] = {"
TABLE_MAP = "/*{0:8}|  0  |  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  A  |  B  |  C  |  D  |  E  |  F  |{0:5}*/\n"

#####################################
######### OPCODE CONSTANTS ##########
EMPTY_NOP_ACTN = "nop"
EMPTY_NOP_CYCLS = "2"
EMPTY_NOP_MOD = "imp"
OPCODE_ROW_LEN = 16
TOTAL_NUMBER_OPCODES = 2 ** 8

#####################################
############# FILENAMES #############
TABLES_HEADER_FNAME = "tables.h"
MNEMONICS_DISASSEM_HEADER_FNAME = "mnemonics.h"
OPCODES_6502_FNAME = "6502.opcodes"
OPCODES_65c02_FNAME = "65c02.opcodes"


#####################################
#####################################


#######################################################################################################################
####################################  Load in a source file with opcode descriptors  ##################################
#######################################################################################################################
def loadSource(srcFile):
    # Read file line by line and omit committed
    fileLinesObject = open(srcFile).readlines()
    strippedOpcodeLines = [opcodeLine.strip() for opcodeLine in fileLinesObject if
                           opcodeLine.strip() != "" and not opcodeLine.startswith(";")]

    # Import Opcodes
    for opcodeLine in strippedOpcodeLines:
        matchLine = re.match(OPCODE_REGEX_STR, opcodeLine)
        assert matchLine is not None, "Format {}".format(opcodeLine)

        opcode = int(matchLine.group(OPCODE_KEY_STR), 16)
        assert opcodesList[opcode] is None, "Duplicate {0:02x}".format(opcode)

        opcodesList[opcode] = {
            MODE_KEY_STR: matchLine.group(MODE_KEY_STR),
            ACTN_KEY_STR: matchLine.group(ACTN_KEY_STR),
            CYCLES_KEY_STR: matchLine.group(CYCLES_KEY_STR),
            OPCODE_KEY_STR: opcode
        }


#######################################################################################################################
#############################################  Fill unused slots with NOPs  ###########################################
#######################################################################################################################
def fillNop():
    for i in range(0, TOTAL_NUMBER_OPCODES):
        if opcodesList[i] is None:
            opcodesList[i] = {
                MODE_KEY_STR: EMPTY_NOP_MOD,
                ACTN_KEY_STR: EMPTY_NOP_ACTN,
                CYCLES_KEY_STR: EMPTY_NOP_CYCLS,
                OPCODE_KEY_STR: i
            }


#######################################################################################################################
###################################################  Output a table  ##################################################
#######################################################################################################################
def generateTable(hFileName, header, element):
    spacing = 5
    hFileName.write("{}{}{}".format("\n", header, "\n"))
    hFileName.write(TABLE_MAP.format("", ""))
    for row in range(0, OPCODE_ROW_LEN):
        elements = ",".join(
            ["{0:>9}".format(opcodesList[opcodeItem][element])[-spacing:] for opcodeItem in
             range(row * OPCODE_ROW_LEN, OPCODE_ROW_LEN * (row + 1))])

        hFileName.write("/* {0:X} */ {3}  {1}{2} /* {0:X} */\n".format(
            row,
            elements,
            " " if row == OPCODE_ROW_LEN - 1 else ",",
            " " if element == ACTN_KEY_STR else "")
        )
    hFileName.write("};\n")



#######################################################################################################################
###################################################  Output a list   ##################################################
#######################################################################################################################
def generateList(hFileName, header, elements):
    hFileName.write("{}{}{}".format("\n", header, "\n"))
    for row in range(0, OPCODE_ROW_LEN):
        hFileName.write("\t// ${0:X}X\n".format(row))

        for op in range(0, OPCODE_ROW_LEN):
            hFileName.write("\t/* ${0:X}{1:X} */ \"{2}\"{3}".format(row, op, elements[row * OPCODE_ROW_LEN + op], "" if row == OPCODE_ROW_LEN-1 and op == OPCODE_ROW_LEN-1 else ",\n"))
        
        hFileName.write("{}".format("" if row == OPCODE_ROW_LEN-1 else "\n"))
    hFileName.write("};\n")



#######################################################################################################################
########################################  Convert opcode structure to mnemonic  #######################################
#######################################################################################################################
def convertMnemonic(opInfo):
    modeStr = {
        "imp": "",
        "imm": "#$%02x",
        "zp": "$%02x",
        "rel": "$%02x",
        "zprel": "$%02x, $%04x",
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
    return "{} {}".format(opInfo[ACTN_KEY_STR], modeStr[opInfo[MODE_KEY_STR]])


#######################################################################################################################
##################################################  Load in opcodes  ##################################################
#######################################################################################################################
if __name__ == '__main__':
    opcodesList = [None] * TOTAL_NUMBER_OPCODES
    # Load 6502 opcodes list
    loadSource(OPCODES_6502_FNAME)
    # Load 65C02 opcodes list
    loadSource(OPCODES_65c02_FNAME)
    # Fill opcodes list with NOP instructions
    fillNop()

    # Create "TABLES_HEADER_FNAME" header file
    with open(TABLES_HEADER_FNAME, "w") as output_h_file:
        output_h_file.write("/* Generated by buildtables.py */\n")
        generateTable(output_h_file, ADDR_MODE_HEADER, MODE_KEY_STR)
        generateTable(output_h_file, ACTN_CODE_HEADER, ACTN_KEY_STR)
        generateTable(output_h_file, MCHN_CYCLES_HEADER, CYCLES_KEY_STR)

    # Create disassembly "MNEMONICS_DISASSEM_HEADER_FNAME" header file.
    mnemonics = [convertMnemonic(opcodesList[x]) for x in range(0, TOTAL_NUMBER_OPCODES)]
    with open(MNEMONICS_DISASSEM_HEADER_FNAME, "w") as output_h_file:
        output_h_file.write("/* Generated by buildtables.py */\n")
        generateList(output_h_file, MNEMONICS_DISASSEM_HEADER, mnemonics)
