#!/usr/bin/env python

import liblo
import sys
import random

def sendBlob(libloTarget):
    r = random.randint(0,255)
    g = random.randint(0,255)
    b = random.randint(0,255)
    payload = [0, 0, 0] * 56;
    liblo.send(libloTarget, "/foo/blob", payload);

def main(argv):
    ip = "127.0.0.1"
    port = "12000"
    if len(argv) > 1:
        ip = argv[1]
    if len(argv) > 2:
        port = argv[2]
    try:
        target = liblo.Address(ip, port)
        #target = liblo.Address("192.168.1.148", "12000")
        #target = liblo.Address("192.168.20.106", "12000")
    except liblo.AddressError, err:
        print str(err)
        sys.exit()
    sendBlob(target)
    #sendString(target)

main(sys.argv)

