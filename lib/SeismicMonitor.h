#ifndef SEISMIC_MONITOR_H_
#define SEISMIC_MONITOR_H_

#include "MPU6050.h"
#include "Esp8266HttpServer.h"
#include "../Config.h"
#include <queue>

struct SeismicEvent {
    SensorData data;
    bool is_significant;
    const char* event_type;
    uint64_t detected_at;
};

class SeismicMonitor {
private:
    MPU6050* sensor;
    Esp8266HttpServer* server;
    
    // Buffer circular para datos del sensor
    static const int BUFFER_SIZE = 50;
    SensorData sensor_buffer[BUFFER_SIZE];
    int buffer_index;
    bool buffer_full;
    
    // Timing
    uint64_t last_sensor_read;
    uint64_t last_api_send;
    uint64_t last_status_send;
    
    // Estado
    bool sensor_initialized;
    int consecutive_errors;
    static const int MAX_CONSECUTIVE_ERRORS = 10;
    
    // Métodos privados
    bool send_sensor_data_to_api(const SeismicEvent& event);
    bool send_status_to_api();
    void add_to_buffer(const SensorData& data);
    float calculate_average_magnitude(int samples = 10);
    bool is_wifi_connected();
    
    // Formatear datos para JSON
    void format_sensor_data_json(const SeismicEvent& event, char* json_buffer, size_t buffer_size);

public:
    SeismicMonitor(MPU6050* mpu_sensor, Esp8266HttpServer* http_server);
    
    // Inicializar monitor sísmico
    bool init();
    
    // Loop principal - debe llamarse continuamente
    void loop();
    
    // Obtener estadísticas
    int get_buffer_count() const;
    float get_current_magnitude();
    bool is_sensor_ok() const;
    
    // Métodos de control manual
    bool force_calibration();
    void reset_error_count();
    
    // Debug
    void print_sensor_status();
};

#endif // SEISMIC_MONITOR_H_
