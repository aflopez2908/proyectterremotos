#include "lib/Esp8266HttpServer.h"
#include <cstdio>
#include <cstring>
#include <cctype>
#include <string_view>
#include <cmath>
#include "../web_page.hpp"

// --- Helpers URL ---
static inline int hexval(int c){ if(c>='0'&&c<='9') return c-'0'; if(c>='A'&&c<='F') return 10+c-'A'; if(c>='a'&&c<='f') return 10+c-'a'; return -1; }
static void url_decode_inplace(char* s){ char* w=s; for(char* r=s; *r; ){ if(*r=='%'){ int h1=hexval(*(r+1)), h2=hexval(*(r+2)); if(h1>=0&&h2>=0){ *w++=(char)((h1<<4)|h2); r+=3; } else { *w++=*r++; } } else { *w++= (*r=='+') ? ' ' : *r; r++; } } *w='\0'; }

using namespace cfg;

Esp8266HttpServer::Esp8266HttpServer() : sensor_ok(false) {
    memset(&current_sensor_data, 0, sizeof(current_sensor_data));
}

void Esp8266HttpServer::set_sensor_data(const SensorData& data, bool ok) {
    current_sensor_data = data;
    sensor_ok = ok;
}

bool Esp8266HttpServer::begin() {

    printf("[UART] Probando enlace a %u...\n", (unsigned)UART_BAUD);
    flush_uart_quiet(100);
    bool got_ok = false;
    for (int i = 0; i < 10 && !got_ok; ++i) {
        send_at("AT");
        got_ok = wait_for("OK\r\n", 300);
        sleep_ms(200);
    }
    if (!got_ok) {
        printf("[UART] ❌ Sin OK a %u. Revisa GP4→RX, GP5←TX, EN/RST altos y GND común.\n",
               (unsigned)UART_BAUD);
        return false;
    }
    printf("[UART] ✅ OK.\n");

    if (AT_DISABLE_ECHO) { send_at("ATE0"); wait_for("OK\r\n", 500); }
    send_at("AT+CWMODE=1"); wait_for("OK\r\n", 500);

    printf("\n[WiFi] Conectando a \"%s\" (timeout: %d ms)...\n",
           WIFI_SSID, WIFI_JOIN_TIMEOUT_MS);
    char cmd[128];
    std::snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"", WIFI_SSID, WIFI_PASS);
    send_at(cmd);
    const char* toks[] = {"OK\r\n", "FAIL\r\n", "ERROR\r\n"};
    int r = wait_for_any(toks, 3, WIFI_JOIN_TIMEOUT_MS);
    if (r != 0) {
        printf("[WiFi] ❌ CWJAP falló con respuesta: %d (%s)\n", r, 
               r == 1 ? "FAIL" : r == 2 ? "ERROR" : "TIMEOUT");
        printf("[WiFi] Verifica SSID '%s' y contraseña\n", WIFI_SSID);
        return false;
    }
    printf("[WiFi] ✅ Conectado exitosamente a '%s'\n", WIFI_SSID);

    // Obtener dirección IP
    printf("[WiFi] Obteniendo dirección IP...\n");
    send_at("AT+CIFSR");
    flush_uart_quiet(2000); // Esperar respuesta con IP
    
    return start_server();
}

