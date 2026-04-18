#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <espnow.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// --- MATRIX CONFIGURATION ---
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4
#define CLK_PIN   14 // D5
#define DATA_PIN  13 // D7
#define CS_PIN    15 // D8

MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// --- WEBSERVER CONFIGURATION ---
const char* ssid = "Flavia";
const char* password = "flaviawifi";
// --- ESP8266 MAC ADDRESSES ---
uint8_t macESP1[] = {0x68, 0xc6, 0x3a, 0xd7, 0x71, 0x96}; // Alessandro
uint8_t macESP2[] = {0x68, 0xc6, 0x3a, 0xd5, 0x33, 0xdb}; // Antonio
uint8_t* macESP = NULL;

ESP8266WebServer server(80);

char incomingMsg[128];
char outgoingMsg[128];

// Struttura dati per l'invio
typedef struct struct_message {
    char text[128];
} struct_message;

struct_message myData;

void utf8ToAscii(char* s) {
  int k = 0;
  for (int i = 0; s[i] != '\0'; i++) {
    unsigned char c = (unsigned char)s[i];
    if (c == 0xC3) {
      i++;
      switch ((unsigned char)s[i]) {
        case 0xA0: s[k++] = 0xE0; break; // à
        case 0xA8: s[k++] = 0xE8; break; // è
        case 0xA9: s[k++] = 0xE9; break; // é
        case 0xAC: s[k++] = 0xEC; break; // ì
        case 0xB2: s[k++] = 0xF2; break; // ò
        case 0xB9: s[k++] = 0xF9; break; // ù
        default: break; 
      }
    } else {
      s[k++] = s[i];
    }
  }
  s[k] = '\0'; // Chiude la stringa correttamente
}

// HTML page
void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'></head><body>"
                "<h2>P.I.G. sender 🐷</h2>"
                "<form action='/send' method='POST'>"
                "<input type='text' name='message' maxlength='127'>"
                "<input type='submit' value='SEND'>"
                "</form></body></html>";
  server.send(200, "text/html", html);
}

// Send data using ESP-NOW
void handleForm() {
  if (server.hasArg("message")) {
    String msg = server.arg("message");
    msg.toCharArray(myData.text, 128);
    
    esp_now_send(macESP, (uint8_t *) &myData, sizeof(myData));
    
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

//Example with curl: curl -X POST http://192.168.1.140/recv --data-binary curl
void handleRecv() {    
  String msg = server.arg("plain");
  msg.toCharArray(myData.text, 128);
  
  Serial.println(msg);
  OnDataRecv(NULL, (uint8_t *) &myData, sizeof(myData));
  server.send(200);
}

void setup() {
  Serial.begin(115200);
  String Macaddress = WiFi.macAddress();

  if (Macaddress[5] == 0x96) {
    macESP = macESP2;
  } else {
    macESP = macESP1;
  }

  // LED matrix
  P.begin();
  P.setIntensity(2);

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  WiFi.localIP().toString().toCharArray(incomingMsg, 128);
  P.displayText(incomingMsg, PA_CENTER, 60, 2000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);

  // ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Errore inizializzazione ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_add_peer(macESP, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
  esp_now_register_recv_cb(OnDataRecv);

  server.on("/", handleRoot);
  server.on("/send", HTTP_POST, handleForm);
  server.on("/recv", HTTP_POST, handleRecv);
  server.begin();
}

// Print received message for debugging
void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&myData, incomingData, sizeof(myData));
  
  utf8ToAscii(myData.text);
  strcpy(incomingMsg, myData.text);
  P.displayClear();
  P.displayText(incomingMsg, PA_CENTER, 60, 2000, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
}

void loop() {
  server.handleClient();
  P.displayAnimate();
}