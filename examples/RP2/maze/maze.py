# Maze generator -- Randomized Prim Algorithm

## Imports
import random
import time

import  vga1_8x8 as font0
import  scriptc as font1
import gc
from machine import Pin,I2C,SPI,PWM,ADC
import gc9a01
DC = 8
CS = 9
SCK = 10
MOSI = 11
RST = 12
BL = 25
I2C_SDA = 6
I2C_SDL = 7
Vbat_Pin = 29
tft = gc9a01.GC9A01(
        SPI(1, baudrate=60000000, sck=Pin(SCK), mosi=Pin(MOSI)),
        240,
        240,
        reset=Pin(RST, Pin.OUT),
        cs=Pin(CS, Pin.OUT),
        dc=Pin(DC, Pin.OUT),
        backlight=Pin(BL, Pin.OUT),
        rotation=0)
gc.enable()
gc.collect()


## Main code
# Init variables
height = 20
width = 20
mazeSize=[width,height]
xOfset = 40
yOfset = 40
bC = [gc9a01.RED,gc9a01.GREEN,gc9a01.WHITE,gc9a01.BLUE,gc9a01.BLUE,gc9a01.BLUE]
wallC = 'H'
pathC = ' '
unvisitedC = 'u'
startC = 'S'
endC = 'E'
meC = '@'
fC= [wallC,pathC,unvisitedC,startC,endC,meC]
pC=["WALL.jpg","PATH.jpg","UNKNOW.jpg","START.jpg","END.jpg","ME.jpg","WIN.jpg"]
startL = 0
endL = 0
my_path = "/maze"
maze = []
th=0.2
record = 9999999999999999999
#input array output pixel
def pos(i,j):
    return [int(i*8+xOfset),int(j*8+yOfset)]

## Functions
def printMaze(maze):
    for i in range(0, height):
        for j in range(0, width):
            l=pos(i,j)
            #print(fC[maze[i][j]], end=" ")
            #tft.text(font0,fC[maze[i][j]],l[0],l[1],bC[maze[i][j]],gc9a01.BLACK)
            tft.jpg(my_path+"/pic/"+pC[maze[i][j]],l[0],l[1],gc9a01.FAST)
        #print('\n')

# Find number of surrounding cells
def surroundingCells(rand_wall):
    s_cells = 0
    if (maze[rand_wall[0]-1][rand_wall[1]] == 1):
        s_cells += 1
    if (maze[rand_wall[0]+1][rand_wall[1]] == 1):
        s_cells += 1
    if (maze[rand_wall[0]][rand_wall[1]-1] == 1):
        s_cells +=1
    if (maze[rand_wall[0]][rand_wall[1]+1] == 1):
        s_cells += 1

    return s_cells