[[noreturn]] void Esp8266HttpServer::loop() {
    uint32_t last_sensor_read = 0;
    
    while (true) {
        // ===== MONITOREO SÍSMICO =====
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_sensor_read >= cfg::SENSOR_READ_INTERVAL) {
            last_sensor_read = now;
            
            float accel_x, accel_y, accel_z;
            read_mpu6050(&accel_x, &accel_y, &accel_z);
            
            float magnitude = calculate_magnitude(accel_x, accel_y, accel_z);
            bool is_earthquake = magnitude > cfg::EARTHQUAKE_THRESHOLD;
            bool is_vibration = magnitude > cfg::VIBRATION_THRESHOLD;
            
            if (is_earthquake || is_vibration) {
                printf("[SENSOR] Magnitud: %.2f m/s² %s\n", magnitude,
                       is_earthquake ? "🚨 TERREMOTO!" : "📳 vibración");
                
                // Enviar datos a la API
                send_earthquake_data(accel_x, accel_y, accel_z, magnitude, is_earthquake);
            }
        }
        
        // ===== MANEJO HTTP =====
        int id = -1, len = 0;
        int ev = wait_ipd_or_ready(&id, &len, 100); // Timeout más corto para no bloquear sensor

        if (ev == -2) {
            printf("\n[ESP] Detectado 'ready'. Reconfigurando servidor...\n");
            if (!start_server()) {
                printf("[ESP] ❌ No se pudo rearmar el servidor. Entrando a diagnóstico.\n");
                diag_bridge();
            }
            continue;
        }
        if (ev != 1) continue;

        printf("[HTTP] Nueva conexión ID=%d, %d bytes\n", id, len);

        int to_read = len; if (to_read > REQ_BUFFER_SIZE) to_read = REQ_BUFFER_SIZE;
        int got = read_bytes(reqbuf_, to_read, 3000);
        if (got <= 0) continue;

        bool get_root = false;
        bool get_favicon = false;
        bool get_api_sensor = false;
        const uint8_t* p_space = nullptr;
        if (got >= 5 && std::memcmp(reqbuf_, "GET /", 5) == 0) {
            p_space  = (const uint8_t*)std::memchr(reqbuf_, ' ', got); // space after path
            if (p_space) {
                const uint8_t* pb = p_space + 1; // should point to '/'
                if (pb < reqbuf_ + got && *pb == '/') {
                    const uint8_t* pe = (const uint8_t*)std::memchr(pb, ' ', got - (pb - reqbuf_));
                    size_t plen = pe ? (size_t)(pe - pb) : 1;
                    get_root = (plen == 1); // "/"
                    if (plen >= 11 && std::memcmp(pb, "/api/sensor", 11) == 0) {
                        get_api_sensor = true;
                    }
                    if (plen >= 12 && std::memcmp(pb, "/favicon.ico", 12) == 0) {
                        get_favicon = true;
                    }
                }
            }
        }
        if (get_root) {
            send_http_200(id);
        } else if (get_api_sensor) {
            send_api_sensor_json(id);
        } else if (get_favicon) {
            const char hdr[] =
                "HTTP/1.1 204 No Content\r\n"
                "Connection: close\r\n\r\n";
            char cmd[40]; std::snprintf(cmd,sizeof(cmd),"AT+CIPSEND=%d,%d", id, (int)sizeof(hdr)-1);
            send_at(cmd);
            if (wait_for(">", 1000)) { uart_send_raw(hdr); wait_for("SEND OK\r\n", 1500); }
            std::snprintf(cmd,sizeof(cmd),"AT+CIPCLOSE=%d",id); send_at(cmd);
        } else {
            send_http_404(id);
        }
    }
}

[[noreturn]] void Esp8266HttpServer::diag_bridge() {
    printf("\n[DIAG] Puente USB↔ESP. Teclea AT y Enter (\\r\\n). Pulsa RST del ESP para ver el bootlog.\n");
    while (true) {
        int ch = getchar_timeout_us(1000);
        if (ch != PICO_ERROR_TIMEOUT) {
            if (ch == '\n') { uart_putc_raw(UART(), '\r'); uart_putc_raw(UART(), '\n'); }
            else uart_putc_raw(UART(), (char)ch);
        }
        if (uart_is_readable(UART())) putchar(uart_getc(UART()));
    }
}

// ======== privados ========

void Esp8266HttpServer::uart_send_raw(const char* s){ while(*s) uart_putc_raw(UART(), *s++); }
void Esp8266HttpServer::send_at(const char* cmd){ uart_send_raw(cmd); uart_putc_raw(UART(), '\r'); uart_putc_raw(UART(), '\n'); }

void Esp8266HttpServer::flush_uart_quiet(uint32_t quiet_ms){
    absolute_time_t dl = make_timeout_time_ms(quiet_ms);
    while(!time_reached(dl)){
        if(uart_is_readable(UART())){
            int ch = uart_getc(UART());
            if (LOG_TO_USB) putchar(ch);
            dl = make_timeout_time_ms(quiet_ms);
        }
    }
}

