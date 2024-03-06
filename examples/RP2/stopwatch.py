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
tft = gc9a01.GC9A01(
        SPI(1, baudrate=60000000, sck=Pin(SCK), mosi=Pin(MOSI)),
        240,
        240,
        reset=Pin(RST, Pin.OUT),
        cs=Pin(CS, Pin.OUT),
        dc=Pin(DC, Pin.OUT),
        backlight=Pin(BL, Pin.OUT),
        rotation=0)
start = time.ticks_ms()
ahmsu = [0,0,0,0]
nowT = [0,0,0,0,0,0,0,0]
#(2022, 12, 27, 23, 19, 31, 1, 361)

gc.enable()
gc.collect()

def plotxy(angle,length):
    return [120+length*math.sin(angle),120-length*math.cos(angle)]
def plotxyTxt(angle,length):#adjust the position for text output
    return [113+length*math.sin(angle),116-length*math.cos(angle)]
def plotxyOffset(angle,length,offsetX,offsetY):
    return [offsetX+120+length*math.sin(angle),offsetY+120-length*math.cos(angle)]
def plotxyTxtOffset(angle,length,offsetX,offsetY):#adjust the position for text output
    return [offsetX+113+length*math.sin(angle),offsetY+116-length*math.cos(angle)]

def update():
    global nowT,ahmsu,start
    dfft=time.ticks_diff(time.ticks_ms(), start)
    #nowT=list(time.ticks_diff(time.ticks_ms, start))
    hour= 0#int(dfft/3600000)
    minute = int((dfft-hour*3600000)/60000)%100
    second = int((dfft-hour*3600000-minute*60000)/1000)
    msec = dfft-hour*3600000-minute*60000-second*1000
    nowT[3]= hour
    nowT[4]= minute
    nowT[5]= second*1000+msec
    ahmsu[0]=2*pi*hour/12+2*pi*minute/(12*60)+2*second/(12*60*60)#-pi/6.0
    ahmsu[1]=2*pi*minute/60+2*pi*second/(60*60)#-pi/6.0
    ahmsu[2]=2*pi*(second+msec/1000)/60#-pi/6.0
    ahmsu[3]=second
def strD(n):#add 0 to the front of the single digit output
    return str(n) if n>9 else "0"+str(n)
def strDS(n):#add 0 to the front of the single digit output
    a=int(n/1000)
    b=int(n/10)-int(int(n/10)/100)*100
    return strD(a)+"."+strD(b)
def timeText():#correct the time delay in digital clock
    return strD(nowT[4]+1)+":00.00" if nowT[5]==59999 else strD(nowT[4])+":"+strDS(nowT[5]+1) 
def drawDigitalClock():
    tft.text(
            font1,
            timeText(),
            54,#col
            96,#row
            gc9a01.WHITE,gc9a01.BLACK
                )
def atd(a):
    return a/180*pi
def minuteFace(x):
    th=x*pi/30
    pf=plotxyOffset(th,20,0,40)
    pe=plotxyOffset(th,22,0,40)
    tft.line(int(pf[0]),int(pf[1]), int(pe[0]), int(pe[1]), gc9a01.WHITE)
    if x%15 == 0:
             pg=plotxyTxtOffset(th,30,0,40)
             tft.text(font0,
                 strD(x),
                 int(pg[0]),int(pg[1]),gc9a01.GREEN,gc9a01.BLACK)
def drawAnalogClockBG():#raw the surface for the clock
    for x in range(60):
        th=x*pi/30
        pa=plotxy(th,92)
        pb=plotxy(th,90)
        tft.line(int(pa[0]),int(pa[1]), int(pb[0]), int(pb[1]), gc9a01.WHITE)
        if x%5 == 0:
            minuteFace(x)
            pc=plotxy(th,96)
            tft.line(int(pc[0]),int(pc[1]), int(pb[0]), int(pb[1]), gc9a01.WHITE)
            pd=plotxyTxt(th,110)
            tft.text(font0,
                     strD(x),
                     int(pd[0]),int(pd[1]),gc9a01.GREEN,gc9a01.BLACK)
def drawAnalogClock():  
    pxy=plotxy(ahmsu[2],89)#cover old second arm
    tft.line(120, 120, int(pxy[0]), int(pxy[1]), gc9a01.BLACK)
    pxy=plotxyOffset(ahmsu[1],19,0,40)#cover old minute arm
    tft.line(120, 160, int(pxy[0]), int(pxy[1]),  gc9a01.BLACK)
    update()
    if nowT[5] >22000 and nowT[5] <38000:
        for x in range(12):
            minuteFace(x*5)
    pxy=plotxy(ahmsu[2],89)#draw new second arm
    tft.line(120, 120, int(pxy[0]), int(pxy[1]), gc9a01.RED)
    pxy=plotxyOffset(ahmsu[1],19,0,40)#draw new minute arm
    tft.line(120, 160, int(pxy[0]), int(pxy[1]),  gc9a01.BLUE)
def main():
    tft.init() 
    Vbat= ADC(Pin(Vbat_Pin))
    tft.fill(gc9a01.BLACK)
    drawAnalogClockBG()
    start = time.ticks_ms()
    while True:
        
        drawDigitalClock()
        drawAnalogClock()
        #print(time.ticks_diff(time.ticks_ms(), start))
        time.sleep(0.08)

main()

