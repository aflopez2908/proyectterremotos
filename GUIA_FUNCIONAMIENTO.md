# 🚀 Guía Completa de Funcionamiento del Sistema Sísmico

## 🔄 Flujo de Funcionamiento Completo

### 1. **Al Encender el Pico (Secuencia de Inicio)**

```
[INICIO] → [I2C] → [UART] → [ESP8266] → [MPU6050] → [API] → [DASHBOARD]
```

#### Paso a Paso:
1. **Pico se enciende** y ejecuta `main.cpp`
2. **Configura I2C** para comunicarse con MPU6050 (pines GP16/GP17)
3. **Configura UART** para comunicarse with ESP8266 (pines GP4/GP5)
4. **Inicializa ESP8266**:
   - Envía comandos AT
   - Se conecta a WiFi configurado en `Config.h`
   - Levanta servidor HTTP en puerto 80
5. **Inicializa MPU6050**:
   - Test de conectividad I2C
   - Calibración automática (100 muestras)
   - Configuración de rangos y filtros
6. **Inicia bucle principal**:
   - Lee sensor cada 100ms
   - Procesa servidor HTTP
   - Envía datos al API cuando detecta eventos

### 2. **Detección y Envío de Datos**

#### Cuando el sensor detecta movimiento:

```cpp
// Umbral de detección
if (magnitude >= 5.0 m/s²) {
    // Clasifica el evento
    if (magnitude >= 15.0 m/s²) {
        event_type = "earthquake";  // 🚨 TERREMOTO
    } else {
        event_type = "vibration";  // 📳 Vibración
    }
    
    // Envía inmediatamente al API
    POST /api/pico/sensor-data
}
```

#### JSON enviado al servidor:
```json
{
  "device_id": "pico_sensor_01",
  "timestamp": 1699123456,
  "acceleration_x": 0.234,
  "acceleration_y": -1.567,
  "acceleration_z": 10.234,
  "gyro_x": 0.012,
  "gyro_y": -0.045,
  "gyro_z": 0.003,
  "magnitude": 10.456,
  "event_type": "vibration",
  "is_significant": false
}
```

### 3. **Envío de Estado Periódico**

Cada 30 segundos envía estado al API:
```json
{
  "device_id": "pico_sensor_01", 
  "timestamp": 1699123456,
  "status": "online",
  "sensor_ok": true,
  "avg_magnitude": 9.81,
  "buffer_count": 45,
  "errors": 0
}
```

## 📋 **Requisitos para Funcionamiento**

### **Hardware Necesario:**
```
✅ Raspberry Pi Pico
✅ ESP8266 (con firmware AT)
✅ MPU6050 (acelerómetro/giroscopio)
✅ Buzzer activo
✅ Protoboard y cables jumper
✅ Fuente de alimentación 3.3V/5V
```

### **Software Necesario:**
```
✅ Pico SDK instalado
✅ VS Code con extensión Pico
✅ Node.js v18+ para Express/Next.js
✅ Git para control de versiones
```

## 🔧 **Configuración Paso a Paso**

### **PASO 1: Configurar Hardware**

#### Conexiones del Pico:
```
MPU6050:
VCC → 3V3 (Pin 36)
GND → GND (Pin 38)  
SDA → GP16 (Pin 21)
SCL → GP17 (Pin 22)

ESP8266:
TX → GP4 (Pin 6)
RX → GP5 (Pin 7)
VCC → VBUS (Pin 40) 
GND → GND (Pin 38)

Buzzer:
+ → GP15 (Pin 20)
- → GND (Pin 38)
```

### **PASO 2: Configurar WiFi en Config.h**

```cpp
// Cambiar por tu red WiFi
inline constexpr char WIFI_SSID[] = "TU_WIFI_AQUI";
inline constexpr char WIFI_PASS[] = "TU_PASSWORD_AQUI";

// Cambiar por la IP de tu servidor
inline constexpr char API_HOST[] = "192.168.1.50"; // IP de tu PC
inline constexpr int  API_PORT   = 3000;
```

### **PASO 3: Compilar y Flashear**

```bash
cd /home/kali/pico/serv_http_esp8266/build
make -j4

# Esto genera: serv_http_esp8266.uf2
```

**Para flashear:**
1. Mantén presionado BOOTSEL en el Pico
2. Conecta USB al PC
3. Aparece como unidad USB
4. Copia `serv_http_esp8266.uf2` a la unidad
5. El Pico se reinicia automáticamente

### **PASO 4: Configurar Servidor Express**

```bash
cd servidor_express

# Crear .env con tu configuración
echo "PICO_IP=192.168.1.100" > .env  # IP que obtendrá el Pico
echo "PICO_PORT=80" >> .env
echo "PORT=3000" >> .env

npm install
npm run dev
```

### **PASO 5: Configurar Dashboard Next.js**

```bash
cd earthquake-dashboard

# Crear .env.local
echo "NEXT_PUBLIC_API_URL=http://localhost:3000/api" > .env.local

npm install
npm run dev
```

## 🌐 **Arquitectura de Red**

