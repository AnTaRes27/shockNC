#include <Arduino.h>
#include <RadioLib.h>
// #include <CC1101.h>
#include <CRC16.h>
#include <SPI.h>

#define LED_INTERNAL 2

SPIClass spi(VSPI);
SPISettings SPISettings(1000000, MSBFIRST, SPI_MODE0);

// CS pin: 5
// GDO0: 26
// RST: NC
// GDO2: 27
CC1101 radio = new Module(5, 26, RADIOLIB_NC, 27, spi, SPISettings);

// crc
const uint16_t poly = 0x1021;
const uint16_t init_value = 0x3be7;
const bool reflect_in = false;
const bool reflect_out = false;
const uint16_t Xor_out = 0x0000;

CRC16 crc(poly, init_value, Xor_out, reflect_in, reflect_out);

void setup()
{
  Serial.begin(9600);

  // initialize CC1101
  spi.begin();
  Serial.println("initializing...");
  int state = radio.begin();
  Serial.println(state);

  radio.setFrequency(433.623);
  radio.setBitRate(1);
  radio.setPreambleLength(128, 4);

  pinMode(LED_INTERNAL, OUTPUT);
}

uint16_t calculated_crc;
uint8_t rolling_code = 27;

void loop()
{
  uint8_t command[14];
  uint8_t x = 0;

  // synchronization (6 bytes)
  // this is always constant
  command[0] = 0x3a;
  command[1] = 0x97;
  command[2] = 0xb2;
  command[3] = 0x59;
  command[4] = 0xc7;
  command[5] = 0x4f;

  // key (2 bytes)
  // this is unique per transmitter. arbitrary
  command[6] = 0x53;
  command[7] = 0xc5;

  // constant1 (4 bits)
  // this is always constant
  command[8] = 0x0a << 4;

  // channel (4 bits)
  // 4 different options: 0x5, 0x6, 0x7, 0x8,
  command[8] = 0x05 | command[8];

  // rolling code (1 byte)
  // 0-255, increment by 1 each time
  command[9] = rolling_code++;

  // constant2 (4 bits)
  // this is always constant
  command[10] = 0x07 << 4;

  // function (4 bits)
  // shock: 0x01
  // buzz: 0x02
  // beep: 0x03
  command[10] = 0x02 | command[10];

  // strength (1 byte)
  // shock: 1-99
  // buzz: 1-9
  command[11] = 5;

  // checksum
  // check CRC parameters above
  crc.restart();
  for (uint8_t i = 0; i < 12; i++)
  {
    Serial.print(command[i], HEX);
    Serial.print(' ');
    crc.add(command[i]);
  }
  calculated_crc = crc.calc();
  command[12] = calculated_crc >> 8 & 0xFF;
  Serial.print(command[12], HEX);
  Serial.print(' ');
  command[13] = calculated_crc & 0xFF;
  Serial.print(command[13], HEX);
  Serial.print(' ');

  Serial.print(calculated_crc, HEX);
  Serial.print(' ');

  // transmit
  Serial.println(radio.transmit(command, 14));

  delay(500);
  digitalWrite(LED_INTERNAL, HIGH);
  delay(500);
  digitalWrite(LED_INTERNAL, LOW);
}