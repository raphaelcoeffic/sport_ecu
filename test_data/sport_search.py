import sys
import argparse
import struct

# Logic I2C CSV format
FIELD_TIME = 0
FIELD_DATA = 1

START_BYTE = 0x7E
SENSOR_ID = 0x1B

class SPORT_CSV_Processor:
    
    last_time = -0.0051
    acc_data  = None

    def receivedPkt(self):
        if self.acc_data[0] != SENSOR_ID:
            return

        if len(self.acc_data) <= 2:
            print self.acc_data
            return

        raw_data = self.acc_data[0:9]
        (sensor_id,tag,value_id,value,crc) = struct.unpack('<BBHIB',raw_data)
        print '%f\t[id=0x%X; value=%u]' % (self.last_time,value_id,value)

    def processRecord(self,fields):

        if (fields is None):
            self.receivedPkt()
            return

        pk_time = float(fields[FIELD_TIME])
        pk_data = (int(fields[FIELD_DATA],16))

        if (pk_time - self.last_time > 0.005) and (pk_data == START_BYTE):
            if (self.acc_data is not None):
                self.receivedPkt()
            self.last_time = pk_time
            self.acc_data = bytearray()
            return
        
        self.acc_data.append(pk_data)
        return
    
    def processFile(self,f):
        try:
            # waste the headers
            f.readline()
            for line in f:
                self.processRecord(line.split(','))
            self.processRecord(None)
        finally:
            f.close()

        return 0


def main (argv):
    parser = argparse.ArgumentParser(description='Decode SPORT stream')
    parser.add_argument('csv_file', type=file)
    args = parser.parse_args()

    sport_csv = SPORT_CSV_Processor()
    return sport_csv.processFile(args.csv_file)

    
if __name__ == "__main__":
    main(sys.argv[1:])
