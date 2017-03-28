from __future__ import print_function
from jumper_sdk import jumper
import unittest
import os

transmitter_bin = os.path.abspath(os.path.join(os.path.dirname(__file__), 'transmitter', '_build', 'nrf52832_xxaa.bin'))
receiver_bin = os.path.abspath(os.path.join(os.path.dirname(__file__), 'receiver', '_build', 'nrf52832_xxaa.bin'))

client = jumper.Client()
system = client.load_system()
print('Connected to system')
transmitter = system.endpoints[0].nrf52
receiver = system.endpoints[1].nrf52
radio_medium = system.radio_medium
temperature_sensor = system.endpoints[0].temperature_sensor

receiver.stop()
transmitter.stop()


class TestNrf52Temperature(unittest.TestCase):
    def setUp(self):
        print('Starting receiver')
        receiver.start_with_binary(receiver_bin)
        print('Starting transmitter')
        transmitter.start_with_binary(transmitter_bin)
        
    def tearDown(self):
        print('Stopping receiver')
        receiver.stop()
        print('Stopping transmitter')
        transmitter.stop()

    def test1_sanity(self):
        with transmitter.uart.record_lines(read_timeout=5, line_separator='\r\n') as uart_sniffer:
            try:
                line = uart_sniffer.wait_for_specific_line('APP:INFO:Sent temperature: -49')
            except jumper.TimeoutError:
                self.fail('Did not receive expected line')
            print(line)

    def test2_changing_temperature(self):
        temperature_profile = [0, 1, 15, 90]

        for temperature in temperature_profile:
            print('Setting temperature to %d' % temperature)
            temperature_sensor.set_temperature(temperature)
            with transmitter.uart.record_lines(read_timeout=5, line_separator='\r\n') as transmitter_uart_sniffer,\
                    receiver.uart.record_lines(read_timeout=5, line_separator='\r\n') as receiver_uart_sniffer:
                try:
                    line = transmitter_uart_sniffer.wait_for_specific_line('APP:INFO:Sent temperature: %d' % temperature)
                except jumper.TimeoutError:
                    self.fail('Did not receive expected line')
                print (line)
                try:
                    line = receiver_uart_sniffer.wait_for_specific_line('APP:INFO:Received temperature %d' % temperature)
                except jumper.TimeoutError:
                    self.fail('Did not receive expected line')
                print(line)

if __name__ == '__main__':
    unittest.main()