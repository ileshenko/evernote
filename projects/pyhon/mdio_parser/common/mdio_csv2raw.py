#!/usr/bin/env python

# Convert snifed MDIO bus to readable form.
# Input - CSV data from Saleae Logic Analyzer on form "timestamp, mdc, mdio"
# Output - formatted test in form "timestamp op phyad regad data"

# MDIO instructin built from:
# PRE - Preamble (32 empty clocks)
# ST - Start of Frame (01)
# OP - Operation Code (01 for read, 10 for write)
# PHYAD - PHY Address (5 bits, msb first)
# REGAD - Register Address (5 bits)
# TA - Turnaround (2 bits)
# DATA (16 bits)

import csv

# State Machine nodes
[
SM_IDLE,
SM_PRE,
SM_ST,
SM_OP,
SM_PHYAD,
SM_REGAD,
SM_TA,
SM_DATA,
] = range(8)

# previous frame data
prev_mdc = 0
#prev_mdio = 0

# State Machine data
status = SM_IDLE
val = 0
buf_len = 0
is_reading = False

output_file = open('mdio_out.txt', 'w')

def bit_push(mdio):
	global buf_len
	global val

	val <<= 1;
	val |= mdio
	buf_len += 1
	return buf_len

def proc_idle(ts, mdio):
	global status

	if mdio == 0:
#		output_file.write('%-.7f: Error ID\n' % ts)
		return

	status = SM_PRE

def proc_preamble(ts, mdio):
	global buf_len
	global status

	if mdio == 0:
		output_file.write('%-.7f: Error PREAMBLE\n' % ts)
		status = SM_IDLE
		return

	buf_len += 1

	if int(buf_len) >= 28: #sometimes we lost first sync data
		status = SM_ST

def proc_start(ts, mdio):
	global buf_len
	global status

	if buf_len == 0:
		if mdio == 0:
			output_file.write("%-.7f " % ts)
			buf_len += 1
		else:
			return
	else:
		if mdio == 1:
			status = SM_OP
		else:
			output_file.write('%-.7f: Error START\n' % ts)
			status = SM_IDLE

def proc_cmd(ts, mdio):
	global status
	global is_reading

	if bit_push(mdio) < 2:
		return

	if val not in (0x1, 0x2):
		output_file.write('%-.7f: Error OPERATION\n' % ts)
		status = SM_IDLE
		return

	is_reading = val == 0x2
	output_file.write("rd " if is_reading else "wr ")

	status = SM_PHYAD

def proc_phy(ts, mdio):
	global status

	if bit_push(mdio) < 5:
		return
	output_file.write('%#04x' % val)
	status = SM_REGAD

def proc_reg(ts, mdio):
	global status

	if bit_push(mdio) == 5:
		output_file.write(' %#04x' % val)
		status = SM_TA

def proc_turnaround(ts, mdio):
	global status

	if bit_push(mdio) == 2:
		status = SM_DATA

def proc_val(ts, mdio):
	global status

	if bit_push(mdio) < 16:
		return
	output_file.write(' %#06x\n' % val)
	status = SM_IDLE

# 1 - rising edge,
# 0 - falling edge
def get_work_edge():
	global status

#For BCM
	return 0

	if status != SM_DATA:
		return 1
	return 0 if is_reading else 1

def process_line(ts, mdc, mdio):
	global prev_mdc
	global buf_len;
	global val
	saved_status = status

	if prev_mdc == mdc:
		return

	prev_mdc = mdc;

#	print(" %-.7f %d %s" %(ts, mdio, is_reading))
	if mdc != get_work_edge():
		return

	if status == SM_IDLE:
		proc_idle(ts, mdio)
	elif status == SM_PRE:
		proc_preamble(ts, mdio)
	elif status == SM_ST:
		proc_start(ts, mdio)
	elif status == SM_OP:
		proc_cmd(ts, mdio)
	elif status == SM_PHYAD:
		proc_phy(ts, mdio)
	elif status == SM_REGAD:
		proc_reg(ts, mdio)
	elif status == SM_TA:
		proc_turnaround(ts, mdio)
	elif status == SM_DATA:
		proc_val(ts, mdio)

	if saved_status != status:
		buf_len = 0
		val = 0

def main():

	with open('u.csv', 'rb') as f:

		reader = csv.reader(f)

		for row in reader:
			ts,mdc,mdio = row[:3]
			ts = float(ts)
			mdc = int(mdc)
			mdio = int(mdio)
			process_line(ts, mdc, mdio)

if __name__ == '__main__':
    main()
