#include <SPI.h>
#include <Adafruit_PN532.h>
#include <Keyboard.h>

// Software SPI pin definitions
#define PN532_SCK   13  // Serial Clock
#define PN532_MISO  11  // Master In, Slave Out
#define PN532_MOSI  12  // Master Out, Slave In
#define PN532_SS    10  // Slave Select
#define PN532_RST   9   // Reset pin

// Instantiate the PN532 using software SPI (SCK, MISO, MOSI, SS)
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);

// Inactivity timer variables
unsigned long lastCardDetected = 0;
const unsigned long INACTIVITY_TIMEOUT = 10000; // 10 seconds without detection
const unsigned long POWER_DOWN_DURATION = 5000;   // 5 seconds power down

// Initialize PN532 (used during startup and after power-up)
void initPN532() {
  nfc.begin();
  nfc.SAMConfig();
  // Update the timer so that we consider the module active now.
  lastCardDetected = millis();
}

void setup() {
  // Begin USB keyboard emulation (Arduino Micro only)
  Keyboard.begin();

  // Set up the PN532 reset pin
  pinMode(PN532_RST, OUTPUT);
  digitalWrite(PN532_RST, HIGH);
  delay(100);

  // Initialize the PN532
  initPN532();
}

void loop() {
  uint8_t uid[7];    // Buffer for tag's UID (max 7 bytes)
  uint8_t uidLength = 0;
  
  // Attempt to detect a tag with a short timeout (500 ms)
  bool cardFound = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 500);

  if (cardFound) {
    // Reset the inactivity timer upon detecting a card
    lastCardDetected = millis();
    
    // Build a continuous hexadecimal string for the UID (no spaces)
    char uidString[16];  // Enough for up to 7 bytes * 2 hex digits + null terminator
    int index = 0;
    for (uint8_t i = 0; i < uidLength; i++) {
      index += sprintf(&uidString[index], "%02X", uid[i]);
    }
    uidString[index] = '\0';

    // Emulate typing the UID via USB keyboard (followed by a newline)
    Keyboard.print(uidString);
    Keyboard.print("\n");

    // Wait until the tag is removed to avoid sending the UID repeatedly
    while (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
      delay(100);
    }
  }
  
  // If no card has been detected for a while, power down the PN532
  if (millis() - lastCardDetected > INACTIVITY_TIMEOUT) {
    // Power down the PN532 by pulling its reset pin low
    digitalWrite(PN532_RST, LOW);
    delay(POWER_DOWN_DURATION);

    // Power the PN532 back up and reinitialize it
    digitalWrite(PN532_RST, HIGH);
    delay(100);
    initPN532();
  }
  
  // Small delay to reduce polling frequency
  delay(100);
}
