import asyncio
import traceback
import unittest
from testbench import X16TestBench

class TestTest(unittest.TestCase):
    e = None

    def __init__(self, *args, **kwargs):
        super(TestTest, self).__init__(*args, **kwargs)
        
        self.e = X16TestBench("../x16emu", ["-prg", "/Users/stefanjakobsson/Programming/x16edit/build/X16EDIT.PRG"])
        self.e.importLabels("/Users/stefanjakobsson/Programming/x16edit/build/ramlabels.txt")
        self.e.waitReady()

    def test_add(self):
        self.e.setMemory(0x6000, 0x18)   #clc
        self.e.setMemory(0x6001, 0xa9)   #lda #5
        self.e.setMemory(0x6002, 0x05)
        self.e.setMemory(0x6003, 0x69)   #adc #4
        self.e.setMemory(0x6004, 0x04)
        self.e.setMemory(0x6005, 0x60)   #rts
        self.e.run(0x6000)
        
        self.assertEqual(self.e.getA(), 9, "Should be 9")

    def test_strlen(self):
        self.e.fillMemory(0x6000, 0x60fe, 0x41)
        self.e.setMemory(0x60ff, 0)
        self.e.setX(0)
        self.e.setY(0x60)
        self.e.run(self.e.getLabelAddress("util_strlen"), 3)
        self.assertEqual(self.e.getY(), 0xff)

if __name__ == '__main__':
    unittest.main()