#! /usr/bin/python3

import socket


def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect(('127.0.0.1', 1234))
    s.send(b'USER anonymous\r\n')

if __name__ == '__main__':
    main()
