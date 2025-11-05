#include "Config.h"
#include "lib/Esp8266HttpServer.h"

int main() {
    stdio_init_all();
    sleep_ms(1200);  // tiempo para abrir consola

    // UART1 en GP4/GP5 @115200 (según Config.h)
    uart_init(cfg::UART(), cfg::UART_BAUD);
    gpio_set_function(cfg::UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(cfg::UART_RX_PIN, GPIO_FUNC_UART);
    uart_set_format(cfg::UART(), 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(cfg::UART(), true);

    Esp8266HttpServer server;
    if (!server.begin()) {
        // Sin enlace con el ESP: abre puente diagnóstico (no retorna)
        server.diag_bridge();
    }

    // Bucle principal del servidor (no retorna)
    server.loop();
    return 0;
}