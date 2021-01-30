#!/usr/bin/python3

from gpiozero import OutputDevice
import signal
import time

def msleep(ms):
    time.sleep(ms / 1000.0)

def usleep(us):
    time.sleep(us / 1000000.0)

def nsleep(ns):
    time.sleep(ns / 1000000000.0)


class LCD:
    def __init__(self, pins):
        self.rs = OutputDevice(pins[0])
        self.rw = OutputDevice(pins[1])
        self.e = OutputDevice(pins[2])
        self.db0 = OutputDevice(pins[3])
        self.db1 = OutputDevice(pins[4])
        self.db2 = OutputDevice(pins[5])
        self.db3 = OutputDevice(pins[6])
        self.db4 = OutputDevice(pins[7])
        self.db5 = OutputDevice(pins[8])
        self.db6 = OutputDevice(pins[9])
        self.db7 = OutputDevice(pins[10])
        self.data = [self.db0, self.db1, self.db2, self.db3, self.db4, self.db5, self.db6, self.db7]

    def _start_cmd(self):
        self.rs.off()
        self.rw.off()
        self.e.on()

    def _start_data(self):
        self.rs.on()
        self.rw.off()
        self.e.on()

    def _clock_in(self):
        usleep(1)
        self.e.off()
        usleep(1)

    def _clear_data(self):
        for i in range(8):
            self.data[i].value = 0

    def clear(self):
        self._start_cmd()
        self._clear_data()
        self.data[0].on()
        self._clock_in()
        msleep(1.52)

    def home(self):
        self._start_cmd()
        self._clear_data()
        self.data[1].value = 1
        self._clock_in()
        msleep(1.52)

    def init(self, on=1, cursor=0, blink=0):
        self._start_cmd()
        self._clear_data()
        self.data[3].on()
        self.data[2].value = on
        self.data[1].value = cursor
        self.data[0].value = blink
        self._clock_in()
        usleep(1)

    def shift(self, screen=1, right=1):
        self._start_cmd()
        self._clear_data()
        self.data[4].value = 1
        self.data[3].value = screen
        self.data[2].value = right
        self._clock_in()
        usleep(1)

    def mode(self, inc=1, shift=0):
        self._start_cmd()
        self._clear_data()
        self.data[2].value = 1
        self.data[1].value = inc
        self.data[0].value = shift
        self._clock_in()
        usleep(1)

    def set_cgaddr(self, addr):
        self._start_cmd()
        self.data[7].value = 0
        self.data[6].value = 1
        for i in range(6):
            self.data[i].value = (addr & 1)
            addr >>= 1
        self._clock_in()
        usleep(1)

    def set_ddaddr(self, addr):
        self._start_cmd()
        self.data[7].value = 1
        for i in range(7):
            self.data[i].value = (addr & 1)
            addr >>= 1
        self._clock_in()
        usleep(1)

    def goto(self, x, y):
        self.set_ddaddr(0x40*y + x)
        usleep(1)

    def function(self, bus_width=1, num_lines=1, font=0):
        self._start_cmd()
        self._clear_data()
        self.data[5].value = 1
        self.data[4].value = bus_width
        self.data[3].value = num_lines
        self.data[2].value = font
        self._clock_in()
        usleep(1)

    def write(self, byte):
        self._start_data()
        for i in range(8):
            self.data[i].value = byte & 1
            byte >>= 1
        self._clock_in()
        usleep(1)

    def putchar(self, ch):
        self.write(ord(ch))

    def print(self, text):
        for ch in text:
            self.putchar(ch)

def main():
    lcd = LCD([2, 3, 4, 17, 27, 22, 10, 9, 11, 0, 5])

    lcd.init()
    lcd.function()
    lcd.clear()

    now = time.localtime()
    lcd.print('%02d:%02d:%02d' % (now.tm_hour, now.tm_min, now.tm_sec))

    lcd.goto(0, 1)
    lcd.print('Hello, world!')

    time.sleep(3)

    while True:
        lcd.shift()
        time.sleep(0.3)

    print('Press Ctrl+C to exit...')
    signal.pause()

def print_all_chars(lcd):
    for i in range(256):
        if i == 0 or (i % 16) == 0:
            print('clearing display...')
            lcd.clear()
        lcd.write(i)
        time.sleep(0.5)

if __name__ == '__main__':
    main()
