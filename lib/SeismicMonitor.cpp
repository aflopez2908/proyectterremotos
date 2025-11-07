#include "SeismicMonitor.h"
#include <cstdio>
#include <cstring>

SeismicMonitor::SeismicMonitor(MPU6050* mpu_sensor, Esp8266HttpServer* http_server)
    : sensor(mpu_sensor), server(http_server), buffer_index(0), buffer_full(false),
      last_sensor_read(0), last_api_send(0), last_status_send(0),
      sensor_initialized(false), consecutive_errors(0) {
}

bool SeismicMonitor::init() {
    printf("[SeismicMonitor] Inicializando...\n");
    
    // Inicializar sensor MPU6050
    if (!sensor->init()) {
        printf("[SeismicMonitor] Error: Falló la inicialización del sensor\n");
        return false;
    }
    
    // Calibrar sensor (importante hacer esto con el dispositivo en reposo)
    printf("[SeismicMonitor] Calibrando sensor (mantener en reposo)...\n");
    if (!sensor->calibrate(cfg::CALIBRATION_SAMPLES)) {
        printf("[SeismicMonitor] Advertencia: Falló la calibración inicial\n");
        // Continuamos sin calibración, pero marcamos el sensor como problemático
        consecutive_errors = MAX_CONSECUTIVE_ERRORS / 2;
    }
    
    sensor_initialized = true;
    
    printf("[SeismicMonitor] Inicialización completada\n");
    return true;
}

void SeismicMonitor::loop() {
    uint64_t current_time = to_ms_since_boot(get_absolute_time());
    
    // 1. Leer datos del sensor periódicamente
    if (current_time - last_sensor_read >= cfg::SENSOR_READ_INTERVAL) {
        last_sensor_read = current_time;
        
        SensorData data;
        if (sensor->read_sensor_data(data)) {
            // Reset contador de errores en lecturas exitosas
            if (consecutive_errors > 0) {
                consecutive_errors--;
            }
            
            // Agregar al buffer
            add_to_buffer(data);
            
            // Imprimir datos del sensor en terminal
            printf("[MPU6050] Accel: X=%.3f, Y=%.3f, Z=%.3f m/s² | Gyro: X=%.2f, Y=%.2f, Z=%.2f °/s | Mag: %.3f m/s²\n",
                   data.accel_x, data.accel_y, data.accel_z,
                   data.gyro_x, data.gyro_y, data.gyro_z,
                   data.magnitude);
            
            // Verificar si es un evento significativo
            if (sensor->is_significant_movement(data, cfg::VIBRATION_THRESHOLD)) {
                SeismicEvent event;
                event.data = data;
                event.is_significant = data.magnitude >= cfg::EARTHQUAKE_THRESHOLD;
                event.event_type = sensor->get_event_type(data.magnitude);
                event.detected_at = current_time;
                
                printf("[SeismicMonitor] Evento detectado: %s (magnitud: %.2f m/s²)\n",
                       event.event_type, data.magnitude);
                
                // Enviar inmediatamente si hay conectividad
                if (is_wifi_connected()) {
                    send_sensor_data_to_api(event);
                    last_api_send = current_time;
                }
            }
        } else {
            consecutive_errors++;
            printf("[SeismicMonitor] Error leyendo sensor (%d errores consecutivos)\n", consecutive_errors);
            
            // Si hay muchos errores, intentar reinicializar
            if (consecutive_errors >= MAX_CONSECUTIVE_ERRORS) {
                printf("[SeismicMonitor] Demasiados errores, reintentando inicialización...\n");
                sensor_initialized = sensor->init();
                consecutive_errors = MAX_CONSECUTIVE_ERRORS / 2; // Reset parcial
            }
        }
    }
    
    // 2. Enviar estado periódico al API
    if (current_time - last_status_send >= cfg::STATUS_SEND_INTERVAL) {
        if (is_wifi_connected()) {
            send_status_to_api();
            last_status_send = current_time;
        }
    }
}

void SeismicMonitor::add_to_buffer(const SensorData& data) {
    sensor_buffer[buffer_index] = data;
    buffer_index = (buffer_index + 1) % BUFFER_SIZE;
    
    if (buffer_index == 0) {
        buffer_full = true;
    }
}

float SeismicMonitor::calculate_average_magnitude(int samples) {
    if (!buffer_full && buffer_index == 0) return 0.0f;
    
    int actual_samples = buffer_full ? BUFFER_SIZE : buffer_index;
    if (samples > actual_samples) samples = actual_samples;
    
    float sum = 0.0f;
    int start_idx = buffer_full ? buffer_index : 0;
    
    for (int i = 0; i < samples; i++) {
        int idx = (start_idx - 1 - i + BUFFER_SIZE) % BUFFER_SIZE;
        sum += sensor_buffer[idx].magnitude;
    }
    
    return sum / samples;
}

bool SeismicMonitor::is_wifi_connected() {
    // Verificación simple basada en si el servidor HTTP está funcionando
    // En una implementación más robusta, se podría hacer ping al API
    return server != nullptr; // Placeholder - mejorar implementación
}

