#!/usr/bin/env python

import csv

# status
# 0 - idle
# 1 - preamble
# 2 - start
# 3 - cmd
# 4 - phy
# 5 - reg
# 6 - pause
# 7 - pause

#[IDLE, PREAMBLE] = range(8)

prev_ts = 0.0
prev_clk = 0
prev_io = 0
status = 0
val = 0
is_reading = 0

output_file = open('mdio_out.txt', 'w')

buf_len = 0

def proc_idle(ts, mdio):
	global status
	global buf_len

	if int(mdio) == 0 :
#		output_file.write('%-8s: Error ID\n' % ts)
		return

	buf_len = 0
	status = 1 # preamble

def proc_preamble(ts, mdio):
	global buf_len
	global status

	if int(buf_len) == 0 :
		output_file.write("%-.7f " % float(ts))

	if int(mdio) == 0 :
		output_file.write('%-8s: Error PR\n' % ts)
		status = 0 #idle

	buf_len += 1

	if int(buf_len) >= 28: #sometimes we lost first sync data
		buf_len = 0
		status = 2

def proc_start(ts, mdio):
	global buf_len
	global status

#	output_file.write("$$ %d %d $$$" % (int(buf_len), int(mdio)))

	if int(buf_len) == 0 :
		if int(mdio) == 0 :
			buf_len += 1
		else :
			return
	else :
		if int(mdio) == 1 :
			status = 3
			buf_len = 0
		else :
			output_file.write('%-8s: Error ST\n' % ts)
			status = 0


def proc_cmd(ts, mdio):
	global buf_len
	global status
	global prev_io
	global is_reading

	if int(buf_len) == 0 :
		prev_io = mdio
		buf_len += 1
		return
	
	if int(prev_io) == int(mdio):
		output_file.write('%-8s: Error CM\n' % ts)
		status = 0
		return

	if int(mdio) == 0:
		is_reading = 1
		output_file.write("rd ")
	else :
		output_file.write("wr ")

	status = 4
	buf_len = 0

def proc_phy(ts, mdio):
	global buf_len
	global status
	global val

	if int(buf_len) == 0 :
		val = 0;

	val *= 2;
	if int(mdio) == 1 :
		val += 1

	buf_len += 1

	if int(buf_len) == 5 :
		output_file.write('%#04x' % int(val))
		status = 5
		buf_len = 0

def proc_reg(ts, mdio):
	global buf_len
	global status
	global val

	if int(buf_len) == 0 :
		val = 0;

	val *= 2;
	if int(mdio) == 1 :
		val += 1

	buf_len += 1

	if int(buf_len) == 5 :
		output_file.write(' %#04x' % int(val))
		status = 6
		buf_len = 0

def proc_pause(ts, mdio):
	global buf_len
	global status
	global prev_io

	buf_len += 1

	if int(buf_len) == 1 :
		return

	status = 7
	buf_len = 0

def proc_val(ts, mdio):
	global buf_len
	global status
	global val
	global is_reading

#	print(" %s %s %d" %(ts, mdio, is_reading))
	if int(buf_len) == 0 :
		val = 0;

	val *= 2;
	if int(mdio) == 1 :
		val += 1

	buf_len += 1

	if int(buf_len) == 16 :
		output_file.write(' %#06x\n' % int(val))
		status = 0
		buf_len = 0
		is_reading = 0

def process_line(ts, mdc, mdio):
	global prev_clk
	global status
	global is_reading

	if int(prev_clk) == int(mdc):
		return

	prev_clk = mdc;

	if int(status) == 7:
		if int(is_reading) == int(mdc):
			return
	else :
		if int(mdc) == 0 :
		
			return

			
	if int(status) == 0 :
		proc_idle(ts, mdio)
	elif int(status) == 1:
		proc_preamble(ts, mdio)
	elif int(status) == 2 :
		proc_start(ts, mdio)
	elif int(status) == 3 :
		proc_cmd(ts, mdio)
	elif int(status) == 4 :
		proc_phy(ts, mdio)
	elif int(status) == 5 :
		proc_reg(ts, mdio)
	elif int(status) == 6 :
		proc_pause(ts, mdio)
	elif int(status) == 7 :
		proc_val(ts, mdio)

def main():
	prev_clk = 0;


	with open('u.csv', 'rb') as f:

		reader = csv.reader(f)

		for row in reader:
			ts,mdc,mdio = row[:3]
			process_line(ts, mdc, mdio)

if __name__ == '__main__':
    main()
