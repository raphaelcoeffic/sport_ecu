import sys
import argparse
import struct
import serial

from time import sleep

FIELD_TIME = 0
FIELD_DATA = 1

START_BYTE = 0x7E

class SerialTelemetry_Processor:

    last_time = 0.0
    acc_data  = None
    byte_stuff = False

    def receivedPkt(self):
        print self.last_time
        return

    def processRecord(self,fields,port):

        # remove last empty field
        fields = fields[:len(fields)-1]

        data = [0x7E]
        byte_stuffing = False
        
        chk_sum = 0
        for x in fields:
            # discard empty fields in between
            if(len(x)==0):
                continue            
            data.append(int(x,16) & 0xFF)

        port.write(data)
        return
    
    def processFile(self,f,port):
        try:
            # waste the headers
            f.readline()
            for line in f:
                self.processRecord(line.split(','),port)
                #sleep(0.600)
        finally:
            f.close()

        return 0

def main (argv):
    parser = argparse.ArgumentParser(description='Fake Hornet telemetry stream')
    #parser.add_argument('serial_dev')
    parser.add_argument('serial_input', type=file)
    args = parser.parse_args()

    port = serial.Serial("/dev/tty.usbserial-DN009KYL",
                         baudrate=9600, timeout=10)

    # reset Arduino
    port.dtr = 0
    sleep(0.05)
    port.dtr = 1

    # wait for bootloader timeout
    sleep(1.0)

    print '+++'
    port.write('+++')
    print port.read(3)

    #port.flush()
    #while 1:
    #    i = int(port.read(2),16)
    #    print '0x%X: <%c>' % (i,chr(i))

    print 'ATPL4'
    port.write('ATPL4\r')
    print port.read(3)

    print 'APL2'
    port.write('APL2\r')
    print port.read(3)
        
    serial_proc = SerialTelemetry_Processor()
    return serial_proc.processFile(args.serial_input,port)

    
if __name__ == "__main__":
    main(sys.argv[1:])
