# ESP-PIX - Sistema de Pagamento PIX para ESP32

Projeto convertido para **ESP-IDF 5.5.0** (anteriormente PlatformIO/Arduino).

## Funcionalidades

- Display ST7735 com QR Code PIX
- WiFi para comunicação com backend
- Cliente HTTP para criação e verificação de cobranças
- Servo motor para dispenser de produtos
- Buzzer para feedback sonoro
- LED de status
- Botão para iniciar cobrança (toque rápido) e cancelar (pressionar 3s)

## Estrutura do Projeto

```
esp-pix/
├── CMakeLists.txt          # Arquivo principal do CMake
├── sdkconfig.defaults      # Configurações padrão
├── README.md
└── main/
    ├── CMakeLists.txt      # Componentes do main
    ├── Kconfig.projbuild   # Configurações do menuconfig
    ├── app_main.c          # Aplicação principal
    ├── wifi_manager.c/h    # Gerenciamento WiFi
    ├── http_client.c/h     # Cliente HTTP
    ├── display_st7735.c/h  # Driver do display
    ├── qrcode_gen.c/h      # Gerador de QR Code
    ├── servo_ctrl.c/h      # Controle do servo
    └── buzzer.c/h          # Controle do buzzer
```

## Pré-requisitos

1. **ESP-IDF 5.5.0** instalado e configurado
2. ESP32 DevKit
3. Display ST7735 128x160
4. Servo motor
5. Buzzer passivo
6. Botão push

## Pinagem Padrão

| Componente | GPIO |
|------------|------|
| LED        | 2    |
| Botão      | 4    |
| Buzzer     | 15   |
| Servo      | 13   |
| TFT CS     | 5    |
| TFT DC     | 16   |
| TFT RST    | 17   |
| TFT MOSI   | 23   |
| TFT SCK    | 18   |

## Compilação e Flash

### 1. Configurar o ambiente ESP-IDF

```bash
. $HOME/esp/esp-idf/export.sh
```

### 2. Configurar o projeto

```bash
idf.py menuconfig
```

Navegue até **ESP-PIX Configuration** para ajustar:
- WiFi SSID e senha
- URL do backend
- GPIOs
- Timeout do pagamento

### 3. Compilar

```bash
idf.py build
```

### 4. Flash

```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

## Configuração via menuconfig

Todas as configurações podem ser alteradas via `idf.py menuconfig`:

```
ESP-PIX Configuration  --->
    (EDILMA_2.5) WiFi SSID
    (password) WiFi Password
    (http://localhost:3000/api) Backend URL
    (2) LED GPIO
    (4) Button GPIO
    (15) Buzzer GPIO
    (13) Servo GPIO
    (5) TFT CS GPIO
    (16) TFT DC GPIO
    (17) TFT Reset GPIO
    (23) TFT MOSI GPIO
    (18) TFT SCK GPIO
    (60000) Payment Timeout (ms)
```

## API Backend Esperada

### POST /api/create_payment

Request:
```json
{
    "amount": 0.50,
    "description": "Produto teste"
}
```

Response:
```json
{
    "paymentId": "abc123",
    "qrCode": "00020126...",
    "amount": 50
}
```

### GET /api/status/{paymentId}

Response:
```json
{
    "status": "APPROVED"  // ou "PENDING", "REJECTED"
}
```

## Diferenças da versão Arduino

| Arduino/PlatformIO | ESP-IDF 5.5.0 |
|--------------------|---------------|
| `WiFi.h` | `esp_wifi.h` |
| `HTTPClient.h` | `esp_http_client.h` |
| `ArduinoJson` | `cJSON` (built-in) |
| `Adafruit_ST7735` | Driver SPI nativo |
| `ESP32Servo` | LEDC PWM |
| `tone()/noTone()` | LEDC PWM |
| `delay()` | `vTaskDelay()` |
| `millis()` | `esp_timer_get_time()` |

## Licença

MIT License
