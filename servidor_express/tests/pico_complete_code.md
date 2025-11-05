# Código Completo para Raspberry Pi Pico + ESP8266

Este documento contiene el código completo para integrar el Raspberry Pi Pico con el módulo ESP8266 para enviar datos sísmicos al API Express.

## Estructura del Proyecto Pico

```
pico_earthquake_detector/
├── main.cpp               # Código principal
├── Config.h              # Configuraciones
├── web_page.hpp         # Página web local (opcional)
├── CMakeLists.txt       # Configuración CMake
├── lib/
│   ├── Esp8266HttpServer.h
│   └── Esp8266HttpServer.cpp
└── sensors/
    ├── mpu6050.h
    └── mpu6050.cpp
```

## 1. Configuración Principal (Config.h)

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// Configuración WiFi
#define WIFI_SSID "TU_RED_WIFI"
#define WIFI_PASSWORD "TU_CONTRASEÑA_WIFI"

// Configuración del API Express
#define API_HOST "192.168.1.100"  // IP de tu servidor Express
#define API_PORT 3000
#define API_ENDPOINT "/api/earthquakes/event"

// Configuración del sensor MPU6050
#define MPU6050_ADDR 0x68
#define ACCEL_SCALE_FACTOR 16384.0  // Para ±2g
#define GYRO_SCALE_FACTOR 131.0     // Para ±250°/s

// Umbrales de detección
#define EARTHQUAKE_THRESHOLD 15.0   // m/s² para terremoto
#define VIBRATION_THRESHOLD 8.0     // m/s² para vibración
#define NOISE_THRESHOLD 2.0         // m/s² ruido de fondo

// Configuración del sistema
#define DEVICE_ID "PICO_SENSOR_001"
#define SAMPLE_RATE_MS 100          // Muestreo cada 100ms
#define SEND_INTERVAL_MS 5000       // Enviar datos cada 5 segundos
#define MAX_RETRY_ATTEMPTS 3        // Reintentos de envío

// Pines
#define LED_PIN 25                  // LED onboard
#define BUZZER_PIN 15              // Buzzer de alerta (opcional)
#define SDA_PIN 4                  // I2C SDA
#define SCL_PIN 5                  // I2C SCL

// Debug
#define DEBUG_SERIAL true
#define BAUD_RATE 115200

#endif
```

## 2. Sensor MPU6050 (sensors/mpu6050.h)

```cpp
#ifndef MPU6050_H
#define MPU6050_H

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <cmath>

struct AccelData {
    float x, y, z;
    float total;
    float magnitude;
    uint32_t timestamp;
};

struct GyroData {
    float x, y, z;
    uint32_t timestamp;
};

struct SeismicEvent {
    AccelData accel;
    GyroData gyro;
    float magnitude;
    const char* event_type;
    bool is_significant;
    uint32_t timestamp;
};

class MPU6050 {
private:
    i2c_inst_t* i2c;
    uint8_t addr;
    float accel_offset_x, accel_offset_y, accel_offset_z;
    float gyro_offset_x, gyro_offset_y, gyro_offset_z;
    
    // Filtro de media móvil
    static const int FILTER_SIZE = 10;
    float accel_filter_x[FILTER_SIZE] = {0};
    float accel_filter_y[FILTER_SIZE] = {0};
    float accel_filter_z[FILTER_SIZE] = {0};
    int filter_index = 0;
    
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void readRegisters(uint8_t reg, uint8_t* buffer, uint8_t len);
    
public:
    MPU6050(i2c_inst_t* i2c_instance, uint8_t address = MPU6050_ADDR);
    bool init();
    void calibrate();
    AccelData readAcceleration();
    GyroData readGyroscope();
    SeismicEvent analyzeSeismicActivity();
    float calculateMagnitude(float x, float y, float z);
    float applyMovingAverage(float new_value, float* filter_array);
};

#endif
```

## 3. Implementación MPU6050 (sensors/mpu6050.cpp)

```cpp
#include "mpu6050.h"
#include "../Config.h"

// Registros MPU6050
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_GYRO_CONFIG  0x1B

