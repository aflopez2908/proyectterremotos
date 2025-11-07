#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>

// Configuración UART idéntica a Config.h
#define UART_ID uart1
#define BAUD_RATE 9600
#define UART_TX_PIN 4  // GP4 -> ESP RX
#define UART_RX_PIN 5  // GP5 <- ESP TX
#define ESP_EN_PIN 7   // GP7 -> ESP EN
#define ESP_IO2_PIN 6  // GP6 -> ESP IO2

int main() {
    stdio_init_all();
    sleep_ms(2000); // Tiempo para conectar USB
    
    printf("🔧 DIAGNÓSTICO ESP8266 - Test UART\n");
    printf("===================================\n");
    
    // Inicializar UART
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);
    
    // Configurar pines de control ESP8266
    gpio_init(ESP_EN_PIN);
    gpio_set_dir(ESP_EN_PIN, GPIO_OUT);
    gpio_init(ESP_IO2_PIN);
    gpio_set_dir(ESP_IO2_PIN, GPIO_OUT);
    
    printf("📍 Pines configurados:\n");
    printf("  UART TX: GP%d -> ESP RX\n", UART_TX_PIN);
    printf("  UART RX: GP%d <- ESP TX\n", UART_RX_PIN);
    printf("  ESP EN:  GP%d\n", ESP_EN_PIN);
    printf("  ESP IO2: GP%d\n", ESP_IO2_PIN);
    
    // Configurar ESP8266 en modo normal
    gpio_put(ESP_IO2_PIN, 1);  // IO2 HIGH = modo normal
    gpio_put(ESP_EN_PIN, 0);   // Reset ESP
    printf("\n🔄 Reiniciando ESP8266...\n");
    sleep_ms(100);
    gpio_put(ESP_EN_PIN, 1);   // Habilitar ESP
    sleep_ms(2000);            // Esperar boot
    
    printf("✅ ESP8266 encendido. Probando comunicación...\n");
    
    int test_count = 0;
    char buffer[256];
    
    while (test_count < 10) {
        test_count++;
        printf("\n🔍 Test #%d - Enviando AT...\n", test_count);
        
        // Enviar comando AT
        uart_puts(UART_ID, "AT\r\n");
        
        // Leer respuesta por 3 segundos
        uint32_t start_time = to_ms_since_boot(get_absolute_time());
        int buffer_pos = 0;
        bool response_received = false;
        
        while ((to_ms_since_boot(get_absolute_time()) - start_time) < 3000) {
            if (uart_is_readable(UART_ID)) {
                char c = uart_getc(UART_ID);
                if (buffer_pos < 255) {
                    buffer[buffer_pos++] = c;
                    buffer[buffer_pos] = '\0';
                }
                response_received = true;
                printf("%c", c); // Imprimir caracteres recibidos
            }
            sleep_ms(1);
        }
        
        if (response_received) {
            printf("\n✅ RESPUESTA RECIBIDA: %s\n", buffer);
            if (strstr(buffer, "OK") != NULL) {
                printf("🎉 ¡ESP8266 RESPONDE CORRECTAMENTE!\n");
                break;
            }
        } else {
            printf("\n❌ Sin respuesta del ESP8266\n");
        }
        
        // Limpiar buffer
        buffer_pos = 0;
        buffer[0] = '\0';
        
        sleep_ms(2000);
    }
    
    if (test_count >= 10) {
        printf("\n🚨 PROBLEMA DETECTADO:\n");
        printf("   1. Verificar conexiones de cables\n");
        printf("   2. Verificar alimentación 3.3V del ESP\n");
        printf("   3. Verificar GND común\n");
        printf("   4. Verificar que ESP tiene firmware AT\n");
    }
    
    printf("\n🔄 Loop infinito para monitoreo manual...\n");
    printf("   Puedes probar comandos AT desde minicom\n");
    
    // Loop infinito para debug manual
    while (true) {
        // Reenviar todo de USB a ESP
        if (stdio_usb.in_chars_available && stdio_usb.in_chars_available()) {
            int c = getchar_timeout_us(0);
            if (c != PICO_ERROR_TIMEOUT) {
                uart_putc(UART_ID, c);
            }
        }
        
        // Reenviar todo de ESP a USB
        if (uart_is_readable(UART_ID)) {
            char c = uart_getc(UART_ID);
            putchar(c);
        }
        
        sleep_ms(1);
    }
    
    return 0;
}
