#include <Arduino.h> // Biblioteca principal
#include <WiFi.h> // Biblioteca para conexão WiFi
#include <HTTPClient.h> // Biblioteca para comunicação HTTP
#include <ArduinoJson.h> // Biblioteca para trabalhar com JSON
#include <Adafruit_GFX.h> // Biblioteca para controle do display
#include <Adafruit_ST7735.h> // Biblioteca para controle do display
#include <SPI.h> // Biblioteca para comunicação SPI
#include <qrcode.h> // Biblioteca para geração de QR Code
#include <ESP32Servo.h> // Biblioteca para controle do servo

// ==========================================================
// Configurações WiFi e backend
const char* ssid = "EDILMA_2.5";
const char* password = "#######";
const char* backend = "http://localhost:3000/api";

// ==========================================================
// Pinos
#define LED_PIN 2
#define BUTTON_PIN 4 // D4
#define BUZZER_PIN 15 // D15
#define TFT_CS   5 // D5
#define TFT_DC   16 // A0
#define TFT_RST  17 // D17
#define SERVO_PIN 13 // D13

Servo servo;
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
QRCode qrcode;

// ==========================================================
// Variáveis globais
String paymentId = ""; // para armazenar o ID do pagamento
String qrData = ""; // para armazenar o QR Code
float amounts = 0; // para armazenar o valor
bool systemActive = false; // para controle do sistema
unsigned long lastPoll = 0; // para controle de polling
unsigned long qrStartTime = 0; // para controle de tempo
const unsigned long qrTimeout = 60000; // 60 segundos (1 minuto)
void cancelCharge();

// ==========================================================
// Beep
void beep(int times, int duration = 200, int frequency = 1000) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, frequency);
    delay(duration);
    noTone(BUZZER_PIN);
    delay(50);
  }
}

// ==========================================================
// Funções de exibição no display
void showMessage(String title, String msg, uint16_t color = ST77XX_WHITE) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(color);

  int16_t x1, y1;
  uint16_t w, h;

  // --- Título ---
  tft.setTextSize(1.5);//
  tft.getTextBounds(title, 0, 0, &x1, &y1, &w, &h);   // calcula largura e altura do texto
  int titleX = (tft.width() - w) / 2;                  // centraliza horizontalmente
  int titleY = 30;                                     // posição vertical
  tft.setCursor(titleX, titleY);
  tft.println(title);

  // --- Mensagem ---
  tft.setTextSize(1);
  tft.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  int msgX = (tft.width() - w) / 2;
  int msgY = 80;                                       // posição vertical
  tft.setCursor(msgX, msgY);
  tft.println(msg);
}

void showQRCode(String payload, float amount) {
  unsigned long elapsed = millis() - qrStartTime;
  int remaining = (qrTimeout - elapsed) / 1000;

  tft.fillScreen(ST77XX_WHITE);

  // --- Inicializa QR Code ---
  uint8_t qrcodeData[qrcode_getBufferSize(8)];
  qrcode_initText(&qrcode, qrcodeData, 8, 0, payload.c_str());

  int scale = 2;

  // --- Centraliza QR Code ---
  int offsetX = (tft.width() - qrcode.size * scale) / 2;
  int offsetY = 20; // mais espaço para texto abaixo do QR

  // --- Desenha QR Code ---
  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      uint16_t color = qrcode_getModule(&qrcode, x, y) ? ST77XX_BLACK : ST77XX_WHITE;
      tft.fillRect(offsetX + x * scale, offsetY + y * scale, scale, scale, color);
    }
  }
  // --- Exibe valor ---
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1.5);
  tft.setCursor(10, tft.height() - 30); 
  tft.print(String((amount/100), 2) + " R$");

}


// ==========================================================
// Setup
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  delay(5000); 

  // Liga o servo e mantém ele parado
  servo.attach(SERVO_PIN); // liga o servo
  servo.write(90); // posicao inicial
  servo.detach(); // desliga o servo
  delay(50);

  // Inicializa display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  tft.fillScreen(ST77XX_BLACK);
  showMessage("Alo, Devs!", "");
  delay(2000);
  showMessage("ESP32 PIX", "Iniciando...");

  // Conecta WiFi
  WiFi.begin(ssid, password);
  Serial.print("Conectando ao WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
  }
  digitalWrite(LED_PIN, HIGH);
  Serial.println("\n✅ WiFi conectado!");
  showMessage("WiFi", "Conectado!");
  delay(1000);
  showMessage("Pronto", "Pressione o botao");
  beep(1, 200, 1500); // beep de boas-vindas
}

// ==========================================================
// Botão
bool handleButton() {
  static unsigned long pressStart = 0;
  static bool holding = false;
  bool pressed = digitalRead(BUTTON_PIN) == LOW;

  if (pressed && pressStart == 0) pressStart = millis();

  if (pressed && (millis() - pressStart > 3000) && systemActive && !holding) {
    holding = true;
    Serial.println("⛔ Cancelando cobrança...");
    showMessage("Cancelando", "Aguarde...", ST77XX_RED);
    cancelCharge();
  }

  if (!pressed && pressStart > 0) {
    unsigned long pressTime = millis() - pressStart;
    pressStart = 0;
    holding = false;

    if (pressTime < 800 && !systemActive) return true; // toque rápido
  }

  return false;
}