def generateMaze(x,y):
    global mazeSize,xOfset,yOfset,height,width,startL,endL
    mazeSize=[x,y]
    xOfset=(30-mazeSize[0])*4
    yOfset=(30-mazeSize[1])*4
    height = mazeSize[0]
    width = mazeSize[1]
    #1. set all cells as unvisited
    for i in range(0, height):
        line = []
        for j in range(0, width):
            line.append(2)
        maze.append(line)

    #2. Randomize starting point and set it a cell
    starting_height = int(random.random()*height)
    starting_width = int(random.random()*width)
    if (starting_height == 0):
        starting_height += 1
    if (starting_height == height-1):
        starting_height -= 1
    if (starting_width == 0):
        starting_width += 1
    if (starting_width == width-1):
        starting_width -= 1

    #3. Mark it as path and add surrounding walls to the list
    maze[starting_height][starting_width] = 1
    walls = []
    walls.append([starting_height - 1, starting_width])
    walls.append([starting_height, starting_width - 1])
    walls.append([starting_height, starting_width + 1])
    walls.append([starting_height + 1, starting_width])

    # Denote walls in maze
    maze[starting_height-1][starting_width] = 0
    maze[starting_height][starting_width - 1] = 0
    maze[starting_height][starting_width + 1] = 0
    maze[starting_height + 1][starting_width] = 0

    while (walls):
        # Pick a random wall
        rand_wall = walls[int(random.random()*len(walls))-1]

        # Check if it is a left wall
        if (rand_wall[1] != 0):
            if (maze[rand_wall[0]][rand_wall[1]-1] == 2 and maze[rand_wall[0]][rand_wall[1]+1] == 1):
                # Find the number of surrounding cells
                s_cells = surroundingCells(rand_wall)

                if (s_cells < 2):
                    # Denote the new path
                    maze[rand_wall[0]][rand_wall[1]] = 1

                    # Mark the new walls
                    # Upper cell
                    if (rand_wall[0] != 0):
                        if (maze[rand_wall[0]-1][rand_wall[1]] != 1):
                            maze[rand_wall[0]-1][rand_wall[1]] = 0
                        if ([rand_wall[0]-1, rand_wall[1]] not in walls):
                            walls.append([rand_wall[0]-1, rand_wall[1]])


                    # Bottom cell
                    if (rand_wall[0] != height-1):
                        if (maze[rand_wall[0]+1][rand_wall[1]] != 1):
                            maze[rand_wall[0]+1][rand_wall[1]] = 0
                        if ([rand_wall[0]+1, rand_wall[1]] not in walls):
                            walls.append([rand_wall[0]+1, rand_wall[1]])

                    # Leftmost cell
                    if (rand_wall[1] != 0):    
                        if (maze[rand_wall[0]][rand_wall[1]-1] != 1):
                            maze[rand_wall[0]][rand_wall[1]-1] = 0
                        if ([rand_wall[0], rand_wall[1]-1] not in walls):
                            walls.append([rand_wall[0], rand_wall[1]-1])


                # Delete wall
                for wall in walls:
                    if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                        walls.remove(wall)

                continue

        # Check if it is an upper wall
        if (rand_wall[0] != 0):
            if (maze[rand_wall[0]-1][rand_wall[1]] == 2 and maze[rand_wall[0]+1][rand_wall[1]] == 1):

                s_cells = surroundingCells(rand_wall)
                if (s_cells < 2):
                    # Denote the new path
                    maze[rand_wall[0]][rand_wall[1]] = 1

                    # Mark the new walls
                    # Upper cell
                    if (rand_wall[0] != 0):
                        if (maze[rand_wall[0]-1][rand_wall[1]] != 1):
                            maze[rand_wall[0]-1][rand_wall[1]] = 0
                        if ([rand_wall[0]-1, rand_wall[1]] not in walls):
                            walls.append([rand_wall[0]-1, rand_wall[1]])

                    # Leftmost cell
                    if (rand_wall[1] != 0):
                        if (maze[rand_wall[0]][rand_wall[1]-1] != 1):
                            maze[rand_wall[0]][rand_wall[1]-1] = 0
                        if ([rand_wall[0], rand_wall[1]-1] not in walls):
                            walls.append([rand_wall[0], rand_wall[1]-1])

                    # Rightmost cell
                    if (rand_wall[1] != width-1):
                        if (maze[rand_wall[0]][rand_wall[1]+1] != 1):
                            maze[rand_wall[0]][rand_wall[1]+1] = 0
                        if ([rand_wall[0], rand_wall[1]+1] not in walls):
                            walls.append([rand_wall[0], rand_wall[1]+1])

                # Delete wall
                for wall in walls:
                    if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                        walls.remove(wall)

                continue

        # Check the bottom wall
        if (rand_wall[0] != height-1):
            if (maze[rand_wall[0]+1][rand_wall[1]] == 2 and maze[rand_wall[0]-1][rand_wall[1]] == 1):

                s_cells = surroundingCells(rand_wall)
                if (s_cells < 2):
                    # Denote the new path
                    maze[rand_wall[0]][rand_wall[1]] = 1

                    # Mark the new walls
                    if (rand_wall[0] != height-1):
                        if (maze[rand_wall[0]+1][rand_wall[1]] != 1):
                            maze[rand_wall[0]+1][rand_wall[1]] = 0
                        if ([rand_wall[0]+1, rand_wall[1]] not in walls):
                            walls.append([rand_wall[0]+1, rand_wall[1]])
                    if (rand_wall[1] != 0):
                        if (maze[rand_wall[0]][rand_wall[1]-1] != 1):
                            maze[rand_wall[0]][rand_wall[1]-1] = 0
                        if ([rand_wall[0], rand_wall[1]-1] not in walls):
                            walls.append([rand_wall[0], rand_wall[1]-1])
                    if (rand_wall[1] != width-1):
                        if (maze[rand_wall[0]][rand_wall[1]+1] != 1):
                            maze[rand_wall[0]][rand_wall[1]+1] = 0
                        if ([rand_wall[0], rand_wall[1]+1] not in walls):
                            walls.append([rand_wall[0], rand_wall[1]+1])

                # Delete wall
                for wall in walls:
                    if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                        walls.remove(wall)


                continue

        # Check the right wall
        if (rand_wall[1] != width-1):
            if (maze[rand_wall[0]][rand_wall[1]+1] == 2 and maze[rand_wall[0]][rand_wall[1]-1] == 1):

                s_cells = surroundingCells(rand_wall)
                if (s_cells < 2):
                    # Denote the new path
                    maze[rand_wall[0]][rand_wall[1]] = 1

                    # Mark the new walls
                    if (rand_wall[1] != width-1):
                        if (maze[rand_wall[0]][rand_wall[1]+1] != 1):
                            maze[rand_wall[0]][rand_wall[1]+1] = 0
                        if ([rand_wall[0], rand_wall[1]+1] not in walls):
                            walls.append([rand_wall[0], rand_wall[1]+1])
                    if (rand_wall[0] != height-1):
                        if (maze[rand_wall[0]+1][rand_wall[1]] != 1):
                            maze[rand_wall[0]+1][rand_wall[1]] = 0
                        if ([rand_wall[0]+1, rand_wall[1]] not in walls):
                            walls.append([rand_wall[0]+1, rand_wall[1]])
                    if (rand_wall[0] != 0):    
                        if (maze[rand_wall[0]-1][rand_wall[1]] != 1):
                            maze[rand_wall[0]-1][rand_wall[1]] = 0
                        if ([rand_wall[0]-1, rand_wall[1]] not in walls):
                            walls.append([rand_wall[0]-1, rand_wall[1]])

                # Delete wall
                for wall in walls:
                    if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                        walls.remove(wall)

                continue

        # Delete the wall from the list anyway
        for wall in walls:
            if (wall[0] == rand_wall[0] and wall[1] == rand_wall[1]):
                walls.remove(wall)
        


    # Mark the remaining unvisited cells as walls
    for i in range(0, height):
        for j in range(0, width):
            if (maze[i][j] == 2):
                maze[i][j] = 0

    # Set entrance and exit
    for i in range(0, width):
        if (maze[1][i] == 1):
            maze[0][i] = 3
            startL = i
            break

    for i in range(width-1, 0, -1):
        if (maze[height-2][i] == 1):
            maze[height-1][i] = 4
            endL = i
            break


