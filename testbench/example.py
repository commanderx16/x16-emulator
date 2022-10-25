import unittest
from testbench import X16TestBench
from testbench import Status

class TestTest(unittest.TestCase):
    e = None

    def __init__(self, *args, **kwargs):
        super(TestTest, self).__init__(*args, **kwargs)
        
        self.e = X16TestBench("../x16emu")
        self.e.waitReady()

    def test_add(self):
        self.e.setMemory(0x6000, 0x18)   #clc
        self.e.setMemory(0x6001, 0xa9)   #lda #ff
        self.e.setMemory(0x6002, 0xff)
        self.e.setMemory(0x6003, 0x69)   #adc #1
        self.e.setMemory(0x6004, 0x01)
        self.e.setMemory(0x6005, 0x60)   #rts
        self.e.run(0x6000)
        
        self.assertEqual(self.e.getA(), 0)
        self.assertNotEqual(self.e.getStatus() & Status.C, 0)
        
if __name__ == '__main__':
    unittest.main()