int Esp8266HttpServer::wait_for_any(const char* const tokens[], int ntokens, uint32_t timeout_ms){
    if (ntokens > 8) ntokens = 8;
    size_t n[8], m[8];
    for (int i=0;i<ntokens;++i){ n[i]=std::strlen(tokens[i]); m[i]=0; }
    absolute_time_t dl = make_timeout_time_ms(timeout_ms);
    while(!time_reached(dl)){
        if(!uart_is_readable(UART())){ tight_loop_contents(); continue; }
        int ch = uart_getc(UART());
        if (LOG_TO_USB) putchar(ch);
        for(int i=0;i<ntokens;++i){
            if ((size_t)ch == (size_t)tokens[i][m[i]]) {
                if(++m[i]==n[i]) return i;
            } else {
                m[i] = (ch == tokens[i][0]) ? 1 : 0;
            }
        }
    }
    return -1;
}
bool Esp8266HttpServer::wait_for(const char* tok, uint32_t ms){ const char* t[]={tok}; return wait_for_any(t,1,ms)==0; }

int Esp8266HttpServer::read_bytes(uint8_t* buf, int maxlen, uint32_t timeout_ms){
    int got = 0;
    absolute_time_t dl = make_timeout_time_ms(timeout_ms);
    while(got < maxlen && !time_reached(dl)){
        if(uart_is_readable(UART())){
            buf[got++] = (uint8_t)uart_getc(UART());
            if (LOG_TO_USB) putchar(buf[got-1]);
            dl = make_timeout_time_ms(timeout_ms);
        } else tight_loop_contents();
    }
    return got;
}

int Esp8266HttpServer::wait_ipd_or_ready(int* out_id, int* out_len, uint32_t timeout_ms){
    static const char tag[] = "+IPD,"; const size_t tag_len = sizeof(tag)-1;
    const char* tok_ready = "ready\r\n";
    size_t m_ipd=0, m_ready=0;
    int id=0,len=0; bool have_id=false, have_len=false;

    absolute_time_t dl = make_timeout_time_ms(timeout_ms);
    while(!time_reached(dl)){
        if(!uart_is_readable(UART())){ tight_loop_contents(); continue; }
        int ch = uart_getc(UART());
        if (LOG_TO_USB) putchar(ch);

        // ready?
        if (ch == tok_ready[m_ready]) {
            if (++m_ready == std::strlen(tok_ready)) return -2;
        } else {
            m_ready = (ch == tok_ready[0]) ? 1 : 0;
        }

        // +IPD,?
        if (ch == (int)tag[m_ipd]) {
            if (++m_ipd == tag_len) {
                // ID
                while(!time_reached(dl)){
                    if(!uart_is_readable(UART())){ tight_loop_contents(); continue; }
                    int c = uart_getc(UART()); if (LOG_TO_USB) putchar(c);
                    if(c==','){ have_id=true; break; }
                    if(!std::isdigit(c)) return 0;
                    id=id*10+(c-'0');
                }
                // LEN
                while(!time_reached(dl)){
                    if(!uart_is_readable(UART())){ tight_loop_contents(); continue; }
                    int c = uart_getc(UART()); if (LOG_TO_USB) putchar(c);
                    if(c==':'){ have_len=true; break; }
                    if(!std::isdigit(c)) return 0;
                    len=len*10+(c-'0');
                }
                if(have_id && have_len){ *out_id=id; *out_len=len; return 1; }
                return 0;
            }
        } else {
            m_ipd = (ch==tag[0]) ? 1 : 0;
        }
    }
    return 0;
}

void Esp8266HttpServer::send_http_200(int id){
    char hdr[256];
    int hlen = std::snprintf(hdr,sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %u\r\n"
        "Connection: close\r\n\r\n",
        web::kIndexContentType,(unsigned)web::kIndexHtmlLen);
    int total = hlen + (int)web::kIndexHtmlLen;

    char cmd[40]; std::snprintf(cmd,sizeof(cmd),"AT+CIPSEND=%d,%d", id, total);
    send_at(cmd);
    if(!wait_for(">",2000)){ std::snprintf(cmd,sizeof(cmd),"AT+CIPCLOSE=%d",id); send_at(cmd); return; }
    uart_send_raw(hdr); uart_send_raw(web::kIndexHtml);
    wait_for("SEND OK\r\n",3000);
    std::snprintf(cmd,sizeof(cmd),"AT+CIPCLOSE=%d",id); send_at(cmd);
}

