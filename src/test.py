import unittest
from testbench import X16TestBench

class TestTest(unittest.TestCase):
    def addtest(self):
        e = X16TestBench("../x16emu")
        e.waitReady()
        
        e.setMemory(0x6000, 0x18)   #clc
        e.setMemory(0x6001, 0xa9)   #lda #5
        e.setMemory(0x6002, 0x05)
        e.setMemory(0x6003, 0x69)   #adc #4
        e.setMemory(0x6004, 0x04)
        e.setMemory(0x6005, 0x60)   #rts
        e.run(0x6000)

        self.assertEqual(e.getA(), 9)

unittest.main()