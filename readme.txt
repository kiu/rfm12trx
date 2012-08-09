rfm12trx - A simple RFM12 434Mhz transceiver
http://schoar.de/tinkering/rfm12trx

1. Flash the RT232RL through USB
    http://www.ftdichip.com/Support/Utilities.htm#FT_Prog
    See firmware/rfm12trx_ftprog.xml (12Mhz on CBUS0)

2. Flash the ATMega8 through ISP
    make fuse
    make program

3. Send/Receive hex strings via ttyUSBx, e.g. screen /dev/ttyUSB0 115200
