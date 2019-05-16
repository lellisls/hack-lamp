#!/usr/bin/python3
 
import gatt
import time
import threading
import argparse

class BTDevice(gatt.Device):
    light_intensity = None
    MAIN_SERVICE_UUID = "00005251-0000-1000-8000-00805f9b34fb"
    LIGHT_INTENSITY_UUID = "00002501-0000-1000-8000-00805f9b34fb"
    AUTH_UUID = "00002506-0000-1000-8000-00805f9b34fb"

    color = 0.0
    intensity = 0.0

    def tr(self, uuid):
        if uuid == self.MAIN_SERVICE_UUID:
            return "Main Service"
        elif uuid == self.LIGHT_INTENSITY_UUID:
            return "Light Intensity"
        elif uuid == self.AUTH_UUID:
            return "Auth"
        return uuid

    def connect_succeeded(self):
        super().connect_succeeded()
        print("[%s] Connected" % (self.mac_address))

    def connect_failed(self, error):
        super().connect_failed(error)
        print("[%s] Connection failed: %s" % (self.mac_address, str(error)))

    def disconnect_succeeded(self):
        super().disconnect_succeeded()
        print("[%s] Disconnected" % (self.mac_address))

    def services_resolved(self):
        super().services_resolved()

        print("[%s] Resolved services" % (self.mac_address))
        for service in self.services:
            print("[%s]  Service [%s]" % (self.mac_address, service.uuid))
            for characteristic in service.characteristics:
                print("[%s]    Characteristic [%s]" % (self.mac_address, characteristic.uuid))

        self.primary_service = next(
            s for s in self.services
            if s.uuid == self.MAIN_SERVICE_UUID)

        self.light_intensity = next(
            c for c in self.primary_service.characteristics
            if c.uuid == self.LIGHT_INTENSITY_UUID)
        
        self.auth = next(
            c for c in self.primary_service.characteristics
            if c.uuid == self.AUTH_UUID)

        self.auth.read_value()

        self.auth.write_value(bytes.fromhex("e7280a2d09"))

        # self.light_intensity.write_value(bytes.fromhex("F8F800"))
        self.set_light(self.intensity, self.color)

    def set_light(self, intensity, color):
        if not 0 <= intensity <= 1.0:
            return
        if not 0 <= color <= 1.0:
            return
        final_color = int(color * 252)
        white = int(intensity * color * 255)
        yellow = int(intensity * ( 1.0 - color) * 255)
        hex_value = "{:02x}{:02x}AA".format(white,yellow)
        print(f"Setting intensity to {intensity} and color to {color} : 0x{hex_value}")
        device.light_intensity.write_value(bytes.fromhex(hex_value))

    def characteristic_value_updated(self, characteristic, value):
        print(f"\t[{self.tr(characteristic.uuid)}] Value updated: ", value.hex())

    def characteristic_write_value_succeeded(self, characteristic):
        print(f"\t[{self.tr(characteristic.uuid)}] Write value succeeded")
    
    def characteristic_write_value_failed(self, characteristic, error):
        print(f"\t[{self.tr(characteristic.uuid)}] Write value failed: {error}")


class Manager(gatt.DeviceManager):
   def run(self):
      print("Running Manager")
      super().run()
   def quit(self):
      print("Stopping Manager")
      super().stop()  

class Range(object):
    def __init__(self, start, end):
        self.start = start
        self.end = end
    def __eq__(self, other):
        return self.start <= other <= self.end
    def __contains__(self, value):
        return self.start <= value <= self.end

parser = argparse.ArgumentParser(description='Send commands to my lamp.')

def restricted_float(x):
    x = float(x)
    if x < 0.0 or x > 1.0:
        raise argparse.ArgumentTypeError("%r not in range [0.0, 1.0]"%(x,))
    return x

parser.add_argument('intensity', type=restricted_float)
parser.add_argument('color', type=restricted_float)
args = parser.parse_args()

manager = Manager(adapter_name='hci0')

device = BTDevice(mac_address='F4:5E:AB:9D:A1:1A', manager=manager)
device.color = args.color
device.intensity = args.intensity

thread = threading.Thread(target = manager.run)
device.connect()
thread.start()

print("Started Thread.")

try:
    while True:
        pass
except KeyboardInterrupt:
    print('KeyboardInterrupt!')
    device.disconnect()
    manager.quit()
    thread.join()
