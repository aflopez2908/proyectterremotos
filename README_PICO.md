# Sistema de Detección Sísmica con Raspberry Pi Pico

Este proyecto implementa un sistema de monitoreo sísmico usando:
- **Raspberry Pi Pico**: Microcontrolador principal
- **ESP8266**: Módulo WiFi para conectividad
- **MPU6050**: Sensor acelerómetro/giroscopio
- **Express.js**: Servidor API para recibir datos
- **Next.js**: Dashboard web para visualización

## 🏗️ Arquitectura del Sistema

```
[MPU6050] ←→ [Raspberry Pi Pico] ←→ [ESP8266] ←→ [WiFi] ←→ [Express API] ←→ [Next.js Dashboard]
```

### Flujo de Datos

1. **MPU6050** lee aceleración y velocidad angular cada 100ms
2. **Pico** procesa los datos y detecta eventos sísmicos
3. **ESP8266** envía datos al API vía HTTP POST cuando detecta:
   - Vibraciones (magnitud > 5.0 m/s²)
   - Terremotos (magnitud > 15.0 m/s²)
4. **Express API** recibe, procesa y almacena los datos
5. **Next.js Dashboard** muestra datos en tiempo real

## 📦 Componentes del Proyecto

### `/` - Código para Raspberry Pi Pico
- `main.cpp`: Programa principal
- `Config.h`: Configuración del sistema
- `lib/MPU6050.*`: Driver del sensor
- `lib/SeismicMonitor.*`: Monitor sísmico
- `lib/Esp8266HttpServer.*`: Servidor HTTP y cliente

### `/servidor_express/` - API Backend
- Express.js server en puerto 3000
- Endpoints para recibir datos del Pico
- Procesamiento de eventos sísmicos
- Integración con WhatsApp (opcional)

### `/earthquake-dashboard/` - Frontend Web
- Dashboard Next.js en puerto 3001
- Visualización en tiempo real
- Control remoto del Pico (buzzer, Morse)
- Estadísticas de eventos

## ⚙️ Configuración de Hardware

### Conexiones del Pico

```
Raspberry Pi Pico:
├── MPU6050 (I2C)
│   ├── VCC → 3V3 (Pin 36)
│   ├── GND → GND (Pin 38)
│   ├── SDA → GP16 (Pin 21)
│   └── SCL → GP17 (Pin 22)
├── ESP8266 (UART)
│   ├── TX → GP4 (UART1 TX)
│   └── RX → GP5 (UART1 RX)
└── Buzzer
    ├── + → GP15 (Pin 20)
    └── - → GND
```

### Configuración del ESP8266

El ESP8266 debe tener firmware AT y estar configurado con:
- Baudrate: 115200
- Conexión WiFi configurada
- Modo cliente TCP

## 🔧 Instalación y Configuración

### 1. Configurar Pico SDK

```bash
# Instalar dependencias
sudo apt update
sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi build-essential

# Descargar Pico SDK
cd ~/
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init

# Configurar variable de entorno
echo 'export PICO_SDK_PATH=~/pico-sdk' >> ~/.bashrc
source ~/.bashrc
```

### 2. Compilar código del Pico

```bash
cd /ruta/a/serv_http_esp8266
mkdir -p build
cd build
cmake ..
make
```

Esto generará `serv_http_esp8266.uf2` para flashear al Pico.

### 3. Configurar API Express

```bash
cd servidor_express
npm install
```

Editar `.env`:
```bash
PICO_IP=192.168.1.100    # IP del Pico en tu red
PICO_PORT=80
PORT=3000
WHATSAPP_ENABLED=false
```

Iniciar servidor:
```bash
npm run dev
```

### 4. Configurar Dashboard Next.js

```bash
cd earthquake-dashboard
npm install
```

Editar `.env.local`:
```bash
NEXT_PUBLIC_API_URL=http://localhost:3000/api
```

Iniciar dashboard:
```bash
npm run dev
```

## 📊 Configuración de Umbrales

En `Config.h`, puedes ajustar:

```cpp
// Umbrales de detección (m/s²)
inline constexpr float EARTHQUAKE_THRESHOLD = 15.0f;  // Terremoto
inline constexpr float VIBRATION_THRESHOLD  = 5.0f;   // Vibración

// Intervalos de tiempo (ms)
inline constexpr int SENSOR_READ_INTERVAL = 100;     // Lectura sensor
inline constexpr int API_SEND_INTERVAL   = 5000;     // Envío eventos
inline constexpr int STATUS_SEND_INTERVAL = 30000;   // Envío estado

// Configuración WiFi
inline constexpr char WIFI_SSID[] = "TuWiFi";
inline constexpr char WIFI_PASS[] = "TuPassword";

// Configuración API
inline constexpr char API_HOST[] = "192.168.1.50";   // IP de tu servidor
inline constexpr int  API_PORT   = 3000;
```

## 🌐 Endpoints del API

### Datos del Sensor
```http
POST /api/pico/sensor-data
Content-Type: application/json

{
  "device_id": "pico_sensor_01",
  "timestamp": 1234567890,
  "acceleration_x": 0.123,
  "acceleration_y": -0.456,
  "acceleration_z": 9.789,
  "gyro_x": 0.001,
  "gyro_y": 0.002,
  "gyro_z": -0.001,
  "magnitude": 9.834,
  "event_type": "vibration",
  "is_significant": false
}
```

### Estado del Dispositivo
```http
POST /api/pico/status
Content-Type: application/json

{
  "device_id": "pico_sensor_01",
  "timestamp": 1234567890,
  "status": "online",
  "sensor_ok": true,
  "avg_magnitude": 9.81,
  "buffer_count": 45,
  "errors": 0
}
```

### Control del Pico
```http
POST /api/pico/buzzer
POST /api/pico/morse
{
  "text": "SOS"
}
```

## 📱 Uso del Dashboard

1. Accede a `http://localhost:3001`
2. Verifica que el Pico aparezca como "Online"
3. Observa los eventos en tiempo real
4. Usa los controles para:
   - Activar buzzer remotamente
   - Enviar mensajes en código Morse
5. Revisa las estadísticas de eventos

## 🔧 Troubleshooting

### Pico aparece como Offline
- Verifica conexiones de hardware
- Confirma que ESP8266 esté conectado a WiFi
- Revisa IP en configuración del API

### No se detectan eventos
- Verifica conexión I2C con MPU6050
- Ajusta umbrales de detección
- Revisa calibración del sensor

### Error en compilación
- Instala Pico SDK correctamente
- Verifica variable `PICO_SDK_PATH`
- Instala herramientas ARM GCC

### API no recibe datos
- Confirma configuración de red
- Verifica endpoint y puerto
- Revisa logs del servidor Express

## 📈 Características Avanzadas

- **Calibración automática**: El sensor se calibra al inicio
- **Buffer circular**: Mantiene historial de 50 mediciones
- **Detección inteligente**: Distingue entre vibraciones y terremotos
- **Recuperación de errores**: Reinicio automático en caso de fallos
- **Monitoreo en tiempo real**: Dashboard actualizado cada 10 segundos
- **Alertas WhatsApp**: Notificaciones para eventos significativos (opcional)

## 🤝 Contribuciones

Este es un proyecto de demostración. Para mejoras:

1. Filtrado avanzado de señales
2. Algoritmos de detección mejorados
3. Interfaz móvil
4. Base de datos persistente
5. Análisis de frecuencias
6. Geolocalización de eventos

## 📄 Licencia

Proyecto educativo de código abierto. Libre para uso y modificación.
