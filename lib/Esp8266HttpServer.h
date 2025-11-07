#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string_view>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "Config.h"
#include "lib/MPU6050.h"  // Para SensorData

class Esp8266HttpServer {
public:
    // ... existing methods ...

    // Establecer datos del sensor para API
    void set_sensor_data(const SensorData& data, bool sensor_ok);

    // ... existing methods ...
    Esp8266HttpServer();

    // Inicializa UART, asocia Wi-Fi y levanta CIPSERVER.
    // Devuelve false si no obtiene "OK" del ESP a 115200.
    bool begin();

    // Bucle principal: atiende +IPD y re-arma servidor si detecta "ready".
    [[noreturn]] void loop();

    // Puente USB↔ESP para diagnóstico.
    [[noreturn]] void diag_bridge();

public:
    // Envía datos del sensor a la API externa
    bool send_earthquake_data(float accel_x, float accel_y, float accel_z, 
                             float magnitude, bool is_earthquake);

    // Cliente HTTP para API externa
    bool http_post_json(const char* host, int port, const char* path, 
                       const char* json_data);

private:
    bool get_buzzer = false;
    uint32_t last_api_send = 0;
    SensorData current_sensor_data;
    bool sensor_ok = false;

    // --- Helpers UART/AT ---
    void uart_send_raw(const char* s);
    void send_at(const char* cmd);
    void flush_uart_quiet(uint32_t quiet_ms);
    int  wait_for_any(const char* const tokens[], int ntokens, uint32_t timeout_ms);
    bool wait_for(const char* tok, uint32_t timeout_ms);
    int  read_bytes(uint8_t* buf, int maxlen, uint32_t timeout_ms);

    // +IPD o "ready" tras reset: 1=+IPD, -2=ready, 0=timeout/otro
    int  wait_ipd_or_ready(int* out_id, int* out_len, uint32_t timeout_ms);

    // HTTP con CIPMUX=1
    void send_http_200(int id);
    void send_http_404(int id);

    // CIPMUX=1, CIPSERVER=1,80 (+ CIPSTO). Imprime estado.
    bool start_server();
    
    // Simulación del sensor MPU6050
    void read_mpu6050(float* accel_x, float* accel_y, float* accel_z);
    float calculate_magnitude(float x, float y, float z);

private:
    uint8_t reqbuf_[cfg::REQ_BUFFER_SIZE] = {0};
};
