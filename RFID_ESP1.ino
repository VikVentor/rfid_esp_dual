#include <MFRC522.h> // Library for RFID-RC522
#include <SPI.h> // Library for SPI communication
#include <esp_now.h>
#include <WiFi.h>

#define SS_PIN    21
#define RST_PIN   22
#define greenPin  12
#define redPin    32

// Initialize RFID and SPI
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Sender ESP32 MAC address (Replace this with the second ESP32's MAC address)
uint8_t receiverMAC[6] = { 0xB8, 0xD6, 0x1A, 0x6A, 0xB0, 0x48 }; // Example MAC address, change to your second ESP32's MAC

// Structure for sending card data
typedef struct struct_message {
  byte uid[4];
} struct_message;

// Create a message structure
struct_message message;

// Array to store sent card UIDs
byte sentCardUIDs[10][4];  // Array to store sent card UIDs (you can change 10 to a different number if you expect more cards)
int sentCount = 0;         // Count of sent cards

void setup() {
  Serial.begin(115200);
  SPI.begin(); // Initialize SPI bus
  pinMode(greenPin, OUTPUT);
  pinMode(redPin, OUTPUT);

  // Initialize MFRC522 module
  mfrc522.PCD_Init();
  Serial.println("Approach your reader card...");

  // Initialize ESP-NOW
  WiFi.mode(WIFI_STA); // Set the mode to Station (Client)
  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW Initialization failed!");
    return;
  }

  // Register the peer device (second ESP32)
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }

  Serial.println("ESP-NOW peer added successfully!");
}

void loop() {
  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Check if the card has already been sent (stored in sentCardUIDs array)
  if (isCardAlreadySent(mfrc522.uid.uidByte)) {
    Serial.println("Card already recognized.");
    return;
  }

  // Turn on the green LED to indicate card detection
  digitalWrite(greenPin, HIGH);
  digitalWrite(redPin, LOW);

  // Display the UID of the card
  Serial.print("Card UID: ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    message.uid[i] = mfrc522.uid.uidByte[i]; // Store UID in message structure
  }
  Serial.println();

  // Send the UID data via ESP-NOW
  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&message, sizeof(message));
  if (result == ESP_OK) {
    Serial.println("Sent card UID data");
    storeSentCard(mfrc522.uid.uidByte);  // Store the card UID to avoid resending
  } else {
    Serial.println("Error sending message");
  }

  // Halt the card
  mfrc522.PICC_HaltA();

  // Reset LEDs after delay
  delay(1000);
  digitalWrite(greenPin, LOW);
  digitalWrite(redPin, HIGH);
}

// Function to check if the card UID has already been sent
bool isCardAlreadySent(byte *uid) {
  for (int i = 0; i < sentCount; i++) {
    bool match = true;
    for (byte j = 0; j < 4; j++) {
      if (sentCardUIDs[i][j] != uid[j]) {
        match = false;
        break;
      }
    }
    if (match) return true;  // Return true if the card is found in the sent list
  }
  return false;  // Card not found in the sent list
}

// Function to store the sent card UID in the sentCardUIDs array
void storeSentCard(byte *uid) {
  if (sentCount < 10) {  // Ensure we do not exceed the maximum number of stored UIDs
    for (byte i = 0; i < 4; i++) {
      sentCardUIDs[sentCount][i] = uid[i];
    }
    sentCount++;
  } else {
    Serial.println("Sent card storage is full.");
  }
}