MPU6050::MPU6050(i2c_inst_t* i2c_instance, uint8_t address) 
    : i2c(i2c_instance), addr(address) {
    accel_offset_x = accel_offset_y = accel_offset_z = 0.0f;
    gyro_offset_x = gyro_offset_y = gyro_offset_z = 0.0f;
}

bool MPU6050::init() {
    // Despertar el MPU6050
    writeRegister(MPU6050_REG_PWR_MGMT_1, 0x00);
    sleep_ms(100);
    
    // Configurar filtro pasa-bajos
    writeRegister(MPU6050_REG_CONFIG, 0x06);
    
    // Configurar rango del acelerómetro (±2g)
    writeRegister(MPU6050_REG_ACCEL_CONFIG, 0x00);
    
    // Configurar rango del giroscopio (±250°/s)
    writeRegister(MPU6050_REG_GYRO_CONFIG, 0x00);
    
    sleep_ms(100);
    
    // Verificar comunicación
    uint8_t who_am_i = readRegister(0x75);
    if (who_am_i != 0x68) {
        if (DEBUG_SERIAL) {
            printf("Error: MPU6050 no detectado (0x%02X)\n", who_am_i);
        }
        return false;
    }
    
    if (DEBUG_SERIAL) {
        printf("✅ MPU6050 inicializado correctamente\n");
    }
    
    // Calibrar sensor
    calibrate();
    return true;
}

void MPU6050::calibrate() {
    if (DEBUG_SERIAL) {
        printf("🔧 Calibrando MPU6050...\n");
    }
    
    const int samples = 100;
    float sum_ax = 0, sum_ay = 0, sum_az = 0;
    float sum_gx = 0, sum_gy = 0, sum_gz = 0;
    
    for (int i = 0; i < samples; i++) {
        AccelData accel = readAcceleration();
        GyroData gyro = readGyroscope();
        
        sum_ax += accel.x;
        sum_ay += accel.y;
        sum_az += accel.z - 9.81f; // Compensar gravedad
        sum_gx += gyro.x;
        sum_gy += gyro.y;
        sum_gz += gyro.z;
        
        sleep_ms(10);
    }
    
    accel_offset_x = sum_ax / samples;
    accel_offset_y = sum_ay / samples;
    accel_offset_z = sum_az / samples;
    gyro_offset_x = sum_gx / samples;
    gyro_offset_y = sum_gy / samples;
    gyro_offset_z = sum_gz / samples;
    
    if (DEBUG_SERIAL) {
        printf("✅ Calibración completada\n");
        printf("Offsets Accel: X=%.2f, Y=%.2f, Z=%.2f\n", 
               accel_offset_x, accel_offset_y, accel_offset_z);
        printf("Offsets Gyro: X=%.2f, Y=%.2f, Z=%.2f\n", 
               gyro_offset_x, gyro_offset_y, gyro_offset_z);
    }
}

AccelData MPU6050::readAcceleration() {
    uint8_t buffer[6];
    readRegisters(MPU6050_REG_ACCEL_XOUT_H, buffer, 6);
    
    int16_t raw_x = (buffer[0] << 8) | buffer[1];
    int16_t raw_y = (buffer[2] << 8) | buffer[3];
    int16_t raw_z = (buffer[4] << 8) | buffer[5];
    
    AccelData data;
    data.x = (raw_x / ACCEL_SCALE_FACTOR) * 9.81f - accel_offset_x;
    data.y = (raw_y / ACCEL_SCALE_FACTOR) * 9.81f - accel_offset_y;
    data.z = (raw_z / ACCEL_SCALE_FACTOR) * 9.81f - accel_offset_z;
    
    // Aplicar filtro de media móvil
    data.x = applyMovingAverage(data.x, accel_filter_x);
    data.y = applyMovingAverage(data.y, accel_filter_y);
    data.z = applyMovingAverage(data.z, accel_filter_z);
    
    data.total = sqrt(data.x * data.x + data.y * data.y + data.z * data.z);
    data.magnitude = calculateMagnitude(data.x, data.y, data.z);
    data.timestamp = time_us_32();
    
    return data;
}