```
Internet
    ↕
Router WiFi (192.168.1.1)
    ├── PC/Laptop (192.168.1.50) ← Express Server (3000) + Dashboard (3001)
    ├── Pico+ESP8266 (192.168.1.100) ← HTTP Server (80)
    └── Tu teléfono/tablet ← Para ver dashboard
```

## 📡 **Comunicación en Tiempo Real**

### **Del Pico al Servidor:**
1. **Eventos sísmicos** → Envío inmediato via HTTP POST
2. **Estado del sistema** → Cada 30 segundos
3. **Respuesta a comandos** → Control de buzzer/morse

### **Del Dashboard al Pico:**
1. **Verificación de estado** → Cada 30 segundos
2. **Comandos de control** → Buzzer y morse
3. **Visualización de datos** → Actualización cada 10 segundos

## 🔄 **Secuencia de Arranque Completa**

### **1. Arrancar el Servidor Express:**
```bash
cd servidor_express
npm run dev

# Salida esperada:
# 🚀 Servidor Express ejecutándose en puerto 3000
# 📡 Esperando conexiones del Pico en /api/pico/*
```

### **2. Arrancar el Dashboard:**
```bash
cd earthquake-dashboard  
npm run dev

# Salida esperada:
# ▲ Next.js 14.0.0
# - Local:        http://localhost:3001
# - Network:      http://192.168.1.50:3001
```

### **3. Encender el Pico:**

**Monitor serie mostrará:**
```
===== Sistema de Detección Sísmica =====
Dispositivo: pico_sensor_01
========================================
Configurando I2C...
Configurando UART para ESP8266...
Inicializando sensor MPU6050...
[MPU6050] Inicializado correctamente
Inicializando servidor ESP8266...
ESP8266 inicializado correctamente
Inicializando monitor sísmico...
[SeismicMonitor] Inicializando...
[MPU6050] Iniciando calibración con 100 muestras...
[MPU6050] Calibración completada. Offsets: X=0.123, Y=-0.045, Z=0.567 m/s²
[SeismicMonitor] Inicialización completada

===== SISTEMA LISTO =====
Servidor HTTP: puerto 80
API destino: 192.168.1.50:3000/api/pico/sensor-data
Intervalo de lectura: 100 ms
Envío de eventos: cada evento significativo  
Envío de estado: cada 30000 ms
========================
```

### **4. Verificar Conectividad:**

**En el navegador:** `http://localhost:3001`

Deberías ver:
- ✅ **Pico Online** (indicador verde)
- 📊 **Estadísticas actualizándose**
- 📳 **Eventos apareciendo en tiempo real**

## 🧪 **Pruebas del Sistema**

### **Test 1: Comunicación Básica**
```bash
# Verificar que el Pico responde
curl http://192.168.1.100/status

# Debería devolver JSON con estado del sistema
```

### **Test 2: Control Remoto** 
En el dashboard:
1. Click "Activar Buzzer" → Debería sonar
2. Escribir "SOS" y enviar → Morse code en buzzer

### **Test 3: Detección Sísmica**
1. **Golpear suavemente** la mesa → Vibración detectada
2. **Golpear fuerte** → Terremoto detectado  
3. **Ver dashboard** → Eventos aparecen en tiempo real

## 🚨 **Troubleshooting**

### **Pico aparece Offline:**
```bash
# 1. Verificar IP del Pico
ping 192.168.1.100

# 2. Verificar servidor HTTP del Pico
curl http://192.168.1.100

# 3. Verificar configuración WiFi en Config.h
```

### **No se detectan eventos:**
```bash
# 1. Monitor serie del Pico para ver lecturas del sensor
# 2. Verificar conexiones I2C (SDA/SCL)
# 3. Revisar calibración del MPU6050
```

### **API no recibe datos:**
```bash
# 1. Verificar logs del Express server
# 2. Confirmar IP y puerto en Config.h
# 3. Verificar firewall/red
```

## 📈 **Monitoreo del Sistema**

### **Logs del Pico (Monitor Serie):**
```
[SeismicMonitor] Evento detectado: vibration (magnitud: 12.34 m/s²)
[SeismicMonitor] Enviando datos al API: {"device_id":"pico_sensor_01"...}
[SeismicMonitor] Datos enviados exitosamente
```

### **Logs del Express Server:**
```
🔗 Datos del sensor recibidos: pico_sensor_01
📊 Magnitud: 12.34 m/s² | Tipo: vibration
🚨 ¡Evento sísmico detectado!
📡 Enviando datos sísmicos al sistema de análisis...
```

### **Dashboard en Tiempo Real:**
- 🟢 **Pico Online**
- 📊 **Total Eventos: 42**
- 🚨 **Terremotos: 3**  
- 📳 **Vibraciones: 39**
- 📈 **Magnitud Promedio: 8.2 m/s²**

## 🎯 **Resultados Esperados**

Una vez todo configurado:

1. **Sistema autónomo** que detecta eventos sísmicos 24/7
2. **Dashboard en tiempo real** accessible desde cualquier dispositivo en la red
3. **Control remoto** del Pico via web interface  
4. **Alertas automáticas** para eventos significativos
5. **Historial completo** de todos los eventos detectados

¡Tu sistema de detección sísmica estará funcionando completamente! 🎉
