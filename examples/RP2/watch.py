'''
author: leoncoolmoon
version: 1.0
'''
import gc
import time
from machine import Pin,I2C,SPI,PWM,ADC
import gc9a01
import  vga1_8x8 as font0
import  vga1_16x16 as font1
import random
import math

DC = 8
CS = 9
SCK = 10
MOSI = 11
RST = 12
BL = 25
I2C_SDA = 6
I2C_SDL = 7
Vbat_Pin = 29
pi = math.pi
wkd=["Mon","Tue","Wed","Thu","Fri","Sat","Sun"]
tft = gc9a01.GC9A01(
        SPI(1, baudrate=60000000, sck=Pin(SCK), mosi=Pin(MOSI)),
        240,
        240,
        reset=Pin(RST, Pin.OUT),
        cs=Pin(CS, Pin.OUT),
        dc=Pin(DC, Pin.OUT),
        backlight=Pin(BL, Pin.OUT),
        rotation=0)

ahmsu = [0,0,0,0]
nowT = [0,0,0,0,0,0,0,0]
#(2022, 12, 27, 23, 19, 31, 1, 361)

gc.enable()
gc.collect()

def plotxy(angle,length):
    return [120+length*math.sin(angle),120-length*math.cos(angle)]
def plotxyTxt(angle,length):#adjust the position for text output
    return [115+length*math.sin(angle),116-length*math.cos(angle)]
def update():
    global nowT,ahmsu
    nowT=list(time.localtime(time.time()))
    hour= nowT[3]
    minute = nowT[4]
    second = nowT[5]
    #tft.text(font0,str(id(nowT)),120,120,gc9a01.WHITE,gc9a01.BLACK)
    #angles_h =
    ahmsu[0]=2*pi*hour/12+2*pi*minute/(12*60)+2*second/(12*60*60)#-pi/6.0
    #angles_m=
    ahmsu[1]=2*pi*minute/60+2*pi*second/(60*60)#-pi/6.0
    #angles_s =
    ahmsu[2]=2*pi*second/60#-pi/6.0
    ahmsu[3]=second
def strD(n):#add 0 to the front of the single digit output
    return str(n) if n>9 else "0"+str(n)
def timeText():#correct the time delay in digital clock
    return strD(nowT[3])+":"+strD(nowT[4]+1)+":00" if nowT[5]==59 else strD(nowT[3])+":"+strD(nowT[4])+":"+strD(nowT[5]+1) 
def drawDigitalClock():
    tft.text(font0,str(nowT[0])+"/"+strD(nowT[1])+"/"+strD(nowT[2]),80,150,gc9a01.RED,gc9a01.BLACK)
    tft.text(font1,"AM" if nowT[3]<12 else "PM",104,72,gc9a01.RED,gc9a01.BLACK)
    tft.text(font0,wkd[nowT[6]],112,180,gc9a01.GREEN,gc9a01.BLACK)
    tft.text(
            font1,
            timeText(),
            52,#col
            96,#row
            gc9a01.WHITE,gc9a01.BLACK
                )
def atd(a):
    return a/180*pi
def drawAnalogClockBG():#raw the surface for the clock
    for x in range(60):
        th=x*pi/30
        pa=plotxy(th,116)
        pb=plotxy(th,118)
        tft.line(int(pa[0]),int(pa[1]), int(pb[0]), int(pb[1]), gc9a01.WHITE)
        if x%5 == 0:
            pc=plotxy(th,110)
            tft.line(int(pc[0]),int(pc[1]), int(pb[0]), int(pb[1]), gc9a01.WHITE)
            pd=plotxyTxt(th,96)
            tft.text(font0,
                     str(int(12 if x==0 else x/5)),
                     int(pd[0]),int(pd[1]),gc9a01.GREEN,gc9a01.BLACK)
def drawAnalogClock():  
    pxy=plotxy(ahmsu[2],96)#cover old second arm
    tft.line(120, 120, int(pxy[0]), int(pxy[1]), gc9a01.BLACK)
    pxy=plotxy(ahmsu[1],84)#cover old minute arm
    tft.line(120, 120, int(pxy[0]), int(pxy[1]),  gc9a01.BLACK)
    pxy=plotxy(ahmsu[0],60)#cover old hour arm
    tft.line(120, 120, int(pxy[0]), int(pxy[1]),  gc9a01.BLACK)
    update()
    pxy=plotxy(ahmsu[2],96)#draw new second arm
    tft.line(120, 120, int(pxy[0]), int(pxy[1]), gc9a01.RED)
    pxy=plotxy(ahmsu[1],84)#draw new minute arm
    tft.line(120, 120, int(pxy[0]), int(pxy[1]),  gc9a01.GREEN)
    pxy=plotxy(ahmsu[0],60)#draw new hour arm
    tft.line(120, 120, int(pxy[0]), int(pxy[1]),  gc9a01.BLUE)  
    if (ahmsu[3]%5) == 1: #fix the analog clock number-texts
        x=ahmsu[3]-1
        th=x*pi/30
        pd=plotxyTxt(th,96)
        tft.text(font0,
                 str(int(12 if x==0 else 11 if x==-5 else x/5)),
                 int(pd[0]),int(pd[1]),gc9a01.GREEN,gc9a01.BLACK)
def main():
    tft.init() 
    col_max = tft.width() - font0.WIDTH*6
    row_max = tft.height() - font0.HEIGHT
    Vbat= ADC(Pin(Vbat_Pin))
    tft.fill(gc9a01.BLACK)
    drawAnalogClockBG()
    while True:   
        drawDigitalClock()
        drawAnalogClock()
        time.sleep(1)

main()