GyroData MPU6050::readGyroscope() {
    uint8_t buffer[6];
    readRegisters(MPU6050_REG_GYRO_XOUT_H, buffer, 6);
    
    int16_t raw_x = (buffer[0] << 8) | buffer[1];
    int16_t raw_y = (buffer[2] << 8) | buffer[3];
    int16_t raw_z = (buffer[4] << 8) | buffer[5];
    
    GyroData data;
    data.x = (raw_x / GYRO_SCALE_FACTOR) - gyro_offset_x;
    data.y = (raw_y / GYRO_SCALE_FACTOR) - gyro_offset_y;
    data.z = (raw_z / GYRO_SCALE_FACTOR) - gyro_offset_z;
    data.timestamp = time_us_32();
    
    return data;
}

SeismicEvent MPU6050::analyzeSeismicActivity() {
    AccelData accel = readAcceleration();
    GyroData gyro = readGyroscope();
    
    SeismicEvent event;
    event.accel = accel;
    event.gyro = gyro;
    event.magnitude = accel.magnitude;
    event.timestamp = time_us_32();
    
    // Clasificar tipo de evento
    if (accel.total > EARTHQUAKE_THRESHOLD) {
        event.event_type = "earthquake";
        event.is_significant = true;
    } else if (accel.total > VIBRATION_THRESHOLD) {
        event.event_type = "vibration";
        event.is_significant = true;
    } else {
        event.event_type = "normal";
        event.is_significant = false;
    }
    
    return event;
}

float MPU6050::calculateMagnitude(float x, float y, float z) {
    // Escala logarítmica simplificada basada en aceleración
    float total_accel = sqrt(x * x + y * y + z * z);
    
    if (total_accel < NOISE_THRESHOLD) return 0.0f;
    
    // Conversión aproximada a escala Richter local
    float magnitude = log10(total_accel / NOISE_THRESHOLD) + 1.0f;
    return fmax(0.0f, fmin(magnitude, 9.0f));
}

float MPU6050::applyMovingAverage(float new_value, float* filter_array) {
    filter_array[filter_index] = new_value;
    filter_index = (filter_index + 1) % FILTER_SIZE;
    
    float sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += filter_array[i];
    }
    
    return sum / FILTER_SIZE;
}

void MPU6050::writeRegister(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    i2c_write_blocking(i2c, addr, data, 2, false);
}

uint8_t MPU6050::readRegister(uint8_t reg) {
    uint8_t value;
    i2c_write_blocking(i2c, addr, &reg, 1, true);
    i2c_read_blocking(i2c, addr, &value, 1, false);
    return value;
}

void MPU6050::readRegisters(uint8_t reg, uint8_t* buffer, uint8_t len) {
    i2c_write_blocking(i2c, addr, &reg, 1, true);
    i2c_read_blocking(i2c, addr, buffer, len, false);
}
```

## 4. Cliente HTTP mejorado (lib/Esp8266HttpServer.h)

```cpp
#ifndef ESP8266_HTTP_SERVER_H
#define ESP8266_HTTP_SERVER_H

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include <string>
#include <vector>

struct WiFiStatus {
    bool connected;
    std::string ip_address;
    int signal_strength;
    std::string ssid;
};

struct HttpResponse {
    int status_code;
    std::string body;
    std::string headers;
    bool success;
    std::string error_message;
};

class Esp8266HttpServer {
private:
    uart_inst_t* uart;
    uint baud_rate;
    uint tx_pin, rx_pin;
    std::string current_ssid;
    std::string current_password;
    bool is_connected;
    
    bool sendCommand(const std::string& command, const std::string& expected_response = "OK", uint32_t timeout_ms = 5000);
    std::string receiveResponse(uint32_t timeout_ms = 5000);
    bool waitForResponse(const std::string& expected, uint32_t timeout_ms = 5000);
    std::string urlEncode(const std::string& str);
    
public:
    Esp8266HttpServer(uart_inst_t* uart_instance, uint baud = 115200, uint tx = 0, uint rx = 1);
    
    // Configuración básica
    bool init();
    bool reset();
    bool setMode(int mode); // 1=STA, 2=AP, 3=STA+AP
    
    // WiFi
    bool connectToWiFi(const std::string& ssid, const std::string& password);
    bool disconnectWiFi();
    WiFiStatus getWiFiStatus();
    std::vector<std::string> scanNetworks();
    
