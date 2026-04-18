#include <WiFi.h>
#include <esp_now.h>
#include <WebServer.h>

// --- DATI DEL TUO WI-FI ---
const char* ssid = "Flavia";
const char* password = "flaviawifi";

// --- INSERISCI QUI I MAC ADDRESS DEI DUE ESP8266 ---
uint8_t macESP1[] = {0x68, 0xc6, 0x3a, 0xd7, 0x71, 0x96}; 
uint8_t macESP2[] = {0x68, 0xc6, 0x3a, 0xd5, 0x33, 0xdb};
WebServer server(80);

typedef struct struct_message {
    char text[100];
} struct_message;

struct_message myData;

// Interfaccia Web
void handleRoot() {
  String html = "<html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:sans-serif; text-align:center; background:#eceff1; padding:20px;} ";
  html += ".card{background:white; padding:30px; border-radius:15px; box-shadow:0 4px 6px rgba(0,0,0,0.1); display:inline-block;} ";
  html += "input{padding:12px; width:100%; box-sizing:border-box; margin-bottom:20px; border:1px solid #ccc; border-radius:5px;} ";
  html += ".btn{padding:15px; color:white; border:none; border-radius:5px; cursor:pointer; width:48%; font-weight:bold;} ";
  html += ".b1{background:#007bff;} .b2{background:#28a745;}</style></head><body>";
  html += "<div class='card'><h2>Invia Messaggio alle Matrici</h2>";
  html += "<form action='/send' method='POST'>";
  html += "<input type='text' name='msg' placeholder='Scrivi qui...' required>";
  html += "<button type='submit' name='dest' value='1' class='btn b1'>Invia a ESP 1</button> ";
  html += "<button type='submit' name='dest' value='2' class='btn b2'>Invia a ESP 2</button>";
  html += "</form></div></body></html>";
  server.send(200, "text/html", html);
}

void handleSend() {
  if (server.hasArg("msg") && server.hasArg("dest")) {
    String message = server.arg("msg");
    int dest = server.arg("dest").toInt();
    
    message.toCharArray(myData.text, 100);
    
    if (dest == 1) {
      esp_now_send(macESP1, (uint8_t *) &myData, sizeof(myData));
    } else {
      esp_now_send(macESP2, (uint8_t *) &myData, sizeof(myData));
    }
    
    // Ritorna subito alla pagina principale
    server.sendHeader("Location", "/");
    server.send(303);
  }
}

void setup() {
  Serial.begin(115200);

  // 1. Connessione al Wi-Fi di casa
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  
  Serial.print("Connessione a "); Serial.println(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }

  Serial.println("\nConnesso!");
  Serial.print("IP per PC/Smartphone: ");
  Serial.println(WiFi.localIP());
  Serial.print("Canale Wi-Fi in uso: ");
  Serial.println(WiFi.channel()); // Fondamentale per gli ESP8266!

  // 2. Inizializzazione ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore ESP-NOW");
    return;
  }

  // 3. Aggiunta dei destinatari
  esp_now_peer_info_t peerInfo = {};
  peerInfo.channel = WiFi.channel(); 
  peerInfo.encrypt = false;

  memcpy(peerInfo.peer_addr, macESP1, 6);
  esp_now_add_peer(&peerInfo);
  
  memcpy(peerInfo.peer_addr, macESP2, 6);
  esp_now_add_peer(&peerInfo);

  // 4. Avvio Server
  server.on("/", handleRoot);
  server.on("/send", HTTP_POST, handleSend);
  server.begin();
}

void loop() {
  server.handleClient();
}