class QMI8658(object):
    def __init__(self,address=0X6B):
        self._address = address
        self._bus = I2C(id=1,scl=Pin(I2C_SDL),sda=Pin(I2C_SDA),freq=100_000)
        bRet=self.WhoAmI()
        if bRet :
            self.Read_Revision()
        else    :
            return NULL
        self.Config_apply()

    def _read_byte(self,cmd):
        rec=self._bus.readfrom_mem(int(self._address),int(cmd),1)
        return rec[0]
    def _read_block(self, reg, length=1):
        rec=self._bus.readfrom_mem(int(self._address),int(reg),length)
        return rec
    def _read_u16(self,cmd):
        LSB = self._bus.readfrom_mem(int(self._address),int(cmd),1)
        MSB = self._bus.readfrom_mem(int(self._address),int(cmd)+1,1)
        return (MSB[0] << 8) + LSB[0]
    def _write_byte(self,cmd,val):
        self._bus.writeto_mem(int(self._address),int(cmd),bytes([int(val)]))
        
    def WhoAmI(self):
        bRet=False
        if (0x05) == self._read_byte(0x00):
            bRet = True
        return bRet
    def Read_Revision(self):
        return self._read_byte(0x01)
    def Config_apply(self):
        # REG CTRL1
        self._write_byte(0x02,0x60)
        # REG CTRL2 : QMI8658AccRange_8g  and QMI8658AccOdr_1000Hz
        self._write_byte(0x03,0x23)
        # REG CTRL3 : QMI8658GyrRange_512dps and QMI8658GyrOdr_1000Hz
        self._write_byte(0x04,0x53)
        # REG CTRL4 : No
        self._write_byte(0x05,0x00)
        # REG CTRL5 : Enable Gyroscope And Accelerometer Low-Pass Filter 
        self._write_byte(0x06,0x11)
        # REG CTRL6 : Disables Motion on Demand.
        self._write_byte(0x07,0x00)
        # REG CTRL7 : Enable Gyroscope And Accelerometer
        self._write_byte(0x08,0x03)

    def Read_Raw_XYZ(self):
        xyz=[0,0,0,0,0,0]
        raw_timestamp = self._read_block(0x30,3)
        raw_acc_xyz=self._read_block(0x35,6)
        raw_gyro_xyz=self._read_block(0x3b,6)
        raw_xyz=self._read_block(0x35,12)
        timestamp = (raw_timestamp[2]<<16)|(raw_timestamp[1]<<8)|(raw_timestamp[0])
        for i in range(6):
            # xyz[i]=(raw_acc_xyz[(i*2)+1]<<8)|(raw_acc_xyz[i*2])
            # xyz[i+3]=(raw_gyro_xyz[((i+3)*2)+1]<<8)|(raw_gyro_xyz[(i+3)*2])
            xyz[i] = (raw_xyz[(i*2)+1]<<8)|(raw_xyz[i*2])
            if xyz[i] >= 32767:
                xyz[i] = xyz[i]-65535
        return xyz
    def Read_XYZ(self):
        xyz=[0,0,0,0,0,0]
        raw_xyz=self.Read_Raw_XYZ()  
        #QMI8658AccRange_8g
        acc_lsb_div=(1<<12)
        #QMI8658GyrRange_512dps
        gyro_lsb_div = 64
        for i in range(3):
            xyz[i]=raw_xyz[i]/acc_lsb_div#(acc_lsb_div/1000.0)
            xyz[i+3]=raw_xyz[i+3]*1.0/gyro_lsb_div
        return xyz

