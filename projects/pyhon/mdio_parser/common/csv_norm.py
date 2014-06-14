#!/usr/bin/env python

import csv

def main():

	output_file = open('csv_norm.csv', 'w')
	with open('u.csv', 'r') as f:

		reader = csv.reader(f)

		for row in reader:
			ts,mdc,mdio = row[:3]
			ts = float(ts)
			output_file.write('%-.7f,%s,%s\n' % (ts, mdc, mdio))

if __name__ == '__main__':
    main()
