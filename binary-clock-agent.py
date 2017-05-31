#!/usr/bin/python

import pytz
import datetime
import time
import sys
import paho.mqtt.client as mqtt

def on_connect(client, userdata,  rc):
    print("Connected with result code "+str(rc))
    client.subscribe("binary_clock/request")

# Should be battery data reporting from sensor
def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload))
    now = datetime.datetime.now(pytz.timezone('US/Central'))
    now_seconds = now - now.replace(hour=0,minute=0,second=0,microsecond=0)
    print(str(now_seconds.total_seconds()))
    client.publish("binary_clock/time",payload=str(now_seconds.total_seconds()),retain=False)
    
client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("192.168.1.111", 1883, 60)
client.loop_forever()
