# Ejemplo de código para el Raspberry Pi Pico
# Este código muestra cómo enviar datos al API Express

```cpp
// Agregar a Esp8266HttpServer.h (después de las funciones existentes)
private:
    // Nuevos métodos para envío de datos sísmicos
    bool sendSeismicData(float magnitude, float ax, float ay, float az, float total_accel);
    bool sendHttpPost(const char* host, int port, const char* endpoint, const char* json_data);
    float calculateMagnitude(float ax, float ay, float az);
    bool detectEarthquake(float total_acceleration);
    
    // Variables para el sensor MPU6050 (simulado)
    unsigned long last_sensor_read = 0;
    unsigned long last_api_send = 0;
```

```cpp
// Agregar a Esp8266HttpServer.cpp (después de las funciones existentes)

#include <cmath>

bool Esp8266HttpServer::sendSeismicData(float magnitude, float ax, float ay, float az, float total_accel) {
    // Determinar tipo de evento
    const char* event_type = (total_accel >= cfg::EARTHQUAKE_THRESHOLD) ? "earthquake" : "vibration";
    
    // Crear JSON payload
    char json_payload[512];
    snprintf(json_payload, sizeof(json_payload),
        "{"
        "\"device_id\":\"%s\","
        "\"magnitude\":%.2f,"
        "\"acceleration_x\":%.2f,"
        "\"acceleration_y\":%.2f,"
        "\"acceleration_z\":%.2f,"
        "\"total_acceleration\":%.2f,"
        "\"timestamp\":\"%s\""
        "}",
        cfg::DEVICE_ID,
        magnitude,
        ax, ay, az,
        total_accel,
        "2024-11-05T15:30:00Z" // En implementación real, usar timestamp actual
    );
    
    printf("[API] Enviando datos sísmicos: %s (%.2f m/s²)\n", event_type, total_accel);
    
    // Enviar al API Express
    bool success = sendHttpPost(cfg::API_HOST, cfg::API_PORT, cfg::API_ENDPOINT, json_payload);
    
    if (success) {
        printf("[API] ✅ Datos enviados exitosamente\n");
    } else {
        printf("[API] ❌ Error enviando datos\n");
    }
    
    return success;
}

bool Esp8266HttpServer::sendHttpPost(const char* host, int port, const char* endpoint, const char* json_data) {
    // Cerrar conexiones existentes si las hay
    send_at("AT+CIPCLOSE");
    sleep_ms(1000);
    
    // Establecer conexión TCP con el servidor API
    char connect_cmd[128];
    snprintf(connect_cmd, sizeof(connect_cmd), "AT+CIPSTART=\"TCP\",\"%s\",%d", host, port);
    send_at(connect_cmd);
    
    if (!wait_for("OK", 5000)) {
        printf("[API] Error conectando a %s:%d\n", host, port);
        return false;
    }
    
    // Preparar petición HTTP POST
    char http_request[1024];
    int content_length = strlen(json_data);
    
    snprintf(http_request, sizeof(http_request),
        "POST %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        endpoint, host, port, content_length, json_data
    );
    
    // Enviar tamaño de datos
    char send_cmd[32];
    snprintf(send_cmd, sizeof(send_cmd), "AT+CIPSEND=%d", (int)strlen(http_request));
    send_at(send_cmd);
    
    if (!wait_for(">", 3000)) {
        printf("[API] Error preparando envío\n");
        return false;
    }
    
    // Enviar datos HTTP
    uart_send_raw(http_request);
    
    // Esperar respuesta
    if (wait_for("200 OK", 10000)) {
        printf("[API] Respuesta 200 OK recibida\n");
        return true;
    } else {
        printf("[API] No se recibió respuesta válida\n");
        return false;
    }
}

float Esp8266HttpServer::calculateMagnitude(float ax, float ay, float az) {
    // Calcular magnitud vectorial
    return sqrt(ax*ax + ay*ay + az*az);
}

bool Esp8266HttpServer::detectEarthquake(float total_acceleration) {
    return total_acceleration >= cfg::EARTHQUAKE_THRESHOLD;
}

// Modificar el método loop() para incluir monitoreo sísmico
[[noreturn]] void Esp8266HttpServer::loop() {
    printf("[SERVIDOR] Iniciando bucle principal...\n");
    
    // Variables para el monitoreo sísmico
    unsigned long current_time = to_ms_since_boot(get_absolute_time());
    
    while (true) {
        // Verificar si es tiempo de leer el sensor
        current_time = to_ms_since_boot(get_absolute_time());
        
        if (current_time - last_sensor_read >= cfg::SENSOR_READ_INTERVAL) {
            // Simular lectura del sensor MPU6050
            // En implementación real, aquí iría la lectura del sensor I2C
            
            // Datos simulados (reemplazar con lectura real del MPU6050)
            float ax = (rand() % 100 - 50) / 10.0f;  // -5.0 a 5.0
            float ay = (rand() % 100 - 50) / 10.0f;
            float az = (rand() % 100 - 50) / 10.0f;
            
            // Ocasionalmente simular un evento sísmico
            if (rand() % 1000 == 0) {  // 0.1% probabilidad
                ax = (rand() % 200 - 100) / 10.0f;  // Valores más altos
                ay = (rand() % 200 - 100) / 10.0f;
                az = (rand() % 300 + 100) / 10.0f;  // 10-40 m/s²
            }
            
            float total_accel = calculateMagnitude(ax, ay, az);
            float magnitude = total_accel * 0.4f + 2.0f;  // Conversión aproximada
            
            // Solo enviar si supera el umbral de vibración o es tiempo de envío
            bool should_send = (total_accel >= cfg::VIBRATION_THRESHOLD) ||
                              (current_time - last_api_send >= cfg::API_SEND_INTERVAL);
            
            if (should_send) {
                sendSeismicData(magnitude, ax, ay, az, total_accel);
                last_api_send = current_time;
            }
            
            last_sensor_read = current_time;
        }
        
        // Manejar peticiones HTTP entrantes (código existente)
        int client_id, data_len;
        int result = wait_ipd_or_ready(&client_id, &data_len, 100); // Timeout corto
        
        if (result == 1) {
            // Procesar petición HTTP como antes
            printf("[SERVIDOR] +IPD recibido de cliente %d, %d bytes\n", client_id, data_len);
            
            if (data_len > 0 && data_len < cfg::REQ_BUFFER_SIZE) {
                int bytes_read = read_bytes(reqbuf_, data_len, 2000);
                if (bytes_read > 0) {
                    reqbuf_[bytes_read] = '\0';
                    printf("[PETICIÓN]:\n%s\n", reqbuf_);
                    
                    // Procesar diferentes rutas
                    if (strstr(reqbuf_, "GET / ") || strstr(reqbuf_, "GET /index")) {
                        send_http_200(client_id);
                    } else if (strstr(reqbuf_, "GET /buzzer")) {
                        // Activar buzzer
                        gpio_put(cfg::BUZZER_PIN, cfg::BUZZER_ACTIVE_LOW ? 0 : 1);
                        sleep_ms(cfg::BUZZER_PULSE_MS);
                        gpio_put(cfg::BUZZER_PIN, cfg::BUZZER_ACTIVE_LOW ? 1 : 0);
                        send_http_200(client_id);
                    } else if (strstr(reqbuf_, "GET /morse")) {
                        // Procesar Morse code
                        char* query = strstr(reqbuf_, "text=");
                        if (query) {
                            query += 5;
                            char* end = strstr(query, " HTTP");
                            if (end) *end = '\0';
                            
                            char text[cfg::MORSE_MAX_LEN + 1];
                            strncpy(text, query, cfg::MORSE_MAX_LEN);
                            text[cfg::MORSE_MAX_LEN] = '\0';
                            url_decode_inplace(text);
                            
                            // Convertir a Morse y activar buzzer
                            for (char* c = text; *c && (c - text) < cfg::MORSE_MAX_LEN; ++c) {
                                const char* morse = morse_for(*c);
                                if (morse) {
                                    for (const char* m = morse; *m; ++m) {
                                        gpio_put(cfg::BUZZER_PIN, cfg::BUZZER_ACTIVE_LOW ? 0 : 1);
                                        int duration = (*m == '.') ? cfg::MORSE_UNIT_MS : (cfg::MORSE_UNIT_MS * 3);
                                        sleep_ms(duration);
                                        gpio_put(cfg::BUZZER_PIN, cfg::BUZZER_ACTIVE_LOW ? 1 : 0);
                                        sleep_ms(cfg::MORSE_UNIT_MS);
                                    }
                                }
                                sleep_ms(cfg::MORSE_UNIT_MS * 3);
                            }
                        }
                        send_http_200(client_id);
                    } else {
                        send_http_404(client_id);
                    }
                }
            }
        } else if (result == -2) {
            printf("[SERVIDOR] ESP reiniciado, reestableciendo servidor...\n");
            sleep_ms(3000);
            if (!start_server()) {
                printf("[SERVIDOR] ❌ Error reestableciendo servidor\n");
            }
        }
        
        sleep_ms(10); // Pequeña pausa para no saturar el CPU
    }
}
```

