#include "Config.h"
#include "lib/Esp8266HttpServer.h"
#include "lib/MPU6050.h"
#include "lib/SeismicMonitor.h"
#include "hardware/i2c.h"
#include <cstdio>

int main() {
    stdio_init_all();
    sleep_ms(2000);  // tiempo para abrir consola y estabilizar
    
    printf("===== Sistema de Detección Sísmica =====\n");
    printf("Dispositivo: %s\n", cfg::DEVICE_ID);
    printf("========================================\n");

    // ===== Configurar I2C para MPU6050 =====
    printf("Configurando I2C...\n");
    i2c_init(i2c0, cfg::I2C_BAUD_RATE);
    gpio_set_function(cfg::MPU6050_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(cfg::MPU6050_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(cfg::MPU6050_SDA_PIN);
    gpio_pull_up(cfg::MPU6050_SCL_PIN);
    
    // ===== UART para ESP8266 =====
    printf("Configurando UART para ESP8266...\n");
    uart_init(cfg::UART(), cfg::UART_BAUD);
    gpio_set_function(cfg::UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(cfg::UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(cfg::UART(), 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(cfg::UART(), true);

    // ===== Inicializar componentes =====
    
    // 1. Sensor MPU6050
    printf("Inicializando sensor MPU6050...\n");
    MPU6050 mpu_sensor(i2c0, cfg::MPU6050_ADDR);
    
    // 2. Servidor HTTP ESP8266
    printf("Inicializando servidor ESP8266...\n");
    Esp8266HttpServer server;
    
    // 3. Monitor sísmico
    printf("Inicializando monitor sísmico...\n");
    SeismicMonitor seismic_monitor(&mpu_sensor, &server);
    
    // ===== Inicialización del ESP8266 =====
    if (!server.begin()) {
        printf("Error: No se pudo inicializar el ESP8266\n");
        printf("Entrando en modo de diagnóstico...\n");
        // En caso de fallo, abrir puente diagnóstico
        server.diag_bridge();
        // No retorna
    }
    
    printf("ESP8266 inicializado correctamente\n");
    
    // ===== Inicialización del monitor sísmico =====
    if (!seismic_monitor.init()) {
        printf("Error: No se pudo inicializar el monitor sísmico\n");
        printf("Continuando solo con servidor HTTP...\n");
    } else {
        printf("Monitor sísmico inicializado correctamente\n");
    }
    
    printf("\n===== SISTEMA LISTO =====\n");
    printf("Servidor HTTP: puerto %d\n", cfg::HTTP_PORT);
    printf("API destino: %s:%d%s\n", cfg::API_HOST, cfg::API_PORT, cfg::API_ENDPOINT);
    printf("Intervalo de lectura: %d ms\n", cfg::SENSOR_READ_INTERVAL);
    printf("Envío de eventos: cada evento significativo\n");
    printf("Envío de estado: cada %d ms\n", cfg::STATUS_SEND_INTERVAL);
    printf("========================\n\n");

    // ===== Bucle principal =====
    uint64_t last_status_print = 0;
    const uint64_t STATUS_PRINT_INTERVAL = 60000; // Cada minuto
    
    while (true) {
        // 1. Procesar servidor HTTP (requests entrantes)
        server.loop();
        
        // 2. Procesar monitor sísmico (lectura de sensor y envío de datos)
        seismic_monitor.loop();
        
        // 3. Actualizar datos del sensor en el servidor para API
        server.set_sensor_data(seismic_monitor.get_current_sensor_data(), seismic_monitor.is_sensor_ok());
        
        // 4. Imprimir estado cada minuto (opcional, para debug)
        uint64_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_status_print >= STATUS_PRINT_INTERVAL) {
            seismic_monitor.print_sensor_status();
            last_status_print = current_time;
        }
        
        // 5. Pequeña pausa para no saturar el CPU
        sleep_ms(10);
    }
    
    return 0; // Nunca debería llegar aquí
}