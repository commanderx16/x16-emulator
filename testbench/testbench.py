import signal
import subprocess

class Status:
    C = 1
    Z = 2
    I = 4
    D = 8
    V = 64
    N = 128

class X16TestBench:
    emu = None
    labels = dict()

    """
    Class constructor.

    Args:
        emulatorpath: Path to the x16emu executable
        emulatoroptions: A list of launch options used when starting the emulator

    Raises:
        FileNotFoundError: Happens if emulatorpath not valid
    """
    def __init__(self, emulatorpath, emulatoroptions=[]):
        path = [emulatorpath, "-testbench"]
        for o in emulatoroptions:
            path.append(o)
        
        self.emu = subprocess.Popen(path, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    
    def __del__(self):
        self.emu.stdout.close()
        self.emu.stdin.close()
        self.emu.terminate()
        self.emu.wait()

    """
    Waits until the emulator has sent the ready signal ("RDY").

    Args:
        timeout: Number of seconds until abort waiting

    Raises:
        Exception: If aborted by timeout
    """
    def waitReady(self, timeout=5):
        #Set timeout
        signal.signal(signal.SIGALRM, self.__timeout)
        signal.alarm(timeout)

        r = ""
        while r.startswith("RDY")==False:
            r = self.__readline()
            if r.startswith("ERR"):
                raise Exception(r[3:].strip())

    def __writeline(self, str):
        self.emu.stdin.write((str + "\n").encode('utf-8'))
        self.emu.stdin.flush()

    def __readline(self):
        return self.emu.stdout.readline().decode('utf-8')

    def __getresponse(self):
        r = ""
        while r == "":
            r = self.__readline()
        
        if r.startswith("ERR"):
            raise Exception("Invalid command")
        else:
            return r

    def __tohex8(self, val):
        if val < 0 or val > 255:
            raise Exception("Value out of range")
        else:
            return "%0.2X" % val

    def __tohex16(self, val):
        if val < 0 or val > 65535:
            raise Exception("Value out of range")
        else:
            return "%0.4X" % val

    def __toint8(self, val):
        i = int(val, 16)
        if i < 0 or i > 255:
            raise Exception("Value out of range")
        else:
            return i
    
    def __toint16(self, val):
        i = int(val, 16)
        if i < 0 or i > 65535:
            raise Exception("Value out of range")
        else:
            return i

    def __timeout(self, s, f):
        raise Exception("Timeout error")

    """
    Opens and reads a text file in VICE format, to import label names 
    and corresponding memory addresses.

    Params:
        path: Path to the label file

    Raises:
        FileNotFoundError
    """
    def importViceLabels(self, path):
        f = open(path, "r")
        for line in f.readlines():
            fields = line.split(" ")
            if len(fields) >= 3:
                self.labels[fields[2][1:].strip()] = self.__toint16(fields[1].strip())
        f.close()

    def setRamBank(self, value):
        self.__writeline("RAM " + self.__tohex8(value))
        self.waitReady()

    def setRomBank(self, value):
        self.__writeline("ROM " + self.__tohex8(value))
        self.waitReady()
    
    def setMemory(self, address, value):
        self.__writeline("STM " + self.__tohex16(address) + " " + self.__tohex8(value))
        self.waitReady()

    def fillMemory(self, address1, address2, value):
        self.__writeline("FLM " + self.__tohex16(address1) + " " + self.__tohex16(address2) + " " + self.__tohex8(value))
        self.waitReady()

    def setA(self, value):
        self.__writeline("STA " + self.__tohex8(value))
        self.waitReady()

    def setX(self, value):
        self.__writeline("STX " + self.__tohex8(value))
        self.waitReady()

    def setY(self, value):
        self.__writeline("STY " + self.__tohex8(value))
        self.waitReady()

    def setStatus(self, value):
        self.__writeline("SST " + self.__tohex8(value))
        self.waitReady()
    
    def setStackPointer(self, value):
        self.__writeline("SSP " + self.__tohex8(value))
        self.waitReady()

    def run(self, address, timeout=5):
        self.__writeline("RUN " + self.__tohex16(address))
        self.waitReady(timeout)

    def getMemory(self, address):
        self.__writeline("RQM " + self.__tohex16(address))
        return self.__toint8(self.__getresponse())

    def getA(self):
        self.__writeline("RQA")
        return self.__toint8(self.__getresponse())

    def getX(self):
        self.__writeline("RQX")
        return self.__toint8(self.__getresponse())

    def getY(self):
        self.__writeline("RQY")
        return self.__toint8(self.__getresponse())

    def getStatus(self):
        self.__writeline("RST")
        return self.__toint8(self.__getresponse())

    def getStackPointer(self):
        self.__writeline("RSP")
        return self.__toint8(self.__getresponse())
