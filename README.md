# Pico W Telemetry Firmware

Firmware embarcado para **Raspberry Pi Pico W** que lê sensores analógicos e digitais e envia telemetria via HTTP para o backend da [Atividade 1 (backend-queue-rabbitmq)](https://github.com/julia-lika/backend-queue-rabbitmq).

## Framework / Toolchain

**Arduino Framework** (Caminho 1) — via [PlatformIO](https://platformio.org/) com o core [arduino-pico (earlephilhower)](https://github.com/earlephilhower/arduino-pico).

## Sensores integrados

| Sensor | Tipo | Pino | Natureza | Range esperado |
|---|---|---|---|---|
| Botão (presença) | Digital | GP15 | `discrete` | 0 (ausente) / 1 (presente) |
| Potenciômetro (temperatura) | Analógico | GP26 (ADC0) | `analog` | 0.0 — 330.0 °C (escala LM35) |

### LED indicador

| Componente | Pino | Função |
|---|---|---|
| LED verde | GP25 (built-in) | Aceso = Wi-Fi conectado |

## Diagrama de conexão

```
                    Raspberry Pi Pico W
                   ┌──────────────────────┐
                   │                      │
  [Botão] ────────│ GP15          GP25  │──── [LED status]
           GND ───│ GND                  │
                   │                      │
  [Pot] SIG ──────│ GP26 (ADC0)          │
        VCC ──────│ 3V3(OUT)             │
        GND ──────│ GND                  │
                   └──────────────────────┘
```

O arquivo `diagram.json` contém o circuito para simulação no [Wokwi](https://wokwi.com/).

## Funcionalidades implementadas

### 1. Leitura de sensor digital (GP15)
- Botão configurado com `INPUT_PULLUP` (ativo em LOW)
- **Debouncing** por software (50 ms) para eliminar ruído de bouncing mecânico
- Envia leitura como `sensor_type: "presence"`, `nature: "discrete"`, `value: 0 ou 1`

### 2. Leitura de sensor analógico (GP26 / ADC0)
- Resolução de 12 bits (0–4095)
- Conversão para escala de temperatura (LM35: 10 mV/°C)
- **Média móvel** com janela de 10 amostras para suavização
- Envia leitura como `sensor_type: "temperature"`, `nature: "analog"`

### 3. Conectividade Wi-Fi
- Conexão automática no boot com até 20 tentativas
- **Reconexão automática** — verifica o link a cada 10 s e reconecta se necessário
- LED built-in indica status da conexão
- Sincronização de relógio via **NTP** (`pool.ntp.org`) para timestamps ISO 8601

### 4. Envio de telemetria HTTP
- POST para o endpoint do backend com payload JSON idêntico ao esperado pela Atividade 1
- **Retry** com backoff linear (até 3 tentativas, delay crescente)
- Timeout de 5 s por requisição

### Payload enviado

```json
{
  "device_id":   "pico-w-001",
  "timestamp":   "2026-03-30T14:22:05",
  "sensor_type": "temperature",
  "nature":      "analog",
  "value":       23.45
}
```

## Compilação e gravação

### Pré-requisitos
- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation.html) ou extensão VS Code
- Cabo USB conectado ao Pico W

### Configuração

Edite `include/config.h` com seus dados:

```c
#define WIFI_SSID        "MinhaRede"
#define WIFI_PASSWORD    "MinhaSenha"
#define BACKEND_URL      "http://192.168.1.100:3000/telemetry"
#define DEVICE_ID        "pico-w-001"
```

### Compilar e gravar

```bash
# Compilar
pio run

# Gravar no Pico W (segure BOOTSEL ao conectar USB)
pio run --target upload

# Monitor serial
pio device monitor
```

### Simulação com Wokwi

Se não tiver o hardware físico, use o [simulador Wokwi](https://wokwi.com/):

1. Instale a extensão **Wokwi for VS Code**
2. Compile o projeto com `pio run`
3. Abra o arquivo `diagram.json` no VS Code
4. Pressione **F1 → Wokwi: Start Simulator**

O arquivo `wokwi.toml` já aponta para o firmware compilado.

## Saída serial esperada

```
======= Pico W Telemetry Firmware =======
[wifi] Connecting to MinhaRede.......
[wifi] Connected — IP 192.168.1.42
[ntp] Time sync requested
[sensor] Temperature (smoothed): 24.31 °C
[http] Sending: {"device_id":"pico-w-001","timestamp":"2026-03-30T14:22:05","sensor_type":"temperature","nature":"analog","value":24.31}
[http] 202 Accepted — {"id":"a1b2c3d4-...","status":"queued","message":"Reading received - processing asynchronously"}
[sensor] Presence: none
[http] Sending: {"device_id":"pico-w-001","timestamp":"2026-03-30T14:22:07","sensor_type":"presence","nature":"discrete","value":0}
[http] 202 Accepted — {"id":"e5f6g7h8-...","status":"queued","message":"Reading received - processing asynchronously"}
[sensor] Temperature (smoothed): 24.28 °C
[http] Sending: ...
```

## Estrutura do projeto

```
pico-w-telemetry/
├── platformio.ini          # Configuração PlatformIO (board, libs)
├── include/
│   └── config.h            # Wi-Fi, backend URL, pinos, intervalos
├── src/
│   └── main.cpp            # Firmware principal
├── diagram.json            # Circuito Wokwi
├── wokwi.toml              # Config simulador Wokwi
└── README.md
```

## Referência

- **Backend (Atividade 1):** [backend-queue-rabbitmq](https://github.com/julia-lika/backend-queue-rabbitmq) — Clojure + RabbitMQ + Datomic + Pedestal
