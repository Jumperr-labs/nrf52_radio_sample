from __future__ import print_function
import os
from threading import Thread, Event
from subprocess import call
from Tkinter import Tk

from jumper_sdk import jumper

from ..common import enter_to_start, print_header, print_section
from ..gui import UartSniffer

build_script = os.path.join(os.path.dirname(__file__), 'build.sh')
bin_transmitter = os.path.join(os.path.dirname(__file__), 'transmitter', '_build', 'nrf52832_xxaa.bin')
bin_receiver = os.path.join(os.path.dirname(__file__), 'receiver', '_build', 'nrf52832_xxaa.bin')


def build():
    print('Building project binaries')
    call(build_script)
    print_section()


class TemperatureRadioSystem:
    def __init__(self):
        client = jumper.Client()
        self.system = client.load_system()
        print('Connected to system')
        print_section()
        self.receiver = self.system.endpoints[0].nrf52
        self.transmitter = self.system.endpoints[1].nrf52
        self.temperature_sensor = self.system.endpoints[1].temperature_sensor
        self.radio_medium = self.system.radio_medium
        self.gui_thread = None
        self.quit_event = Event()

    def __enter__(self):
        build()
        self.stop_devices()

        def gui():
            root = Tk()
            UartSniffer(root, self.receiver.uart, self.quit_event, 'white', 'green')
            UartSniffer(root, self.transmitter.uart, self.quit_event, 'white', 'red')

            def on_closing():
                root.destroy()
                root.quit()

            root.protocol("WM_DELETE_WINDOW", on_closing)
            root.mainloop()

        self.gui_thread = Thread(target=gui)
        self.gui_thread.start()

        return self

    def stop_devices(self):
        print('Turning the receiver off')
        self.receiver.stop()
        print('Turning the transmitter off')
        self.transmitter.stop()
        print_section()

    def start_devices(self):
        enter_to_start()
        print('Turning the receiver on')
        self.receiver.start_with_binary(bin_receiver)
        print('Turning the transmitter on')
        self.transmitter.start_with_binary(bin_transmitter)
        print_section()

    def transmitter_debug_mode(self):
        self.transmitter.open_gdb_server()
        print("Let's connect a debugger to the transmitter. Open the 'transmitter' project in Eclipse and start the debugger")
        self.change_temperature_mode()

    def change_temperature_mode(self):
        print('Enter an integer to set the temperature. (Press Enter to continue')
        while True:
            user_input = raw_input('Temperature: ')
            if user_input == '':
                break
            try:
                self.temperature_sensor.set_temperature(int(user_input))
            except ValueError:
                print('Please make sure to enter an integer value')
        print_section()

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.quit_event.set()