#!/usr/bin/python3
#
# Displays date, time, temperature, humidity and weather forecast on a 2004
# 4-line LCD display.
#

import socket
import os.path
import sys
import struct
import fcntl
import os
import time
import liquidcrystal_i2c
from forecast import fetch_forecast
import RPi.GPIO as GPIO
import dht11
import time

CHARS_PER_LINE=20

class InfoDisplay:
    def __init__(self):
        self._init_dht11()
        self._init_lcd()
        self.second_line_interval = 5 # seconds
        self.temperature = 0.0
        self.humidity = 0.0
        self.forecast = fetch_forecast()

    def _init_lcd(self):
        self.lcd = liquidcrystal_i2c.LiquidCrystal_I2C(0x27, 1, numlines=4)

    def _init_dht11(self):
        GPIO.setwarnings(False)
        GPIO.setmode(GPIO.BCM)
        GPIO.cleanup()
        self.dht11_sensor = dht11.DHT11(4)

    def print_sensor_info(self):
        result = self.dht11_sensor.read()
        if result.is_valid():
            t = result.temperature
            h = result.humidity
            self.temperature = t
            self.humidity = h
        else:
            t = self.temperature
            h = self.humidity
        text = 'Room: %.0f\xdfC %dH' % (t, h)
        text += (CHARS_PER_LINE-len(text))*' ' # pad with spaces on right
        self.lcd.printline(1, text)

    def print_machine_info(self):
        core_temp = 0
        with open('/sys/class/thermal/thermal_zone0/temp', 'r') as f:
            core_temp = float(f.read()) / 1000.0
        load_avg_1min = ''
        with open('/proc/loadavg', 'r') as f:
            load_avg_1min = float(f.read().split(' ')[0])
        text = 'CPU %-2.0f\xdfC LdAvg %.1f' % (core_temp, load_avg_1min)
        text += (CHARS_PER_LINE-len(text))*' ' # pad with spaces on right
        self.lcd.printline(3, text)

    def print_forecast(self):
        text = '%s' % self.forecast
        text += (CHARS_PER_LINE-len(text))*' ' # pad with spaces on right
        self.lcd.printline(2, text)

    def print_time(self):
        tm = time.localtime()
        text = '%d-%02d-%02d %02d:%02d' % (tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min)
        self.lcd.printline(0, text)

    def run(self):
        t = 0.0
        h = 0.0
        i = 0
        while True:
            now = time.localtime()
            self.print_time()
            self.print_sensor_info()
            self.print_forecast()
            self.print_machine_info()
            if (now.tm_min == 0 or self.forecast is None) and now.tm_sec == 0:
                forecast = fetch_forecast()
                if forecast is not None:
                    self.forecast = forecast
            time.sleep(1)

if __name__ == '__main__':
    info_display = InfoDisplay()
    info_display.run()
