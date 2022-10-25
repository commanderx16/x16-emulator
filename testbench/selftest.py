import unittest
from testbench import X16TestBench
from testbench import Status

class SelfTest(unittest.TestCase):
    e = None

    def __init__(self, *args, **kwargs):
        super(SelfTest, self).__init__(*args, **kwargs)
        
        self.e = X16TestBench("../x16emu", ["-warp"])
        self.e.waitReady()

    def test_rombank(self):
        self.e.setRomBank(0)
        self.assertEqual(self.e.getMemory(1),0)

        self.e.setRomBank(10)
        self.assertEqual(self.e.getMemory(1),10)

    def test_rambank(self):
        self.e.setRamBank(0)
        self.assertEqual(self.e.getMemory(0),0)

        self.e.setRamBank(10)
        self.assertEqual(self.e.getMemory(0),10)

    def test_mem(self):
        #Set and retrieve value 0
        self.e.setMemory(0x6000, 0)
        self.assertEqual(self.e.getMemory(0x6000), 0)

        #Set and retrieve value 255
        self.e.setMemory(0x6000, 255)
        self.assertEqual(self.e.getMemory(0x6000), 255)

        #Set value -1, should cause an exception
        with self.assertRaises(Exception):
            self.e.setMemory(0x6000, -1)

        #Set value 256, should cause an exception
        with self.assertRaises(Exception):
            self.e.setMemory(0x6000, 256)

        #Use address -1, should cause an exception
        with self.assertRaises(Exception):
            self.e.setMemory(-1, 0)
        
        #Use address 10000, should cause an exception
        with self.assertRaises(Exception):
            self.e.setMemory(0x10000, 0)

    def test_fillmemory(self):
        self.e.fillMemory(0x6000, 0x61ff, 0x80)
        self.e.fillMemory(0x6001, 0x61fe, 0xff)
        
        self.assertEqual(self.e.getMemory(0x6000), 0x80)
        self.assertEqual(self.e.getMemory(0x6001), 0xff)
        self.assertEqual(self.e.getMemory(0x61fe), 0xff)
        self.assertEqual(self.e.getMemory(0x61ff), 0x80)

    def test_a(self):
        #Set and retrieve value 0
        self.e.setA(0)
        self.assertEqual(self.e.getA(), 0)

        #Set and retrieve value 255
        self.e.setA(255)
        self.assertEqual(self.e.getA(), 255)

        #Set value -1, should cause an exception
        with self.assertRaises(Exception):
            self.e.setA(-1)

        #Set value 256, should cause an exception
        with self.assertRaises(Exception):
            self.e.setA(256)

    def test_x(self):
        #Set and retrieve value 0
        self.e.setX(0)
        self.assertEqual(self.e.getX(), 0)

        #Set and retrieve value 255
        self.e.setX(255)
        self.assertEqual(self.e.getX(), 255)

        #Set value -1, should cause an exception
        with self.assertRaises(Exception):
            self.e.setX(-1)

        #Set value 256, should cause an exception
        with self.assertRaises(Exception):
            self.e.setX(256)

    def test_y(self):
        #Set and retrieve value 0
        self.e.setY(0)
        self.assertEqual(self.e.getY(), 0)

        #Set and retrieve value 255
        self.e.setY(255)
        self.assertEqual(self.e.getY(), 255)

        #Set value -1, should cause an exception
        with self.assertRaises(Exception):
            self.e.setY(-1)

        #Set value 256, should cause an exception
        with self.assertRaises(Exception):
            self.e.setY(256)

    def test_stackpointer(self):
        #Set and retrieve value 0
        self.e.setStackPointer(0)
        self.assertEqual(self.e.getStackPointer(), 0)

        #Set and retrieve value 255
        self.e.setStackPointer(255)
        self.assertEqual(self.e.getStackPointer(), 255)

        #Set value -1, should cause an exception
        with self.assertRaises(Exception):
            self.e.setStackPointer(-1)

        #Set value 256, should cause an exception
        with self.assertRaises(Exception):
            self.e.setStackPointer(256)

    def test_statusregister(self):
        #Set and retrieve value 0
        self.e.setStatus(0)
        self.assertEqual(self.e.getStatus(), 0)

        #Set and retrieve value 255
        self.e.setStatus(255)
        self.assertEqual(self.e.getStatus(), 255)

        #Set value -1, should cause an exception
        with self.assertRaises(Exception):
            self.e.setStatus(-1)

        #Set value 256, should cause an exception
        with self.assertRaises(Exception):
            self.e.setStatus(256)

    def test_carryflag(self):
        self.e.setStatus(self.e.getStatus() & (255-Status.C))

        self.e.setMemory(0x6000, 0x38)      #sec
        self.e.setMemory(0x6001, 0x60)      #rts
        self.e.run(0x6000)
        self.assertEqual(self.e.getStatus() & Status.C, Status.C)

        self.e.setMemory(0x6000, 0x18)      #clc
        self.e.run(0x6000)
        self.assertNotEqual(self.e.getStatus() & Status.C, Status.C)

    def test_zeroflag(self):
        #Clear zero flag
        self.e.setStatus(self.e.getStatus() & (255-Status.Z))

        #Run code that sets zero flag
        self.e.setMemory(0x6000, 0xa9)      #lda #0
        self.e.setMemory(0x6001, 0x00)
        self.e.setMemory(0x6002, 0x60)      #rts
        self.e.run(0x6000)
        self.assertEqual(self.e.getStatus() & Status.Z, Status.Z)

        #Run code that clears zero flag
        self.e.setMemory(0x6000, 0xa9)      #lda #1
        self.e.setMemory(0x6001, 0x01)
        self.e.setMemory(0x6002, 0x60)      #rts
        self.e.run(0x6000)
        self.assertNotEqual(self.e.getStatus() & Status.Z, Status.Z)

    def test_overflowflag(self):
        #Clear carry and overflow flags
        self.e.setStatus(self.e.getStatus() & (255-Status.C-Status.V))

        #Run code that doesn't cause overflow
        self.e.setMemory(0x6000, 0xa9)      #lda #1
        self.e.setMemory(0x6001, 0x01)
        self.e.setMemory(0x6002, 0x69)      #adc #1
        self.e.setMemory(0x6003, 0x01)
        self.e.setMemory(0x6004, 0x60)      #rts
        self.e.run(0x6000)
        self.assertNotEqual(self.e.getStatus() & Status.V, Status.V)

        #Run code that causes overflow
        self.e.setMemory(0x6000, 0xa9)      #lda #80
        self.e.setMemory(0x6001, 0x80)
        self.e.setMemory(0x6002, 0x69)      #adc #ff
        self.e.setMemory(0x6003, 0xff)
        self.e.setMemory(0x6004, 0x60)      #rts
        self.e.run(0x6000)
        self.assertEqual(self.e.getStatus() & Status.V, Status.V)

    def test_negativeflag(self):
        #Clear negative flag
        self.e.setStatus(self.e.getStatus() & (255-Status.N))

        #Run code that should set negative flag
        self.e.setMemory(0x6000, 0x38)      #sec
        self.e.setMemory(0x6001, 0xa9)      #lda #0
        self.e.setMemory(0x6002, 0x00)
        self.e.setMemory(0x6003, 0xe9)      #sbc #1
        self.e.setMemory(0x6004, 0x01)
        self.e.setMemory(0x6005, 0x60)      #rts
        self.e.run(0x6000)
        self.assertEqual(self.e.getStatus() & Status.N, Status.N)

        #Run code that should clear the negative flag
        self.e.setMemory(0x6000, 0x38)      #sec
        self.e.setMemory(0x6001, 0xa9)      #lda #ff
        self.e.setMemory(0x6002, 0xff)
        self.e.setMemory(0x6003, 0xe9)      #sbc #81
        self.e.setMemory(0x6004, 0x81)
        self.e.setMemory(0x6005, 0x60)      #rts
        self.e.run(0x6000)
        self.assertNotEqual(self.e.getStatus() & Status.N, Status.N)

    def test_run(self):
        #Use address -1, should cause an exception
        with self.assertRaises(Exception):
            self.e.run(-1)
        
        #Use address 10000, should cause an exception
        with self.assertRaises(Exception):
            self.e.run(0x10000)

if __name__ == '__main__':
    unittest.main()