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

  // synchronization
  command[0] = 0x3a;
  command[1] = 0x97;
  command[2] = 0xb2;
  command[3] = 0x59;
  command[4] = 0xc7;
  command[5] = 0x4f;
  // key
  command[6] = 0x53;
  command[7] = 0xc5;
  // constant1
  command[8] = 0b1010 << 4;
  // channel
  command[8] = 0b0101 | command[8];
  // rolling code
  command[9] = rolling_code++;
  // constant2
  command[10] = 0b0111 << 4;
  // function
  command[10] = 0b0010 | command[10];
  // strength
  command[11] = 5;

  // checksum
  // calculated_crc = crc.add;

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

  Serial.println(radio.transmit(command, 14));

  delay(500);
  digitalWrite(LED_INTERNAL, HIGH);
  delay(500);
  digitalWrite(LED_INTERNAL, LOW);
}