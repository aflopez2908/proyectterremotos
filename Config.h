// Config.h
#ifndef CONFIG_H_
#define CONFIG_H_

#include <cstdint>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"  // uart0/uart1

namespace cfg {

    // ===== UART hacia ESP8266 =====
    inline constexpr int      UART_INDEX    = 1;        // uart1 (GP4/GP5)
    inline constexpr uint32_t UART_BAUD     = 115200;
    inline constexpr int      UART_TX_PIN   = 4;    // Pico TX -> ESP RX
    inline constexpr int      UART_RX_PIN   = 5;    // Pico RX <- ESP TX
    inline uart_inst_t* UART() { return (UART_INDEX == 0) ? uart0 : uart1; }

    // ===== Wi-Fi (ESP8266 con firmware AT) =====
    inline constexpr char WIFI_SSID[]       = "Felipe";
    inline constexpr char WIFI_PASS[]       = "pipe1012";

    // ===== Parámetros AT / tiempos =====
    inline constexpr bool AT_DISABLE_ECHO   = true;     // ATE0
    inline constexpr int  AT_OK_TIMEOUT_MS  = 1500;
    inline constexpr int  AT_READY_TIMEOUT_MS = 3000;
    inline constexpr int  WIFI_JOIN_TIMEOUT_MS = 20000; // un poco más de margen
    inline constexpr int  IPD_WAIT_TIMEOUT_MS  = 60000;
    inline constexpr char CRLF[]            = "\r\n";

    // ===== Servidor HTTP =====
    // Nota: el main actual usa CIPMUX=1 explícito (múltiples conexiones).
    inline constexpr int  HTTP_PORT         = 80;
    inline constexpr bool SINGLE_CONNECTION = false;    // informativo
    inline constexpr int  SERVER_IDLE_TIMEOUT_S = 10;

    // ===== Buffers =====
    inline constexpr int  REQ_BUFFER_SIZE   = 1024;
    inline constexpr int  AT_CMD_MAX_LEN    = 64;

    // ===== Log por USB (stdio) =====
    inline constexpr bool LOG_TO_USB        = true;

    // ===== Pines de control (opcional) =====
    inline constexpr int  PIN_EN_CH_PD      = -1;
    inline constexpr int  PIN_RST           = -1;
    inline constexpr int  PIN_GPIO0         = -1;

    inline constexpr int BUZZER_PIN = 15; // Pin GPIO para el buzzer
    inline constexpr int BUZZER_PULSE_MS = 500; // Duración del pulso en ms
    // Buzzer: low level trigger -> activo en bajo y es tipo activo (no requiere tono)
    inline constexpr bool BUZZER_IS_PASSIVE = false;    // buzzer activo
    inline constexpr bool BUZZER_ACTIVE_LOW = true;     // se activa en nivel bajo
    inline constexpr int  BUZZER_TONE_HZ   = 800;       // ignorado si es activo

    // Morse
    inline constexpr int  MORSE_UNIT_MS    = 120;       // duración de un "punto"
    inline constexpr int  MORSE_MAX_LEN    = 64;        // máx. caracteres aceptados

    // ===== API Externa =====
    inline constexpr char API_HOST[]       = "tu-api.com";
    inline constexpr int  API_PORT         = 443;       // HTTPS
    inline constexpr char API_ENDPOINT[]   = "/api/earthquake";
    inline constexpr char DEVICE_ID[]      = "pico_sensor_01";
    
    // ===== Sensor MPU6050 (simulado) =====
    inline constexpr float EARTHQUAKE_THRESHOLD = 15.0f; // m/s² para terremoto
    inline constexpr float VIBRATION_THRESHOLD  = 5.0f;  // m/s² para vibración menor
    inline constexpr int   SENSOR_READ_INTERVAL = 100;   // ms entre lecturas
    inline constexpr int   API_SEND_INTERVAL   = 5000;   // ms entre envíos a API

} // namespace cfg

#endif // CONFIG_H_