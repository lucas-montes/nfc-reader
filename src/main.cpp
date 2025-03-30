#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <nfc_pn532.h>
#include <LiquidCrystal_I2C.h>

#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);
LiquidCrystal_I2C lcd(0x27, 16, 2); // 16x2 LCD Display

void setupLCD() {
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Initializing...");
}

void setupNFC() {
    Serial.begin(115200);
    Serial.println("\n🔵 Starting NFC Module...");
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("Number of Cores: %d\n", ESP.getChipCores());


    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();

    if (!versiondata) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("❌ PN532 NOT FOUND");
        Serial.println("❌ PN532 NOT FOUND");
        while (1); // Halt execution
    }

    Serial.print("✅ Found PN532, Firmware: ");
    Serial.print((versiondata >> 16) & 0xFF);
    Serial.print(".");
    Serial.println((versiondata >> 8) & 0xFF);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("PN532 Ready");
}

void readNtag2xx() {
    uint8_t data[4];
    Serial.println("🔹 Detected NTAG2xx tag (7-byte UID)");

    for (uint8_t i = 0; i < 42; i++) {
        if (nfc.ntag2xx_ReadPage(i, data)) {
            Serial.print("PAGE ");
            Serial.print(i);
            Serial.print(": ");
            nfc.PrintHexChar(data, 4);
        } else {
            Serial.print("PAGE ");
            Serial.print(i);
            Serial.println(": ❌ Unable to read");
        }
    }
}

void readMifareClassic(uint8_t *uid, uint8_t uidLength) {
    Serial.println("🔹 Detected MIFARE Classic Card");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MIFARE Classic");

    // Try to authenticate a block using default key (0xFFFFFFFFFFFF)
    uint8_t keyA[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keyA)) {
        Serial.println("✅ Authentication successful!");
    } else {
        Serial.println("❌ Authentication failed!");
    }
}

void loop() {
    uint8_t success;
    uint8_t uid[7];  // Buffer for UID
    uint8_t uidLength;

    Serial.println("\n🔄 Scanning for an NFC tag...");
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Card Detected");

        Serial.println("✅ Found an ISO14443A card");
        Serial.print("  UID Length: ");
        Serial.print(uidLength);
        Serial.println(" bytes");
        Serial.print("  UID Value: ");
        nfc.PrintHex(uid, uidLength);
        Serial.println("");

        if (uidLength == 4) {
            Serial.println("🔹 Possible MIFARE Classic or Ultralight");
            readMifareClassic(uid, uidLength);
        }
        else if (uidLength == 7) {
            Serial.println("🔹 Likely an NTAG or MIFARE Ultralight");
            readNtag2xx();
        }
        else {
            Serial.println("🔹 Unknown card type");
        }

        Serial.println("\n🔄 Place another card to scan again...");
        delay(2000);
    }
}

void setup() {
    setupLCD();
    setupNFC();
}
