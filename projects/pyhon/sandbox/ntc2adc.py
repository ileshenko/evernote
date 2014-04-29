"""
convert ntc file (voltage on thermoresistor) to ADC code
calculation adc = v/3.3*255
write to stdout 
"""

import sys

NUMBER_FORMAT = '{0:#04x}'

def main():
    if len(sys.argv) != 2:
        print 'Usage: {0} [input-file]'.format(sys.argv[0])
    filename = sys.argv[1]

    with open(filename, 'r') as ntc:
        prev_voltage = float(ntc.readline())
        for line in ntc:
            if not line.strip():
                continue
            voltage = float(line)

            mean = (prev_voltage + voltage) / 2
            result = int(mean / 3.3 * 0xff)
            print NUMBER_FORMAT.format(result)

            prev_voltage = voltage

        print NUMBER_FORMAT.format(0)

if __name__ == '__main__':
    main()
