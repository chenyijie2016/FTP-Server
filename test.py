#! /usr/bin/python3
import subprocess
import random
import time
import filecmp
import struct
import os
import shutil
import string
from ftplib import FTP

log = open('log.txt','w')
def test(port=1100, directory='/tmp'):
    try:
        server = subprocess.Popen(
            ['./server', '-port', '%d' % port, '-root', directory], stdout=log)
        ftp = FTP()
        print({'str':ftp.connect('127.0.0.1', port)})
        res =ftp.login()
        print(res)
        res = ftp.sendcmd('SYST')
        print(res)
        res = ftp.set_pasv(False)
        print(res)

        print('Test Finished! Press ^C to exit')
        server.wait()
    except KeyboardInterrupt:
        log.close()
        exit(0)
test()
log.close()