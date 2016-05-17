import sys
import argparse
import struct

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

    def processRecord(self,fields):

        # remove last empty field
        fields = fields[:len(fields)-1]

        data = []
        byte_stuffing = False
        
        chk_sum = 0
        for x in fields:
            # discard empty fields in between
            if(len(x)==0):
                continue
            
            b = int(x,16)
            if b == 0x7D:
                byte_stuffing = True
            else:
                
                if byte_stuffing:
                    b ^= 0x20
                    byte_stuffing = False
                data.append(b)

                chk_sum += b
                chk_sum &= 0xFF
                
        #print ' '.join( (format(int(x,16), '002X') for x in fields) )
        #print ' '.join( (format(x, '002X') for x in data) ) + (' (%X)' % chk_sum)

        if chk_sum != 0x24:
            #print '## bad frame ##'
            return

        if len(fields) < 20:
            #print '## packtet too small ##'
            return

        thro = data[14]
        rpm  = data[8] | (data[9] << 8)
        egt  = data[10] | (data[11] << 8)
        vbat = data[12]
        vpmp = data[13]
        fuel = data[15] | (data[16] << 8)
        status = data[19]


        #print 'data[7]=%X;data[8]=%X;data[9]=%X' % (data[7],data[8],data[9])
        print 'th=%i; rpm=%i; egt=%i; vbat=%i; vpmp=%i; fuel=%i; status=%i; chk=%x' % (thro,rpm,egt,vbat,vpmp,fuel,status,chk_sum)
        return
    
    def processFile(self,f):
        try:
            # waste the headers
            f.readline()
            for line in f:
                self.processRecord(line.split(','))
            #self.processRecord(None)
        finally:
            f.close()

        return 0

def main (argv):
    parser = argparse.ArgumentParser(description='Decode Hornet telemetry stream')
    parser.add_argument('serial_log', type=file)
    args = parser.parse_args()

    serial_proc = SerialTelemetry_Processor()
    return serial_proc.processFile(args.serial_log)

    
if __name__ == "__main__":
    main(sys.argv[1:])
