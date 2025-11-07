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
    inline constexpr uint32_t UART_BAUD     = 9600;     // 9600 baudios para ESP8266
    inline constexpr int      UART_TX_PIN   = 4;        // Pico GP4 TX -> ESP RX
    inline constexpr int      UART_RX_PIN   = 5;        // Pico GP5 RX <- ESP TX
    inline uart_inst_t* UART() { return (UART_INDEX == 0) ? uart0 : uart1; }

    // ===== Wi-Fi (ESP8266 con firmware AT) =====
    inline constexpr char WIFI_SSID[]       = "Redmi Note 12S";     // 👈 Tu WiFi
    inline constexpr char WIFI_PASS[]       = "david022211";        // 👈 Tu contraseña

    // ===== Parámetros AT / tiempos =====
    inline constexpr bool AT_DISABLE_ECHO   = true;     // ATE0
    inline constexpr int  AT_OK_TIMEOUT_MS  = 3000;     // Más tiempo para respuestas
    inline constexpr int  AT_READY_TIMEOUT_MS = 5000;   // Más tiempo para inicialización
    inline constexpr int  WIFI_JOIN_TIMEOUT_MS = 30000; // Más tiempo para conectar WiFi
    inline constexpr int  IPD_WAIT_TIMEOUT_MS  = 60000;
    inline constexpr char CRLF[]            = "\r\n";

    // ===== Servidor HTTP =====
    inline constexpr int  HTTP_PORT         = 80;
    inline constexpr bool SINGLE_CONNECTION = false;    // informativo
    inline constexpr int  SERVER_IDLE_TIMEOUT_S = 10;

    // ===== Buffers =====
    inline constexpr int  REQ_BUFFER_SIZE   = 1024;
    inline constexpr int  AT_CMD_MAX_LEN    = 64;

    // ===== Log por USB (stdio) =====
    inline constexpr bool LOG_TO_USB        = true;

    // ===== Pines de control ESP8266 =====
    inline constexpr int  PIN_EN_CH_PD      = 7;        // GP7 - Pin ENABLE del ESP8266 (HIGH para habilitar)
    inline constexpr int  PIN_RST           = -1;       // No conectado 
    inline constexpr int  PIN_GPIO0         = -1;       // Conectado a 3.3V (modo normal)
    inline constexpr int  PIN_GPIO2         = 6;        // GP6 - Pin IO2 del ESP8266 (HIGH para boot normal)

    // ===== Buzzer =====
    inline constexpr int BUZZER_PIN = 15; // Pin GPIO para el buzzer
    inline constexpr int BUZZER_PULSE_MS = 500; 
    inline constexpr bool BUZZER_IS_PASSIVE = false;    
    inline constexpr bool BUZZER_ACTIVE_LOW = true;     
    inline constexpr int  BUZZER_TONE_HZ   = 800;       

    // Morse
    inline constexpr int  MORSE_UNIT_MS    = 120;       
    inline constexpr int  MORSE_MAX_LEN    = 64;        

    // ===== API Externa =====
    inline constexpr char API_HOST[]       = "192.168.56.1"; 
    inline constexpr int  API_PORT         = 3000;           
    inline constexpr char API_ENDPOINT[]   = "/api/pico/sensor-data";
    inline constexpr char DEVICE_ID[]      = "pico_sensor_01";
    
    // ===== Sensor MPU6050 =====
    inline constexpr int   MPU6050_SDA_PIN = 21;          
    inline constexpr int   MPU6050_SCL_PIN = 22;          
    inline constexpr int   I2C_INSTANCE    = 0;           
    inline constexpr int   I2C_BAUD_RATE   = 400000;      
    inline constexpr uint8_t MPU6050_ADDR  = 0x68;        
    
    // Umbrales de detección
    inline constexpr float EARTHQUAKE_THRESHOLD = 15.0f; 
    inline constexpr float VIBRATION_THRESHOLD  = 5.0f;  
    inline constexpr int   SENSOR_READ_INTERVAL = 100;   
    inline constexpr int   API_SEND_INTERVAL   = 5000;   
    inline constexpr int   STATUS_SEND_INTERVAL = 30000; 
    
    // Filtros y calibración
    inline constexpr float ACCEL_SCALE_FACTOR = 16384.0f; 
    inline constexpr float GRAVITY = 9.81f;               
    inline constexpr int   CALIBRATION_SAMPLES = 100;     

} // namespace cfg

#endif // CONFIG_H_
