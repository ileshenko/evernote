#!/usr/bin/env python

# Registers
# 0x10 - [nnnn nnnn 0000 0001] page number
# 0x11 - [rrrr rrrr 0000 00op] Register number and operation
#        op - 00 - none; 01 write; 10 read
# 0x12 - Access Status (nothing to do with it)
# 0x18 - access bits [15..0]
# 0x19 - access bits [31..16]
# 0x20 - access bits [47..32]
# 0x21 - access bits [63..48]

import csv

# State Machine nodes
[
SM_IDLE,
SM_PAGE_SET,
SM_READ_START,
SN_READ_WAIT,
SM_READ_DATA,
SM_SET_DATA,
SM_WRITE_START,
] = range(7)

status = SM_IDLE
arreg = 0
arval = 0
is_read = False

output_file = open('bcm5312x_reg_out.txt', 'w')

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

SM_IDLE,
SM_PAGE_SET,
SM_READ_START,
SN_READ_WAIT,
SM_READ_DATA,
SM_SET_DATA,
SM_WRITE_START,
	raw_write = rw == 'wr'

	
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

	with open('mdio_out.txt', 'rb') as f:

		reader = csv.reader(f, delimiter=' ')
		for row in reader:
			ts,rw,phy,reg, val = row[:5]

			phy = int(phy, 0)
			reg = int(reg, 0)
			val = int(val, 0)

			process_line(ts, rw, phy, reg, val)

if __name__ == '__main__':
    main()