// ==========================================================
// Criar cobrança
void createCharge(float amount, const char* description) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi desconectado!");
    beep(3, 150, 500);
    showMessage("Erro", "Sem WiFi!", ST77XX_RED);
    return;
  }

  HTTPClient http;
  String url = String(backend) + "/create_payment";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String body = String("{\"amount\":") + String(amount, 2) + ",\"description\":\"" + description + "\"}";
  Serial.println("➡️ Enviando: " + body);

  int code = http.POST(body);

  if (code == 200) {
    String payload = http.getString();

    JsonDocument doc; //
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      paymentId = doc["paymentId"].as<String>();
      qrData = doc["qrCode"].as<String>();
      amounts = doc["amount"].as<float>();

      showQRCode(qrData, amounts);
      beep(2, 150, 1500);
      systemActive = true;
      qrStartTime = millis(); // marca início da contagem
    } else {
      showMessage("Erro", "JSON invalido", ST77XX_RED);
      beep(3, 200, 500);
    }
  } else {
    Serial.printf("❌ Erro HTTP (%d)\n", code);
    showMessage("Erro HTTP", String(code).c_str(), ST77XX_RED);
    beep(3, 200, 500);
  }

  http.end();
}

// ==========================================================
// Cancelar cobrança
void cancelCharge() {
  servo.detach(); // desliga o servo
  if (paymentId.length() > 0) {
    Serial.println("❌ Cobrança cancelada!");
    paymentId = "";
  }
  systemActive = false;
  digitalWrite(LED_PIN, LOW);
  beep(2, 150, 600);
  showMessage("Cancelado", "Pressione o botao");
}

// ==========================================================
// Checar status do pagamento
bool checkPaymentStatus() {
  if (paymentId.isEmpty() || WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  String url = String(backend) + "/status/" + paymentId;
  http.begin(url);

  int code = http.GET(); // 200 OK
  if (code == 200) {
    String payload = http.getString();
    JsonDocument doc;
    if (deserializeJson(doc, payload) == DeserializationError::Ok) {
      String status = doc["status"].as<String>();
      if (status == "APPROVED") {
        beep(2, 150, 2000);
        return true;
      }
    }
  }
  http.end();
  return false;
}

void moverServo() {
  servo.attach(SERVO_PIN); // garante que está ligado

  // Vai de 0° até 180°
  for (int pos = 0; pos <= 90; pos++) {
    servo.write(pos);
    delay(10); 
  }

  delay(200);

  servo.write(90);     // garante posição final
  delay(200);          // pequena pausa
  servo.detach();      // 🔌 desliga o sinal do servo -> para totalmente
  Serial.println("✅ Movimento finalizado e servo desligado.");
}

// ==========================================================
// Liberar produto
void dispense() {
  Serial.println("💰 Pagamento confirmado!");
  showMessage("Pagamento", "Confirmado!", ST77XX_GREEN);
  beep(3, 150, 1800);
  moverServo();

  showMessage("Liberado", "Retire o produto");
  delay(3000);
  showMessage("Obrigado!", "Volte sempre!", ST77XX_GREEN);
  delay(2000);

  for (int i = 0; i < 5; i++) {
    digitalWrite(LED_PIN, LOW);
    delay(150);
    digitalWrite(LED_PIN, HIGH);
    delay(150);
  }

  paymentId = "";
  systemActive = false;
  showMessage("Pronto", "Pressione o botao");
}

// ==========================================================
// Exibir contador regressivo
void showCountdown() {
  if (!systemActive) return;

  unsigned long elapsed = millis() - qrStartTime;
  if (elapsed >= qrTimeout) {
    Serial.println("⏰ Tempo expirado!");
    showMessage("Expirado", "Cobranca cancelada", ST77XX_RED);
    cancelCharge();
    return;
  }

  int remaining = (qrTimeout - elapsed) / 1000;
  tft.fillRect(0, 155, 160, 10, ST77XX_WHITE); // limpa área inferior
  tft.setTextColor(ST77XX_BLACK);
  tft.setCursor(10, 140); //
  tft.setTextSize(1);
  tft.printf("Tempo restante: %ds", remaining);

}

// ==========================================================
// Loop
void loop() {
  if (handleButton()) {
    digitalWrite(LED_PIN, HIGH);
    beep(1, 150, 1500);
    showMessage("Gerando PIX", "Aguarde...", ST77XX_YELLOW);
    createCharge(0.50, "Produto teste");
  }

  if (systemActive && !paymentId.isEmpty()) { // cobrança ativa
    showCountdown();
    // verifica status a cada 5s
    if (millis() - lastPoll > 5000) { // 5s
      lastPoll = millis();
      if (checkPaymentStatus()) {
        dispense();
      } else {
        Serial.println("⏳ Aguardando pagamento...");
      }
    }
  }
}
