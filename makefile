
default: exe.ihex

# Upload:
u: exe.ihex
	avrdude -p atmega328pb -c stk500v2 -b 115200 -P usb -U flash:w:exe.ihex:i

exe.ihex: exe.elf
	avr-objcopy -O ihex -R .eeprom exe.elf exe.ihex


exe.elf: main.c ../custom_usart/usart.h
	avr-gcc -mmcu=atmega328pb -O1 main.c -o exe.elf


asm: main.c
	avr-gcc -mmcu=atmega328pb -S -O0 main.c -o main.asm


clean:
	rm *.hex *.ihex *.o *.asm *.elf
