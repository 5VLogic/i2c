# i2c
A project for the ATmega328PB to use the TWI0 peripheral for reading and sending data across the i2c bus.

About the versions of the ATmega you are using.

### If you are using a standard Arduino Uno / Nano
then you likely have the regular ATmega328P an you should remove
all trailing 0 (zeros) from the TWI register names.
Then remove the "B" from all occurrances of the ATmega328PB name
in the makefile.

### If you are using the PB version as me
you likely need to include the specific libraries.
For this follow the following steps:

1) Download libraries from atmel website: http://packs.download.atmel.com/

2) Put gcc/dev/atmega328pb/avr5/crtatmega328pb.o and
            gcc/dev/atmega328pb/avr5/libatmega328pb.a in
            /usr/lib/avr/lib/avr5/

3) Then put "include/avr/iom328pb.h" in /usr/lib/avr/include/

4) Add the following lines to /usr/lib/avr/include/io.h:
            "
            elif defined (__AVR_ATmega328PB__)
            include <avr/iom328pb.h>
            "
Note: the two library files are also available in the folder "pb_lib"
