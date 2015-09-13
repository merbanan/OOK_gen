# OOK_gen
OOK modulator for Arduino

Usage:

Compile the code with the Arduino IDE. Run the code on the arduino and connect to the serial port (9600 baud).
Press enter and you should get the following prompt output:

CMD: 
>

 
You can now access the following serial commands:
p - load packet, the command accepts hex bytes. (p 1234ABCD for example)
b - bits in a packet
r - amout of repeats
z - length of 0 bit in us
o - length of 1 bit in us
i - invert bits if enabled
m - choose modulation (0 for PWM and 1 for PPM)
d - Pulse / Pulse distance length in us
e - Packet pause length in us
a - preamble length (not implemented yet)
t - transmit signal

As an example:
r 6
b 33
o 698
z 213
e 8000
d 800
p 83d90bcb
t

will send a turn on signal to a power switch

p 83d903c7
t

will change the packet buffer to a command that will
turn off the power switch and then send the signal.

