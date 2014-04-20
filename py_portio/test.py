import sys,portio

if portio.enable_io():
	print "Access to I/O ports successfully enabled"
else:
	print "Failed to enable access to I/O ports (try running as root)"
	sys.exit(1)

port = 0x378
print "Reading port 0x%x: 0x%x"%(port, portio.inb(port))
