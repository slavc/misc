#!/usr/bin/python3
#
# Displays date, time, temperature, humidity and weather forecast on a 1602A
# LCD display.
# Also lights up an LED when it becomes dark.
#

import RPi.GPIO as GPIO
from lcd import LCD
from forecast import fetch_forecast
import dht11
from gpiozero import LightSensor
from gpiozero import LED
import time

class InfoDisplay:
    def __init__(self):
        self._init_dht11()
        self._init_lcd()
        self._init_light_sensor()
        self._init_dark_time_led()
        self.second_line_interval = 5 # seconds
        self.temperature = 0.0
        self.humidity = 0.0
        self.forecast = fetch_forecast()

    def _init_lcd(self):
        lcd = LCD([2, 3, 4, 17, 27, 22, 10, 9, 11, 0, 5])
        lcd.init()
        lcd.function()
        lcd.clear()
        self.lcd = lcd

    def _init_dht11(self):
        GPIO.setwarnings(False)
        GPIO.setmode(GPIO.BCM)
        GPIO.cleanup()
        self.dht11_sensor = dht11.DHT11(21)

    def _init_light_sensor(self):
        self.light_sensor = LightSensor(18)

    def _init_dark_time_led(self):
        self.led = LED(12)

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
        text = 'Room: %-2.0f\xdfC %-2d%%' % (t, h)
        text += (16-len(text))*' ' # pad with spaces on right
        self.lcd.goto(0, 1)
        self.lcd.print(text)

    def print_machine_info(self):
        core_temp = 0
        with open('/sys/class/thermal/thermal_zone0/temp', 'r') as f:
            core_temp = float(f.read()) / 1000.0
        load_avg_1min = ''
        with open('/proc/loadavg', 'r') as f:
            load_avg_1min = float(f.read().split(' ')[0])
        self.lcd.goto(0, 1)
        if core_temp >= 60.0:
            text = 'CPU @ %-2.0f\xdfC!' % core_temp
        else:
            text = 'Load Avg %.1f' % load_avg_1min
        text += (16-len(text))*' ' # pad with spaces on right
        self.lcd.print(text)

    def print_forecast(self):
        self.lcd.goto(0, 1)
        text = '%s' % self.forecast
        text += (16-len(text))*' ' # pad with spaces on right
        self.lcd.print(text)

    def print_time(self, tm):
        self.lcd.goto(0, 0)
        text = '%d-%02d-%02d %02d:%02d' % (tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min)
        self.lcd.print(text)

    def run(self):
        t = 0.0
        h = 0.0
        print_second_line_funcs = [self.print_sensor_info, self.print_forecast]
        i = 0
        while True:
            now = time.localtime()
            self.print_time(now)
            if now.tm_sec % self.second_line_interval == 0:
                i += 1
                i %= len(print_second_line_funcs)
            print_second_line_funcs[i]()

            if (now.tm_min == 0 or self.forecast is None) and now.tm_sec == 0:
                forecast = fetch_forecast()
                if forecast is not None:
                    self.forecast = forecast

            # FIXME should be separate OR abstract the class
            if self.light_sensor.light_detected:
                self.led.off()
            else:
                self.led.on()

            time.sleep(1)

if __name__ == '__main__':
    info_display = InfoDisplay()
    info_display.run()
