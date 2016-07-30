autonet
=======

Automatic network configuration for OpenBSD.

Scans the list of network interfaces every few seconds and runs a program when
an interface is attached or detached, e.g. `/etc/autonet/onattach urndis0`.
