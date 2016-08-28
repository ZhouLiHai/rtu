#coding: utf-8

#创建一个TCP服务程序，这个程序会把客户发送过来的字符串加上一个时间戳，然后显示,并返回客户端 
from socket import *

import time
import struct
import hashlib

HOST = '192.168.0.103'
BUFSIZE = 1024    #1KB

#封装数据帧
def mkData(plus, x, fLength):
    head = b"\x68"+ bytearray([len(plus) + 14]) + "\x00\x00\x00\x00\xff" + bytearray([len(plus), 6 , 0, 1, 0])
    head += bytearray([x % 256, x / 256, fLength % 256, fLength / 256])
    head += bytearray(plus)

    return head

#打开文件计算长度
def readyFile(name):
    fObject = open(name)

    fDatas = fObject.read()
    fLength = len(fDatas) / 128

    return fDatas, fLength

def update_add(add) :
    #建立连接
    ADDR = (HOST, 2404)  
    tcpSerSock = socket(AF_INET, SOCK_STREAM)  
    tcpClientSock = socket(AF_INET, SOCK_STREAM)  
    tcpClientSock.connect(ADDR)

    #请求连接报文
    head = b"\x68\x04\x07" + bytearray([0, 0, 0])
    tcpClientSock.send(head)

    #忽略应答报文
    res = tcpClientSock.recv(128)

    #准备数据
    fDatas, fLength = readyFile('Dcore')

    curIdx = 0

    #循环接收终端请求的帧序号,并根据帧序号传输相应的数据块
    #开始curIdx为0表示启动传输
    while curIdx != fLength:
        plus = []
        plus += fDatas[curIdx*128: (curIdx+1)*128]

        tcpClientSock.send(mkData(plus, curIdx, fLength))

        # time.sleep(1)
        res = tcpClientSock.recv(128)

        curIdxL, curIdxH, tolNumL, tolNumH, state = struct.unpack("BBBBB", res[12:17])
        curIdx = curIdxH*256+curIdxL

        if state != 0:
            print "Error: " + str(state)
            break
        # print [curIdx]

    #发送最后一帧
    plus = []
    plus += fDatas[fLength*128:]

    tcpClientSock.send(mkData(plus, fLength, fLength))
    print ">>>>>>>>>", [len(plus)]

    #接收最后一帧的当前序号和总序号
    res = tcpClientSock.recv(128)

    curIdxL, curIdxH, tolNumL, tolNumH, state = struct.unpack("BBBBB", res[12:17])
    curIdx = curIdxH*256+curIdxL;
    tolNum = tolNumH*256+tolNumL;

    print len(res), curIdx, tolNum, state

    #计算文件md5
    m2 = hashlib.md5()   
    m2.update(fDatas)   
    print m2.hexdigest()

    #发送升级启动帧
    tcpClientSock.send(mkData(m2.hexdigest(), 0xffff, 0xffff))

    time.sleep(1)
    tcpClientSock.close()  

if __name__ == '__main__':
    update_add(HOST)