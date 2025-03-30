#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <nfc_pn532.h>


#define SDA_PIN 21
#define SCL_PIN 22

Adafruit_PN532 nfc(SDA_PIN, SCL_PIN);



uint8_t defaultKeys[][6] = {
    {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7},  // Example custom key (D3 F7 D3 F7 D3 F7)
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  // Default Key A (FF FF FF FF FF FF)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  // Default Key B (00 00 00 00 00 00)
    {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5},  // Example custom key (A0 A1 A2 A3 A4 A5)
    // Add more keys here as needed
};

int numKeys = sizeof(defaultKeys) / sizeof(defaultKeys[0]); // Total number of keys


void setupNFC() {
    Serial.begin(115200);
    Serial.println("\n🔵 Starting NFC Module...");
    Serial.printf("Chip Model: %s\n", ESP.getChipModel());
    Serial.printf("Chip Revision: %d\n", ESP.getChipRevision());
    Serial.printf("Number of Cores: %d\n", ESP.getChipCores());


    nfc.begin();
    uint32_t versiondata = nfc.getFirmwareVersion();


    if (!versiondata) {
        Serial.println("❌ PN532 NOT FOUND");
        while (1); // Halt execution
    }

    Serial.print("✅ Found PN532, Firmware: ");
    Serial.print((versiondata >> 16) & 0xFF);
    Serial.print(".");
    Serial.println((versiondata >> 8) & 0xFF);
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

bool readMifareClassicBlock(uint8_t sector) {
    // Read all 3 data blocks in this sector (blocks 0, 1, and 2)
    uint8_t fullData[48];  // 16 bytes * 3 blocks
    memset(fullData, 0, sizeof(fullData));  // Clear the buffer

    for (int blockOffset = 0; blockOffset < 3; blockOffset++) {
        uint8_t data[16];
        if (nfc.mifareclassic_ReadDataBlock(sector * 4 + blockOffset, data)) {
            memcpy(fullData + (blockOffset * 16), data, 16);  // Append data
        } else {
            Serial.println("❌ Failed to read block!");
            return false;
        }
    }

    // Print the full concatenated data
    Serial.print("📂 Full Data: ");
    nfc.PrintHexChar(fullData, 48);  // Print the full 48 bytes
    return true;
}

bool readMifareClassicSector(uint8_t *uid, uint8_t uidLength, uint8_t sector) {
    Serial.print("🔍 Reading Sector ");
    Serial.println(sector);
    for (int i = 0; i < numKeys; i++) {
        if (nfc.mifareclassic_AuthenticateBlock(uid, uidLength, sector * 4, 0, defaultKeys[i])) {
            return readMifareClassicBlock(sector);
        } else {
            Serial.println("❌ Authentication failed!");
            Serial.println(i);
        }
    };

    return false;
}

/* Mifare classic can be 1k. Those have 16 sectors, with 4 blocks of 16 bytes */
void readMifareClassic(uint8_t *uid, uint8_t uidLength) {
    Serial.println("🔹 Detected MIFARE Classic Card");
    readMifareClassicSector(uid, uidLength, 1);
}

void loop() {
    uint8_t success;
    uint8_t uid[7];  // Buffer for UID
    uint8_t uidLength;

    Serial.println("\n🔄 Scanning for an NFC tag...");
    success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

    if (success) {
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
    setupNFC();
}
