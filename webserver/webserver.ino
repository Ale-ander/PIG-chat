#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <espnow.h>

// --- CONFIGURAZIONE ---
const char* ssid = "Flavia";
const char* password = "flaviawifi";
// --- INSERISCI QUI I MAC ADDRESS DEI DUE ESP8266 ---
uint8_t macESP1[] = {0x68, 0xc6, 0x3a, 0xd7, 0x71, 0x96}; // Alessandro
uint8_t macESP2[] = {0x68, 0xc6, 0x3a, 0xd5, 0x33, 0xdb}; // Antonio

ESP8266WebServer server(80);

// Struttura dati per l'invio
typedef struct struct_message {
    char text[32];
} struct_message;

struct_message myData;

// Callback invio (opzionale, per debug)
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Stato invio: ");
  Serial.println(sendStatus == 0 ? "Successo" : "Fallimento");
}

// Pagina HTML
void handleRoot() {
  String html = "<html><body>"
                "<h2>ESP-NOW Sender</h2>"
                "<form action='/send' method='POST'>"
                "<input type='text' name='message' maxlength='31'>"
                "<input type='submit' value='SEND'>"
                "</form></body></html>";
  server.send(200, "text/html", html);
}

// Gestione ricezione dati dal form
void handleForm() {
  if (server.hasArg("message")) {
    String msg = server.arg("message");
    msg.toCharArray(myData.text, 32);
    
    // Invia via ESP-NOW
    esp_now_send(macESP2, (uint8_t *) &myData, sizeof(myData));
    
    server.send(200, "text/html", "Inviato: " + msg + "<br><a href='/'>Indietro</a>");
  }
}

void setup() {
  Serial.begin(115200);

  // Connessione WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnesso! IP: " + WiFi.localIP().toString());

  // Inizializza ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Errore inizializzazione ESP-NOW");
    return;
  }

  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(OnDataSent);
  
  // Aggiungi il destinatario
  esp_now_add_peer(macESP2, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

  // Registra la funzione callback creata sopra
  esp_now_register_recv_cb(OnDataRecv);

  // Rotte Web Server
  server.on("/", handleRoot);
  server.on("/send", HTTP_POST, handleForm);
  server.begin();
}

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  // Copia i dati in arrivo nella nostra struct
  memcpy(&myData, incomingData, sizeof(myData));
  
  // Stampa sulla console seriale
  Serial.println();
  Serial.print("--- Messaggio Ricevuto ---");
  Serial.print("Da MAC: ");
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
  Serial.print("Contenuto: ");
  Serial.println(myData.text);
  Serial.println("--------------------------");
}

void loop() {
  server.handleClient();
}