#!/usr/bin/python3

from gpiozero import LightSensor
import time

sensor = LightSensor(18)

while True:
    if sensor.light_detected:
        print("It's light!")
    else:
        print("It's dark!")
    time.sleep(1)
