#include <ESP8266WiFi.h>
#include <espnow.h>
#include <CRC32.h>


uint8_t broadcastAddress[] = {0x94, 0xb9, 0x7e, 0x14, 0x60, 0x89};

typedef struct struct_message {
  char message[32];
  int pseudoRandInt;
  int status;
  uint32_t hash; 
} struct_message;

struct_message payload;

CRC32 crc;

const uint8_t INDICATOR = 2;

unsigned long lastTime = 0;  
unsigned long timerDelay = 6000;  // send readings timer

// CRC32 algorithm parameters (standard IEEE 802.3 polynomial)
#define CRC32_POLYNOMIAL 0xEDB88320L
#define CRC32_INITIAL    0xFFFFFFFFL
uint32_t calculateChecksum(const void* input, size_t size) {
    uint32_t crc = CRC32_INITIAL;
    
    if (input != NULL) {
        const uint8_t* data = (const uint8_t*)input;
        
        for (size_t i = 0; i < size; ++i) {
            uint8_t byte = data[i];
            
            for (int j = 0; j < 8; ++j) {
                uint8_t bit = (byte >> 7) & 1;
                crc ^= (bit << 31);
                byte <<= 1;
                
                if (crc & 0x80000000) {
                    crc = (crc << 1) ^ CRC32_POLYNOMIAL;
                } else {
                    crc <<= 1;
                }
            }
        }
    }
    
    return crc ^ CRC32_INITIAL;
}

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Last Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}
 
void setup() {
  Serial.begin(115200);
   WiFi.mode(WIFI_STA);

  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  pinMode(INDICATOR, OUTPUT);

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
}
 
void loop() {
  if ((millis() - lastTime) > timerDelay) {

    payload.pseudoRandInt = random(10,100);
    strcpy(payload.message, "Kygm message");
    payload.status = ((payload.pseudoRandInt % 2) == 0); //if the rand is even then true else false
    payload.hash = calculateChecksum(&payload.pseudoRandInt, sizeof(int));

    if(payload.status != 1)//looks backwards but this is the only way it seems to work
    {
      digitalWrite(INDICATOR, HIGH);
    }
    else {
      digitalWrite(INDICATOR, LOW);
    }

    Serial.println(payload.hash);
    Serial.println(payload.pseudoRandInt);

    esp_now_send(broadcastAddress, (uint8_t *) &payload, sizeof(payload));

    lastTime = millis();
  }
}