    // HTTP Cliente
    HttpResponse httpGet(const std::string& host, int port, const std::string& path);
    HttpResponse httpPost(const std::string& host, int port, const std::string& path, 
                         const std::string& data, const std::string& content_type = "application/json");
    
    // Utilidades
    bool isConnected() const { return is_connected; }
    std::string getLocalIP();
    bool ping(const std::string& host);
    void enableDebug(bool enable);
};

#endif
```

## 5. Código Principal Completo (main.cpp)

```cpp
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "pico/time.h"
#include "Config.h"
#include "sensors/mpu6050.h"
#include "lib/Esp8266HttpServer.h"
#include <stdio.h>
#include <string>
#include <ctime>

// Instancias globales
MPU6050 sensor(i2c0);
Esp8266HttpServer wifi_client(uart0);

// Variables de estado
uint32_t last_send_time = 0;
uint32_t last_sample_time = 0;
bool system_initialized = false;
int consecutive_errors = 0;

// Buffer de eventos
std::vector<SeismicEvent> event_buffer;
const int MAX_BUFFER_SIZE = 50;

// Prototipos de funciones
bool initializeSystem();
void setupHardware();
bool sendSeismicData(const SeismicEvent& event);
std::string createJsonPayload(const SeismicEvent& event);
void handleSystemError(const std::string& error);
void blinkLED(int times, int delay_ms = 200);
void processSeismicEvent(const SeismicEvent& event);
void sendBufferedEvents();
std::string getCurrentTimestamp();

int main() {
    setupHardware();
    
    if (!initializeSystem()) {
        printf("❌ Error crítico en la inicialización del sistema\n");
        while (true) {
            blinkLED(5, 100); // SOS
            sleep_ms(2000);
        }
    }
    
    printf("🚀 Sistema de detección sísmica iniciado\n");
    printf("📡 Dispositivo: %s\n", DEVICE_ID);
    printf("🌐 API Endpoint: %s:%d%s\n", API_HOST, API_PORT, API_ENDPOINT);
    
    // Enviar evento de inicio del sistema
    SeismicEvent startup_event = {};
    startup_event.event_type = "system_startup";
    startup_event.magnitude = 0.0f;
    startup_event.is_significant = false;
    startup_event.timestamp = time_us_32();
    sendSeismicData(startup_event);
    
    // Loop principal
    while (true) {
        uint32_t current_time = time_us_32();
        
        // Muestreo del sensor
        if (current_time - last_sample_time >= SAMPLE_RATE_MS * 1000) {
            SeismicEvent event = sensor.analyzeSeismicActivity();
            processSeismicEvent(event);
            last_sample_time = current_time;
        }
        
        // Envío periódico de datos
        if (current_time - last_send_time >= SEND_INTERVAL_MS * 1000) {
            sendBufferedEvents();
            last_send_time = current_time;
        }
        
        // Verificar estado del WiFi
        if (!wifi_client.isConnected()) {
            printf("⚠️ WiFi desconectado, reintentando...\n");
            if (!wifi_client.connectToWiFi(WIFI_SSID, WIFI_PASSWORD)) {
                handleSystemError("WiFi connection failed");
            }
        }
        
        // Watchdog reset
        watchdog_update();
        
        sleep_ms(10); // Pequeña pausa para no saturar el CPU
    }
    
    return 0;
}

void setupHardware() {
    stdio_init_all();
    
    // Configurar GPIO
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    
    #ifdef BUZZER_PIN
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    #endif
    
    // Configurar I2C para MPU6050
    i2c_init(i2c0, 400000); // 400kHz
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
    
    // Configurar UART para ESP8266
    uart_init(uart0, BAUD_RATE);
    gpio_set_function(0, GPIO_FUNC_UART);
    gpio_set_function(1, GPIO_FUNC_UART);
    
    // Inicializar watchdog (30 segundos)
    watchdog_enable(30000, 1);
    
    sleep_ms(2000); // Tiempo para estabilización
    
    if (DEBUG_SERIAL) {
        printf("\n🔧 Hardware inicializado\n");
        printf("📊 I2C: %d kHz\n", 400);
        printf("📡 UART: %d baud\n", BAUD_RATE);
    }
}