## Configuración requerida en Config.h

Asegúrate de tener estas configuraciones en tu archivo Config.h:

```cpp
// ===== API Externa =====
inline constexpr char API_HOST[]       = "localhost";     // Cambiar por IP real del servidor
inline constexpr int  API_PORT         = 3000;            // Puerto del API Express
inline constexpr char API_ENDPOINT[]   = "/api/earthquakes/event";
inline constexpr char DEVICE_ID[]      = "pico_sensor_01";

// ===== Sensor MPU6050 =====
inline constexpr float EARTHQUAKE_THRESHOLD = 15.0f;      // m/s² para terremoto
inline constexpr float VIBRATION_THRESHOLD  = 5.0f;       // m/s² para vibración menor
inline constexpr int   SENSOR_READ_INTERVAL = 100;        // ms entre lecturas
inline constexpr int   API_SEND_INTERVAL   = 5000;        // ms entre envíos a API
```

## Notas de implementación:

1. **Sensor MPU6050**: El código actual simula datos. Necesitas implementar la comunicación I2C real con el sensor.

2. **Timestamp**: Actualmente usa un timestamp fijo. Implementa un RTC o sincronización con servidor.

3. **Error handling**: Añade más manejo de errores para conexiones fallidas.

4. **Configuración de red**: Cambia "localhost" por la IP real del servidor Express.

5. **Optimización**: Considera usar interrupciones para la lectura del sensor en lugar de polling.

6. **Calibración**: Implementa calibración del sensor para mayor precisión.

7. **Buffer circular**: Para almacenar datos cuando no hay conexión de red.