bool SeismicMonitor::send_sensor_data_to_api(const SeismicEvent& event) {
    if (!server) return false;
    
    char json_buffer[512];
    format_sensor_data_json(event, json_buffer, sizeof(json_buffer));
    
    printf("[SeismicMonitor] Enviando datos al API: %s\n", json_buffer);
    
    // Usar el método del servidor para enviar datos
    bool success = server->http_post_json(cfg::API_HOST, cfg::API_PORT, cfg::API_ENDPOINT, json_buffer);
    
    if (success) {
        printf("[SeismicMonitor] Datos enviados exitosamente\n");
    } else {
        printf("[SeismicMonitor] Error enviando datos al API\n");
    }
    
    return success;
}

bool SeismicMonitor::send_status_to_api() {
    printf("[SeismicMonitor] Enviando estado al API...\n");
    
    char json_buffer[256];
    float avg_magnitude = calculate_average_magnitude(10);
    
    snprintf(json_buffer, sizeof(json_buffer),
        "{"
        "\"device_id\":\"%s\","
        "\"timestamp\":%lu,"
        "\"status\":\"online\","
        "\"sensor_ok\":%s,"
        "\"avg_magnitude\":%.3f,"
        "\"buffer_count\":%d,"
        "\"errors\":%d"
        "}",
        cfg::DEVICE_ID,
        (unsigned long)to_ms_since_boot(get_absolute_time()),
        is_sensor_ok() ? "true" : "false",
        avg_magnitude,
        get_buffer_count(),
        consecutive_errors
    );
    
    printf("[SeismicMonitor] Estado: %s\n", json_buffer);
    
    // Enviar al endpoint de estado (puedes crear uno diferente si lo necesitas)
    if (server) {
        return server->http_post_json(cfg::API_HOST, cfg::API_PORT, "/api/pico/status", json_buffer);
    }
    
    return false;
}

void SeismicMonitor::format_sensor_data_json(const SeismicEvent& event, char* json_buffer, size_t buffer_size) {
    snprintf(json_buffer, buffer_size,
        "{"
        "\"device_id\":\"%s\","
        "\"timestamp\":%lu,"
        "\"acceleration_x\":%.6f,"
        "\"acceleration_y\":%.6f,"
        "\"acceleration_z\":%.6f,"
        "\"gyro_x\":%.3f,"
        "\"gyro_y\":%.3f,"
        "\"gyro_z\":%.3f,"
        "\"magnitude\":%.6f,"
        "\"event_type\":\"%s\","
        "\"is_significant\":%s"
        "}",
        cfg::DEVICE_ID,
        (unsigned long)event.data.timestamp,
        event.data.accel_x,
        event.data.accel_y,
        event.data.accel_z,
        event.data.gyro_x,
        event.data.gyro_y,
        event.data.gyro_z,
        event.data.magnitude,
        event.event_type,
        event.is_significant ? "true" : "false"
    );
}

int SeismicMonitor::get_buffer_count() const {
    return buffer_full ? BUFFER_SIZE : buffer_index;
}

float SeismicMonitor::get_current_magnitude() {
    if (get_buffer_count() == 0) return 0.0f;
    
    int last_idx = (buffer_index - 1 + BUFFER_SIZE) % BUFFER_SIZE;
    return sensor_buffer[last_idx].magnitude;
}

bool SeismicMonitor::is_sensor_ok() const {
    return sensor_initialized && consecutive_errors < MAX_CONSECUTIVE_ERRORS / 2;
}

bool SeismicMonitor::force_calibration() {
    printf("[SeismicMonitor] Iniciando calibración forzada...\n");
    if (sensor->calibrate(cfg::CALIBRATION_SAMPLES)) {
        consecutive_errors = 0;
        return true;
    }
    return false;
}

void SeismicMonitor::reset_error_count() {
    consecutive_errors = 0;
    printf("[SeismicMonitor] Contador de errores reiniciado\n");
}

SensorData SeismicMonitor::get_current_sensor_data() const {
    if (get_buffer_count() == 0) {
        SensorData empty = {0};
        return empty;
    }
    
    int last_idx = (buffer_index - 1 + BUFFER_SIZE) % BUFFER_SIZE;
    return sensor_buffer[last_idx];
}

bool SeismicMonitor::is_sensor_ok() const {
    return sensor_initialized && consecutive_errors < MAX_CONSECUTIVE_ERRORS / 2;
}
    printf("\n===== Estado del Monitor Sísmico =====\n");
    printf("Sensor inicializado: %s\n", sensor_initialized ? "Sí" : "No");
    printf("Sensor OK: %s\n", is_sensor_ok() ? "Sí" : "No");
    printf("Errores consecutivos: %d/%d\n", consecutive_errors, MAX_CONSECUTIVE_ERRORS);
    printf("Muestras en buffer: %d/%d\n", get_buffer_count(), BUFFER_SIZE);
    printf("Magnitud actual: %.3f m/s²\n", get_current_magnitude());
    printf("Magnitud promedio (10 muestras): %.3f m/s²\n", calculate_average_magnitude(10));
    printf("=====================================\n\n");
}
