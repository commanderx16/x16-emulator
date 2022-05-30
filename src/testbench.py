import signal
import subprocess

class Label:
    name = ""
    address = ""

class X16TestBench:
    emu = None
    labels = []

    def __init__(self, emulatorpath, emulatoroptions=[]):
        options = [emulatorpath,"-testbench"]
        for o in emulatoroptions:
            options.append(o)

        self.emu = subprocess.Popen(options, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    
    def __del__(self):
        self.emu.stdout.close()
        self.emu.stdin.close()
        self.emu.terminate()
        self.emu.wait()

    def __writeline(self, str):
        self.emu.stdin.write((str + "\n").encode('utf-8'))
        self.emu.stdin.flush()

    def __readline(self):
        return self.emu.stdout.readline().decode('utf-8')

    def __getresponse(self):
        r = ""
        while r == "":
            r = self.__readline()
        
        if r[0:3] == "ERR":
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

    def importLabels(self, path):
        f = open(path, "r")
        for line in f.readlines():
            l = Label()
            addr_start = line.find(" ") + 1
            addr_len = line[addr_start:].find(" ")
            label_start = addr_start + addr_len + line[addr_start+addr_len:].find(".") + 1
            l.name = line[label_start:-1] 
            l.address = self.__toint16(line[addr_start:addr_start+addr_len])
            self.labels.append(l)
        f.close()

    def getLabelAddress(self, labelName):
        for l in self.labels:
            if l.name == labelName:
                return l.address
        raise Exception("Label not found")

    def waitReady(self, timeout=5):
        #Set timeout
        signal.signal(signal.SIGALRM, self.__timeout)
        signal.alarm(timeout)

        r = ""
        while r != "RDY\n":
            r = self.__readline()

    def setRamBank(self, value):
        self.__writeline("RAM " + self.__tohex8(value))

    def setRomBank(self, value):
        self.__writeline("ROM " + self.__tohex8(value))
    
    def setMemory(self, address, value):
        self.__writeline("STM " + self.__tohex16(address) + " " + self.__tohex8(value))

    def fillMemory(self, address1, address2, value):
        self.__writeline("FLM " + self.__tohex16(address1) + " " + self.__tohex16(address2) + " " + self.__tohex8(value))

    def setA(self, value):
        self.__writeline("STA " + self.__tohex8(value))

    def setX(self, value):
        self.__writeline("STX " + self.__tohex8(value))

    def setY(self, value):
        self.__writeline("STY " + self.__tohex8(value))

    def setStatus(self, value):
        self.__writeline("SST " + self.__tohex8(value))
    
    def setStackPointer(self, value):
        self.__writeline("SSP " + self.__tohex8(value))

    def run(self, address, timeout=5):
        #Set timeout
        signal.signal(signal.SIGALRM, self.__timeout)
        signal.alarm(timeout)

        #Run code
        self.__writeline("RUN " + self.__tohex16(address))
        self.waitReady()

    def getMemory(self, address):
        self.__writeline("RQM " + self.__tohex16(address))
        return self.__int8(self.__getresponse())

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