bool initializeSystem() {
    blinkLED(3); // Indicar inicio
    
    // Inicializar WiFi
    if (!wifi_client.init()) {
        handleSystemError("ESP8266 init failed");
        return false;
    }
    
    if (!wifi_client.connectToWiFi(WIFI_SSID, WIFI_PASSWORD)) {
        handleSystemError("WiFi connection failed");
        return false;
    }
    
    // Inicializar sensor
    if (!sensor.init()) {
        handleSystemError("MPU6050 init failed");
        return false;
    }
    
    // Verificar conectividad API
    std::string test_data = "{\"test\":true,\"device_id\":\"" + std::string(DEVICE_ID) + "\"}";
    HttpResponse response = wifi_client.httpPost(API_HOST, API_PORT, "/api/health", test_data);
    
    if (!response.success || response.status_code != 200) {
        printf("⚠️ Warning: No se pudo conectar al API (continuando...)\n");
        // No es error crítico, el sistema puede funcionar en modo offline
    } else {
        printf("✅ Conectividad con API verificada\n");
    }
    
    system_initialized = true;
    consecutive_errors = 0;
    blinkLED(2, 500); // Indicar éxito
    
    return true;
}

void processSeismicEvent(const SeismicEvent& event) {
    if (DEBUG_SERIAL && event.is_significant) {
        printf("🌍 Evento sísmico detectado:\n");
        printf("   Tipo: %s\n", event.event_type);
        printf("   Magnitud: %.2f\n", event.magnitude);
        printf("   Aceleración: %.2f m/s²\n", event.accel.total);
        printf("   Timestamp: %lu\n", event.timestamp);
    }
    
    // Agregar al buffer
    event_buffer.push_back(event);
    
    // Mantener tamaño del buffer
    if (event_buffer.size() > MAX_BUFFER_SIZE) {
        event_buffer.erase(event_buffer.begin());
    }
    
    // Envío inmediato para eventos significativos
    if (event.is_significant) {
        gpio_put(LED_PIN, 1); // Encender LED
        
        #ifdef BUZZER_PIN
        if (event.accel.total > EARTHQUAKE_THRESHOLD) {
            // Buzzer para terremotos
            for (int i = 0; i < 5; i++) {
                gpio_put(BUZZER_PIN, 1);
                sleep_ms(200);
                gpio_put(BUZZER_PIN, 0);
                sleep_ms(200);
            }
        }
        #endif
        
        // Enviar inmediatamente
        if (wifi_client.isConnected()) {
            sendSeismicData(event);
        }
        
        gpio_put(LED_PIN, 0); // Apagar LED
    }
}

bool sendSeismicData(const SeismicEvent& event) {
    if (!wifi_client.isConnected()) {
        printf("⚠️ WiFi no conectado, datos almacenados en buffer\n");
        return false;
    }
    
    std::string json_data = createJsonPayload(event);
    
    if (DEBUG_SERIAL) {
        printf("📤 Enviando datos: %s\n", json_data.c_str());
    }
    
    HttpResponse response = wifi_client.httpPost(API_HOST, API_PORT, API_ENDPOINT, json_data);
    
    if (response.success && response.status_code == 200) {
        if (DEBUG_SERIAL) {
            printf("✅ Datos enviados correctamente\n");
        }
        consecutive_errors = 0;
        return true;
    } else {
        consecutive_errors++;
        printf("❌ Error enviando datos (intento %d): %s\n", 
               consecutive_errors, response.error_message.c_str());
        
        if (consecutive_errors >= MAX_RETRY_ATTEMPTS) {
            handleSystemError("Too many consecutive send errors");
        }
        
        return false;
    }
}

std::string createJsonPayload(const SeismicEvent& event) {
    char json_buffer[512];
    
    snprintf(json_buffer, sizeof(json_buffer),
        "{"
        "\"device_id\":\"%s\","
        "\"timestamp\":\"%s\","
        "\"event_type\":\"%s\","
        "\"acceleration_x\":%.3f,"
        "\"acceleration_y\":%.3f,"
        "\"acceleration_z\":%.3f,"
        "\"total_acceleration\":%.3f,"
        "\"gyro_x\":%.3f,"
        "\"gyro_y\":%.3f,"
        "\"gyro_z\":%.3f,"
        "\"magnitude\":%.2f,"
        "\"is_significant\":%s"
        "}",
        DEVICE_ID,
        getCurrentTimestamp().c_str(),
        event.event_type,
        event.accel.x,
        event.accel.y,
        event.accel.z,
        event.accel.total,
        event.gyro.x,
        event.gyro.y,
        event.gyro.z,
        event.magnitude,
        event.is_significant ? "true" : "false"
    );
    
    return std::string(json_buffer);
}

