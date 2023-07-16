import serial

print("A7,A6,A4,B3,B2,C3,C1,C0")

ser = serial.Serial('/dev/ttyACM1')

while True:
	b = bin(int(ser.read(1).hex(), base=16))[2:]
	b = (8 - len(b)) * "0" + b
	print( ",".join(b) )
