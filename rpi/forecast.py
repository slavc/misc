#!/usr/bin/python3

import time
from urllib.request import urlopen
import xml.etree.ElementTree as ET

# https://data.gov.ie/dataset/met-eireann-weather-forecast-api
url_fmt = 'http://metwdb-openaccess.ichec.ie/metno-wdb2ts/locationforecast?lat=%f;long=%f;from=%s;to=%s'
latitude = 53.3472753
longitude = -6.2588932

class Forecast:
    def __init__(self):
        pass

    def __str__(self):
        return '%.0fC %.0f%s %.0f%% %.0f%%' % (self.temp, self.wind_speed, self.wind_dir, self.humidity, self.precip)

def fetch_forecast():
    try:
        forecast = Forecast()
        now = time.gmtime()
        time_from = '%d-%02d-%02dT%02d:%02d' % (now.tm_year, now.tm_mon, now.tm_mday, now.tm_hour, now.tm_min)
        time_to = '%d-%02d-%02dT%02d:%02d' % (now.tm_year, now.tm_mon, now.tm_mday, now.tm_hour+1, now.tm_min)
        url = url_fmt % (latitude, longitude, time_from, time_to)
        response = urlopen(url).read()
        #print('Full response from %s:\n%s\n' % (url, response))
        root = ET.fromstring(response)
        product = root.find('product')
        location = product.find('time').find('location')
        forecast.temp = float(location.find('temperature').get('value'))
        forecast.wind_dir = location.find('windDirection').get('name')
        forecast.wind_speed = float(location.find('windSpeed').get('mps'))
        forecast.humidity = float(location.find('humidity').get('value'))
        forecast.cloudiness = float(location.find('cloudiness').get('percent'))
        location = product.findall('time')[1].find('location')
        forecast.precip = float(location.find('precipitation').get('probability'))
        return forecast
    except:
        return None

if __name__ == '__main__':
    forecast = fetch_forecast()
    print(forecast)