void Esp8266HttpServer::send_http_404(int id){
    static const char body[]="<h1>404 Not Found</h1>";
    char hdr[200];
    int hlen = std::snprintf(hdr,sizeof(hdr),
        "HTTP/1.1 404 Not Found\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %u\r\n"
        "Connection: close\r\n\r\n",(unsigned)sizeof(body)-1);
    int total = hlen + (int)sizeof(body)-1;

    char cmd[40]; std::snprintf(cmd,sizeof(cmd),"AT+CIPSEND=%d,%d", id, total);
    send_at(cmd);
    if(!wait_for(">",2000)){ std::snprintf(cmd,sizeof(cmd),"AT+CIPCLOSE=%d",id); send_at(cmd); return; }
    uart_send_raw(hdr); uart_send_raw(body);
    wait_for("SEND OK\r\n",3000);
    std::snprintf(cmd,sizeof(cmd),"AT+CIPCLOSE=%d",id); send_at(cmd);
}

bool Esp8266HttpServer::start_server(){
    printf("[HTTP] Iniciando servidor HTTP...\n");
    
    send_at("AT+CIPMUX=1");
    if (!wait_for("OK\r\n", 800)) {
        const char* tk[]={"no change\r\n"};
        wait_for_any(tk,1,400);
    }
    printf("[HTTP] CIPMUX configurado\n");
    
    send_at("AT+CIPSERVER=0");
    wait_for("OK\r\n", 600); // ignora si no llega

    char cmd[32];
    std::snprintf(cmd,sizeof(cmd),"AT+CIPSERVER=1,%d", HTTP_PORT);
    send_at(cmd);
    const char* toks_srv[]={"OK\r\n","no change\r\n"};
    int tr = wait_for_any(toks_srv, 2, 1500);
    if (tr < 0) { 
        printf("[HTTP] ❌ CIPSERVER no arrancó (timeout)\n"); 
        return false; 
    }
    printf("[HTTP] CIPSERVER iniciado en puerto %d\n", HTTP_PORT);

    std::snprintf(cmd,sizeof(cmd),"AT+CIPSTO=%d", SERVER_IDLE_TIMEOUT_S);
    send_at(cmd); wait_for("OK\r\n", 800);

    send_at("AT+CIPSTATUS"); 
    flush_uart_quiet(500);
    printf("[HTTP] Servidor listo y esperando conexiones\n");
    return true;
}

void Esp8266HttpServer::send_api_sensor_json(int id) {
    char json_body[256];
    int len = std::snprintf(json_body, sizeof(json_body),
        "{"
        "\"accel_x\":%.6f,"
        "\"accel_y\":%.6f,"
        "\"accel_z\":%.6f,"
        "\"gyro_x\":%.3f,"
        "\"gyro_y\":%.3f,"
        "\"gyro_z\":%.3f,"
        "\"magnitude\":%.6f,"
        "\"timestamp\":%lu,"
        "\"status\":\"%s\""
        "}",
        current_sensor_data.accel_x,
        current_sensor_data.accel_y,
        current_sensor_data.accel_z,
        current_sensor_data.gyro_x,
        current_sensor_data.gyro_y,
        current_sensor_data.gyro_z,
        current_sensor_data.magnitude,
        (unsigned long)current_sensor_data.timestamp,
        sensor_ok ? "online" : "offline"
    );
    
    char hdr[160];
    int hlen = std::snprintf(hdr, sizeof(hdr),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Connection: close\r\n\r\n",
        len);
    
    int total = hlen + len;
    char cmd[48]; std::snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%d,%d", id, total);
    send_at(cmd);
    if (wait_for(">", 2000)) {
        uart_send_raw(hdr);
        uart_send_raw(json_body);
        wait_for("SEND OK\r\n", 3000);
    }
    std::snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%d", id);
    send_at(cmd);
}

