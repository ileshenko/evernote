#To be able to read csv formated files, we will first have to import the
#csv module.

import csv

# status
# 0 - idle

status = 0
arreg = 0
arval = 0

output_file = open('ar8337_reg_out.txt', 'w')

def proc_high(rw, reg, val):
	global status
	global arreg
	global arval

	if bool(reg & 0x01) :
		val <<= 16
	arval |= val

	output_file.write("ar8337 %s %#06x %#010x\n" % (rw, arreg, arval))
	status = 0


def proc_low(phy, reg, val):
	global status
	global arreg
	global arval

	arreg |= (phy & 0xf)<<6 
	arreg |= (reg & 0x1e)<<1

	if bool(reg & 0x01) :
		val <<= 16
	arval |= val
	status = 2

def proc_start(phy, reg, val):
	global status
	global arreg
	global arval

	arreg = val<<9
	arval = 0
	status = 1


def process_line(ts, rw, phy, reg, val):
	global status

	is_write = rw == 'wr'

	if status == 0:
		output_file.write(ts + " ")

		if not is_write or phy != 0x18:
			output_file.write("phy   %s %#04x %#04x %#06x\n" % (rw, phy, reg, val))
			return
		proc_start(phy, reg, val)

	elif status == 1:
		proc_low(phy, reg, val)
	elif status == 2:
		proc_high(rw, reg, val)



def main():
	global prev_clk
	prev_clk = 0


	with open('mdio_out.txt', 'rb') as f:

		reader = csv.reader(f, delimiter=' ')
		for row in reader:
			ts,rw,phy,reg, val = row[:5]

#			is_write = rw == 'wr'
			phy = int(phy, 0)
			reg = int(reg, 0)
			val = int(val, 0)

			process_line(ts, rw, phy, reg, val)

if __name__ == '__main__':
    main()
