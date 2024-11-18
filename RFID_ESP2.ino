#include <esp_now.h>
#include <WiFi.h>
#include <MFRC522.h>
#include <SPI.h>

// RFID pins
#define SS_PIN    21
#define RST_PIN   22
#define greenPin  12
#define redPin    32

// Initialize RFID and SPI
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Structure for receiving card data
typedef struct struct_message {
  byte uid[4];
} struct_message;

// Storage for received card UIDs
const int MAX_CARDS = 10;  // Maximum number of cards we can store
byte storedUIDs[MAX_CARDS][4]; // Array to store UIDs
int storedCount = 0;  // Count of stored UIDs

// Create a message structure
struct_message receivedMessage;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // Set the mode to Station (Client)

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization failed!");
    return;
  }

  // Set up the callback function to handle incoming messages
  esp_now_register_recv_cb(onDataReceived);

  // Initialize RFID reader
  SPI.begin();
  mfrc522.PCD_Init();

  // Initialize LED pins
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);

  Serial.println("Waiting for data...");
}

void loop() {
  // Check if a new card is present on second ESP32
  if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    // Display the UID of the card
    Serial.print("Card UID on second ESP32: ");
    byte newCardUID[4];
    for (byte i = 0; i < mfrc522.uid.size; i++) {
      newCardUID[i] = mfrc522.uid.uidByte[i];
      Serial.print(newCardUID[i] < 0x10 ? " 0" : " ");
      Serial.print(newCardUID[i], HEX);
    }
    Serial.println();

    // Check if the card is already stored
    if (isCardStored(newCardUID)) {
      // Remove the card if it exists in the stored list
      removeCard(newCardUID);
      Serial.println("Card removed from storage.");
      digitalWrite(greenPin, HIGH); // Green LED to indicate card removed
      delay(500);
      digitalWrite(greenPin, LOW);
    } else {
      // If not stored, add it
      storeCardUID(newCardUID);
      Serial.println("Card added to storage.");
      digitalWrite(redPin, HIGH); // Red LED to indicate card added
      delay(500);
      digitalWrite(redPin, LOW);
    }

    // Halt the card
    mfrc522.PICC_HaltA();
  }

  // Nothing to do in loop, waiting for data
}

// Callback function for receiving data
void onDataReceived(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
  // Copy the incoming data to the receivedMessage structure
  memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));

  // Display the received card UID
  Serial.print("Received Card UID: ");
  for (byte i = 0; i < 4; i++) {
    Serial.print(receivedMessage.uid[i] < 0x10 ? " 0" : " ");
    Serial.print(receivedMessage.uid[i], HEX);
  }
  Serial.println();

  // Store the received card UID in storage
  storeCardUID(receivedMessage.uid);
}

// Function to check if a card UID is already stored
bool isCardStored(byte *uid) {
  for (int i = 0; i < storedCount; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (storedUIDs[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      Serial.println("Card found in stored list.");
      return true;
    }
  }
  Serial.println("Card not found in stored list.");
  return false;
}

// Function to store a new card UID
void storeCardUID(byte *uid) {
  if (storedCount < MAX_CARDS) {
    for (byte i = 0; i < 4; i++) {
      storedUIDs[storedCount][i] = uid[i];
    }
    storedCount++;
    Serial.print("Stored cards count: ");
    Serial.println(storedCount);
  } else {
    Serial.println("Storage full. Cannot store more cards.");
  }
}

// Function to remove a card from the stored list
void removeCard(byte *uid) {
  for (int i = 0; i < storedCount; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (storedUIDs[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) {
      // Shift the remaining cards to remove the matched card
      for (int k = i; k < storedCount - 1; k++) {
        for (byte j = 0; j < 4; j++) {
          storedUIDs[k][j] = storedUIDs[k + 1][j];
        }
      }
      storedCount--;
      Serial.print("Stored cards count: ");
      Serial.println(storedCount);
      return;
    }
  }
  Serial.println("Card not found in storage.");
}