def main():
    global startL,endL,maze,pC,th,record
    tft.init() 
    qmi8658 = QMI8658()
    while True:
        tft.fill(gc9a01.BLACK)
        generateMaze(width,height)
        printMaze(maze)
        play = True
        #print(str(startL)+", "+str(endL))
        currentL = [0,startL] # 0,startL -> height-1,endL#array
        ctrl = [0,0]#array
        l=pos(currentL[0],currentL[1])
        tft.jpg(my_path+"/pic/"+pC[5],l[0],l[1],gc9a01.FAST)
        #tft.text(font0,fC[5],l[0],l[1],bC[5],gc9a01.BLACK)
        start = time.ticks_ms() 
        while play:
            
            tft.jpg(my_path+"/pic/"+pC[maze[currentL[0]][currentL[1]]],l[0],l[1],gc9a01.FAST)
            #tft.text(font0,fC[maze[currentL[0]][currentL[1]]],l[0],l[1],bC[maze[currentL[0]][currentL[1]]],gc9a01.BLACK)
            xyz=qmi8658.Read_XYZ()
            ctrl = [int(abs(xyz[1])/xyz[1]) if abs(xyz[1]) > th else 0,int(abs(xyz[0])/xyz[0]) if abs(xyz[0]) > th else 0]
            #print(str(xyz[0])+", "+str(xyz[1]))
            if maze[currentL[0]+ctrl[0]][currentL[1]-ctrl[1]] in [1,3,4]:
                currentL[0]=currentL[0]+ctrl[0]
                currentL[1]=currentL[1]-ctrl[1]
            l=pos(currentL[0],currentL[1])
            tft.jpg(my_path+"/pic/"+pC[5],l[0],l[1],gc9a01.FAST)
            #tft.text(font0,fC[5],l[0],l[1],bC[5],gc9a01.BLACK)
            if (currentL[0]== height-1 and currentL[1]== endL):
                tt=time.ticks_diff(time.ticks_ms(), start)
                play = False
                tft.fill(gc9a01.WHITE)
                maze = []
                gc.collect()
#                tft.jpg(my_path+"/pic/"+pC[6],0,0,gc9a01.FAST)
                tft.draw(font1,"Congratulations!",10,100,gc9a01.RED)
                tft.draw(font1,"you win!",50,130,gc9a01.BLACK)
                tft.text(font0,"Time you take = "+str(tt)+"ms",50,156,gc9a01.RED if tt < record else gc9a01.GREEN,gc9a01.WHITE)
                tft.text(font0,"Best record = "+str(record)+"ms",50,172,gc9a01.BLACK,gc9a01.WHITE)
                record = tt if tt < record else record
                time.sleep(3)
            time.sleep(0.1)
main()