bool Esp8266HttpServer::http_post_json(const char* host, int port, const char* path, const char* json_data) {
    char cmd[128];
    
    // Conectar TCP al host de la API
    std::snprintf(cmd, sizeof(cmd), "AT+CIPSTART=4,\"TCP\",\"%s\",%d", host, port);
    send_at(cmd);
    
    const char* conn_tokens[] = {"OK\r\n", "ALREADY CONNECTED\r\n", "ERROR\r\n"};
    int conn_result = wait_for_any(conn_tokens, 3, 5000);
    if (conn_result < 0 || conn_result == 2) {
        printf("[API] ❌ Error conectando a %s:%d\n", host, port);
        return false;
    }
    
    // Preparar HTTP POST request
    char http_request[512];
    int content_length = strlen(json_data);
    int request_len = std::snprintf(http_request, sizeof(http_request),
        "POST %s HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        path, host, content_length, json_data);
    
    // Enviar datos
    std::snprintf(cmd, sizeof(cmd), "AT+CIPSEND=4,%d", request_len);
    send_at(cmd);
    
    if (wait_for(">", 2000)) {
        uart_send_raw(http_request);
        if (wait_for("SEND OK\r\n", 3000)) {
            printf("[API] ✅ Datos enviados a %s\n", host);
            
            // Esperar respuesta
            flush_uart_quiet(2000);
            
            // Cerrar conexión
            send_at("AT+CIPCLOSE=4");
            wait_for("OK\r\n", 1000);
            
            return true;
        }
    }
    
    printf("[API] ❌ Error enviando datos\n");
    send_at("AT+CIPCLOSE=4");
    return false;
}

// ===== SIMULACIÓN SENSOR MPU6050 =====

void Esp8266HttpServer::read_mpu6050(float* accel_x, float* accel_y, float* accel_z) {
    // Simulación de lecturas del sensor con ruido aleatorio
    static uint32_t seed = 12345;
    seed = seed * 1103515245 + 12345; // LCG simple
    
    float noise_x = ((float)(seed % 1000) / 1000.0f - 0.5f) * 2.0f; // ±1.0
    seed = seed * 1103515245 + 12345;
    float noise_y = ((float)(seed % 1000) / 1000.0f - 0.5f) * 2.0f;
    seed = seed * 1103515245 + 12345;
    float noise_z = ((float)(seed % 1000) / 1000.0f - 0.5f) * 2.0f;
    
    // Valores base (gravedad + ruido)
    *accel_x = 0.0f + noise_x;
    *accel_y = 0.0f + noise_y;
    *accel_z = 9.81f + noise_z;
    
    // Simular ocasionalmente un evento sísmico
    if ((seed % 1000) < 5) { // 0.5% probabilidad
        *accel_x += ((seed % 100) / 10.0f) - 5.0f; // ±5-15 m/s²
        *accel_y += ((seed % 100) / 10.0f) - 5.0f;
        *accel_z += ((seed % 100) / 10.0f) - 5.0f;
    }
}

float Esp8266HttpServer::calculate_magnitude(float x, float y, float z) {
    return sqrt(x*x + y*y + z*z);
}

// ===== ENVÍO DE DATOS SÍSMICOS =====

bool Esp8266HttpServer::send_earthquake_data(float accel_x, float accel_y, float accel_z, 
                                           float magnitude, bool is_earthquake) {
    // Control de frecuencia de envío
    uint32_t now = to_ms_since_boot(get_absolute_time());
    if (now - last_api_send < cfg::API_SEND_INTERVAL) {
        return true; // No enviar aún
    }
    last_api_send = now;
    
    // Crear JSON payload
    char json_data[256];
    std::snprintf(json_data, sizeof(json_data),
        "{"
        "\"device_id\":\"%s\","
        "\"timestamp\":%lu,"
        "\"accel_x\":%.2f,"
        "\"accel_y\":%.2f,"
        "\"accel_z\":%.2f,"
        "\"magnitude\":%.2f,"
        "\"is_earthquake\":%s"
        "}",
        cfg::DEVICE_ID,
        now,
        accel_x, accel_y, accel_z,
        magnitude,
        is_earthquake ? "true" : "false"
    );
    
    printf("[SENSOR] Enviando: mag=%.2f %s\n", magnitude, 
           is_earthquake ? "🚨TERREMOTO" : "📊normal");
    
    return http_post_json(cfg::API_HOST, cfg::API_PORT, cfg::API_ENDPOINT, json_data);
}