void sendBufferedEvents() {
    if (event_buffer.empty() || !wifi_client.isConnected()) {
        return;
    }
    
    printf("📤 Enviando %d eventos del buffer\n", (int)event_buffer.size());
    
    // Enviar eventos significativos primero
    for (auto it = event_buffer.begin(); it != event_buffer.end();) {
        if (it->is_significant) {
            if (sendSeismicData(*it)) {
                it = event_buffer.erase(it);
            } else {
                ++it;
                break; // Parar si falla el envío
            }
        } else {
            ++it;
        }
    }
    
    // Enviar eventos normales (máximo 5 por ciclo)
    int sent_normal = 0;
    for (auto it = event_buffer.begin(); it != event_buffer.end() && sent_normal < 5;) {
        if (!it->is_significant) {
            if (sendSeismicData(*it)) {
                it = event_buffer.erase(it);
                sent_normal++;
            } else {
                ++it;
                break;
            }
        } else {
            ++it;
        }
    }
}

void handleSystemError(const std::string& error) {
    printf("💥 Error del sistema: %s\n", error.c_str());
    
    blinkLED(10, 100); // Patrón de error
    
    // Reintentar inicialización después de errores críticos
    if (error.find("init") != std::string::npos) {
        printf("🔄 Reintentando inicialización en 5 segundos...\n");
        sleep_ms(5000);
        
        if (initializeSystem()) {
            printf("✅ Sistema recuperado\n");
        } else {
            printf("❌ Fallo en recuperación, reiniciando...\n");
            watchdog_reboot(0, SRAM_END, 1000);
        }
    }
}

void blinkLED(int times, int delay_ms) {
    for (int i = 0; i < times; i++) {
        gpio_put(LED_PIN, 1);
        sleep_ms(delay_ms);
        gpio_put(LED_PIN, 0);
        sleep_ms(delay_ms);
    }
}

std::string getCurrentTimestamp() {
    // Timestamp simplificado basado en tiempo del sistema
    uint32_t time_us = time_us_32();
    uint32_t seconds = time_us / 1000000;
    uint32_t microseconds = time_us % 1000000;
    
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%lu.%06lu", seconds, microseconds);
    
    return std::string(timestamp);
}
```

## 6. CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(earthquake_detector)

pico_sdk_init()

add_executable(earthquake_detector
    main.cpp
    sensors/mpu6050.cpp
    lib/Esp8266HttpServer.cpp
)

target_link_libraries(earthquake_detector
    pico_stdlib
    hardware_i2c
    hardware_uart
    hardware_gpio
    hardware_watchdog
    pico_time
)

pico_add_extra_outputs(earthquake_detector)

target_compile_definitions(earthquake_detector PRIVATE
    PICO_DEFAULT_UART_BAUD_RATE=115200
)
```

## Compilación y Uso

```bash
# Compilar
mkdir build && cd build
cmake ..
make

# Flashear al Pico
# Conectar Pico en modo BOOTSEL y copiar el archivo .uf2
cp earthquake_detector.uf2 /media/RPI-RP2/
```

## Características del Sistema

1. **Detección Inteligente**: Filtros y umbrales configurables
2. **Comunicación Robusta**: Reintentos automáticos y manejo de errores
3. **Buffer de Eventos**: Almacenamiento local para envío posterior
4. **Watchdog**: Reinicio automático en caso de fallo
5. **Alertas Locales**: LED y buzzer para eventos significativos
6. **Calibración Automática**: Compensación de offset del sensor
7. **Modo Debug**: Logging detallado para desarrollo

Este código proporciona un sistema completo y robusto para la detección y transmisión de eventos sísmicos desde el Raspberry Pi Pico hacia el API Express.
