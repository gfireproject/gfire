#!/usr/bin/env python

import subprocess, time, string, re, itertools

seconds = 5
command = ['sudo', 'tcpdump', '-f']
grep = 'UDP'

regex = re.compile('\d*:\d*:\d*.\d* IP \d*.\d*.\d*.\d*.\d\d\d\d\d > \d*-\d*-\d*-\d*.*, length*')
regex_ip = re.compile('\d*.\d*.\d*.\d*.\d\d\d\d\d')

try:
	output = subprocess.Popen(command, stderr=subprocess.STDOUT, stdout=subprocess.PIPE, shell=False)
	time.sleep(seconds)
	output.kill()

	ips = 0
	for line in itertools.islice(output.stdout, 3, None):
		if grep in line and re.match(regex, line) and ips < 20:
			line_fields = line.strip().split()
			ip_full = line_fields[2]
			ip_fields = ip_full.split('.')
			print "%s.%s.%s.%s:%s" % (ip_fields[0], ip_fields[1], ip_fields[2], ip_fields[3], ip_fields[4])

except OSError:
	print "OSError"
