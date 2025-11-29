# ESP-PIX - Sistema de Pagamento PIX para ESP32

![visitors](https://visitor-badge.laobi.icu/badge?page_id=RapportTecnologia.esp32-pix-firmware)
[![Build](https://img.shields.io/github/actions/workflow/status/RapportTecnologia/esp32-pix-firmware/ci.yml?branch=main)](https://github.com/RapportTecnologia/esp32-pix-firmware/actions)
[![Issues](https://img.shields.io/github/issues/RapportTecnologia/esp32-pix-firmware)](https://github.com/RapportTecnologia/esp32-pix-firmware/issues)
[![Stars](https://img.shields.io/github/stars/RapportTecnologia/esp32-pix-firmware)](https://github.com/RapportTecnologia/esp32-pix-firmware/stargazers)
[![Forks](https://img.shields.io/github/forks/RapportTecnologia/esp32-pix-firmware)](https://github.com/RapportTecnologia/esp32-pix-firmware/network/members)
[![Language](https://img.shields.io/badge/Language-C%2FC%2B%2B-brightgreen.svg)]()
[![License: CC BY 4.0](https://img.shields.io/badge/license-CC%20BY%204.0-blue.svg)](https://creativecommons.org/licenses/by/4.0/)

---

> **Fork:** Este projeto é um fork/adaptação do trabalho original de [mazinhoandrade](https://github.com/mazinhoandrade).

Projeto convertido para **ESP-IDF 5.5.0** (anteriormente PlatformIO/Arduino).

## Funcionalidades

- Display ST7735 com QR Code PIX
- WiFi para comunicação com backend
- Cliente HTTP para criação e verificação de cobranças
- **Servidor HTTP REST** para configuração remota
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
    ├── http_server.c/h     # Servidor HTTP REST
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
| ---------- | ---- |
| LED        | 2    |
| Botão      | 4    |
| Buzzer     | 21   |
| Servo      | 13   |
| TFT CS     | 5    |
| TFT DC     | 20   |
| TFT RST    | 22   |
| TFT MOSI   | 23   |
| TFT SCK    | 6    |

> **⚠️ ESP32-P4:** Os GPIOs 14-19 são reservados para comunicação SDIO com o co-processador WiFi (ESP-Hosted). Não utilize esses pinos para outros periféricos.

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
    (21) Buzzer GPIO
    (13) Servo GPIO
    (5) TFT CS GPIO
    (20) TFT DC GPIO
    (22) TFT Reset GPIO
    (23) TFT MOSI GPIO
    (6) TFT SCK GPIO
    (60000) Payment Timeout (ms)
```

## API REST do Firmware

O ESP32 expõe um servidor HTTP na porta 80 para configuração remota. Ao iniciar, o IP é exibido no terminal e no display.

### GET /status

Verifica o status do dispositivo.

**Request:**

```bash
curl http://192.168.1.100/status
```

**Response:**

```json
{
    "status": "online",
    "device": "ESP32-PIX",
    "api_key_set": false
}
```

### GET /addapikey

Define a API key para validação de conexões com o frontend. A chave é persistida em NVS (Non-Volatile Storage).

**Request:**

```bash
curl "http://192.168.1.100/addapikey?key=minha_chave_secreta"
```

**Response (sucesso):**

```json
{
    "success": true,
    "message": "API key saved successfully",
    "key_length": 20
}
```

**Response (erro):**

```json
{
    "success": false,
    "error": "Missing key parameter"
}
```

> **Nota:** A API key é armazenada em NVS e persiste entre reinicializações do dispositivo.

---

## Configuração do Mercado Pago

Para utilizar o sistema de pagamentos PIX, é necessário criar uma aplicação no painel de desenvolvedores do Mercado Pago.

### 1. Acessar o Painel de Desenvolvedores

Acesse: [https://www.mercadopago.com.br/developers/panel/app](https://www.mercadopago.com.br/developers/panel/app)

### 2. Criar Nova Aplicação

1. Clique em **"Criar aplicação"**
2. Preencha os dados:
   - **Nome da aplicação:** ESP32-PIX (ou nome de sua preferência)
   - **Modelo de integração:** Selecione "Checkout Pro" ou "Checkout API"
   - **Produto a integrar:** Pagamentos online
3. Aceite os termos e clique em **"Criar aplicação"**

### 3. Obter Credenciais

Após criar a aplicação, você terá acesso às credenciais:

| Credencial              | Descrição                                  |
| ----------------------- | -------------------------------------------- |
| **Public Key**    | Chave pública para identificar a conta      |
| **Access Token**  | Token de acesso para autenticação nas APIs |
| **Client ID**     | Identificador da aplicação                 |
| **Client Secret** | Segredo da aplicação                       |

> ⚠️ **Importante:** Nunca exponha o `Access Token` ou `Client Secret` em código público!

### 4. Ambientes

O Mercado Pago oferece dois ambientes:

- **Sandbox (Testes):** Use para desenvolvimento e testes com cartões de teste
- **Produção:** Use quando o sistema estiver pronto para cobranças reais

### 5. Configurar no Backend

As credenciais devem ser configuradas no **backend** (não no ESP32):

```env
# .env do backend
MERCADO_PAGO_ACCESS_TOKEN=APP_USR-xxxxxxxxxxxxx
MERCADO_PAGO_PUBLIC_KEY=APP_USR-xxxxxxxxxxxxx
```

### 6. Cartões de Teste (Sandbox)

Para testes, use os cartões de teste do Mercado Pago:

| Número do Cartão  | CVV | Vencimento | Status   |
| ------------------- | --- | ---------- | -------- |
| 5031 4332 1540 6351 | 123 | 11/25      | Aprovado |
| 4235 6477 2802 5682 | 123 | 11/25      | Recusado |

> Consulte a [documentação oficial](https://www.mercadopago.com.br/developers/pt/docs/checkout-api/integration-test/test-cards) para mais cartões de teste.

---

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

### GET /api/status/

Response:

```json
{
    "status": "APPROVED"  // ou "PENDING", "REJECTED"
}
```

## Diferenças da versão Arduino

| Arduino/PlatformIO  | ESP-IDF 5.5.0            |
| ------------------- | ------------------------ |
| `WiFi.h`          | `esp_wifi.h`           |
| `HTTPClient.h`    | `esp_http_client.h`    |
| `ArduinoJson`     | `cJSON` (built-in)     |
| `Adafruit_ST7735` | Driver SPI nativo        |
| `ESP32Servo`      | LEDC PWM                 |
| `tone()/noTone()` | LEDC PWM                 |
| `delay()`         | `vTaskDelay()`         |
| `millis()`        | `esp_timer_get_time()` |

## Licença

Este projeto está licenciado sob a [Creative Commons Attribution 4.0 International License (CC BY 4.0)](https://creativecommons.org/licenses/by/4.0/).

Veja o arquivo [LICENSE](LICENSE) para mais detalhes.

---

## Autor / Contato

**Carlos Delfino**

- 🌐 Website: [https://carlosdelfino.eti.br](https://carlosdelfino.eti.br)
- 📧 Email: [consultoria@carlosdelfino.eti.br](mailto:consultoria@carlosdelfino.eti.br)
- 📱 WhatsApp: [(+55 85) 98520-5490](https://wa.me/5585985205490)
- 🐙 GitHub: [https://github.com/carlosdelfino](https://github.com/carlosdelfino)

---

> Baseado no projeto original de [mazinhoandrade](https://github.com/mazinhoandrade).
