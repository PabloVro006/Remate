# IMPORT
import spidev
import RPi.GPIO as gpio
from time import sleep

# GPIO CONFIG
gpio.setwarnings(False)
gpio.setmode(gpio.BCM)
gpio.setup(8, gpio.OUT)
gpio.output(8, gpio.HIGH)

# SLAVE ACTIVATE
def activateSlave(state):
  gpio.output(8, not state)
  sleep(0.1)

# SEND DATA
def spiTx(txData):
  print(f"Tx Data: {txData[0]}")
  spi.xfer(txData)

def spiRx():
  rxData = spi.readbytes(2) # 2 bytes of len ("3" and "\0")
  print(f"Rx Data: {rxData[0]}")

def main(bus, cs):
  # SPI CONFIG
  global spi
  spi = spidev.SpiDev() # Instanza SPI
  spi.open(bus, cs)
  spi.max_speed_hz = 12500000 # 12,5MHz
  spi.mode = 0b00 #CPOL = 0, CPHA = 1EDGE (0)
  spi.bits_per_word = 8

  # MAIN LOOP
  try:
    while True:
      for i in range (11):
        activateSlave(True)
        txBufferData = [i] # data to send
        spiTx(txBufferData) # send data
        sleep(0.5)
        spiRx() # receive data
        sleep(0.1)
        activateSlave(False)
        print("")
        sleep(1)
  except KeyboardInterrupt:
    print(" Exiting")
  except Exception as e:
    print(f"Error: {e}")
  finally:
    spi.close()
    gpio.cleanup()

if __name__ == "__main__":
    main(0, 0)
