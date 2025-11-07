#include "MPU6050.h"
#include "../Config.h"
#include <cstdio>

MPU6050::MPU6050(i2c_inst_t* i2c_instance, uint8_t addr) 
    : i2c(i2c_instance), address(addr), accel_offset_x(0), accel_offset_y(0), accel_offset_z(0) {
}

int MPU6050::write_register(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    return i2c_write_blocking(i2c, address, data, 2, false);
}

int MPU6050::read_register(uint8_t reg, uint8_t* value) {
    int ret = i2c_write_blocking(i2c, address, &reg, 1, true);
    if (ret < 0) return ret;
    return i2c_read_blocking(i2c, address, value, 1, false);
}

int MPU6050::read_registers(uint8_t reg, uint8_t* buffer, size_t len) {
    int ret = i2c_write_blocking(i2c, address, &reg, 1, true);
    if (ret < 0) return ret;
    return i2c_read_blocking(i2c, address, buffer, len, false);
}

bool MPU6050::init() {
    // Despertar el MPU6050 (por defecto está en sleep mode)
    if (write_register(MPU6050_PWR_MGMT_1, 0x00) < 0) {
        printf("[MPU6050] Error: No se pudo despertar el sensor\n");
        return false;
    }
    
    sleep_ms(100); // Esperar a que se estabilice
    
    // Configurar acelerómetro para rango ±2g
    if (write_register(MPU6050_ACCEL_CONFIG, 0x00) < 0) {
        printf("[MPU6050] Error: No se pudo configurar acelerómetro\n");
        return false;
    }
    
    // Verificar conexión
    if (!test_connection()) {
        printf("[MPU6050] Error: Falló la verificación de conexión\n");
        return false;
    }
    
    printf("[MPU6050] Inicializado correctamente\n");
    return true;
}

bool MPU6050::test_connection() {
    uint8_t who_am_i;
    if (read_register(MPU6050_WHO_AM_I, &who_am_i) < 0) {
        return false;
    }
    
    // El MPU6050 debería responder con 0x68 (o 0x69 si AD0 está alto)
    return (who_am_i == 0x68 || who_am_i == 0x69);
}

bool MPU6050::calibrate(int samples) {
    printf("[MPU6050] Iniciando calibración con %d muestras...\n", samples);
    
    float sum_x = 0, sum_y = 0, sum_z = 0;
    int valid_samples = 0;
    
    for (int i = 0; i < samples; i++) {
        int16_t ax, ay, az, gx, gy, gz;
        
        if (read_raw_data(&ax, &ay, &az, &gx, &gy, &gz)) {
            // Convertir a g's y luego a m/s²
            float accel_x = (float)ax / cfg::ACCEL_SCALE_FACTOR * cfg::GRAVITY;
            float accel_y = (float)ay / cfg::ACCEL_SCALE_FACTOR * cfg::GRAVITY;
            float accel_z = (float)az / cfg::ACCEL_SCALE_FACTOR * cfg::GRAVITY;
            
            sum_x += accel_x;
            sum_y += accel_y;
            sum_z += accel_z - cfg::GRAVITY; // Compensar gravedad en Z
            valid_samples++;
        }
        
        sleep_ms(10);
    }
    
    if (valid_samples < samples * 0.8) { // Al menos 80% de muestras válidas
        printf("[MPU6050] Error: No se obtuvieron suficientes muestras válidas\n");
        return false;
    }
    
    accel_offset_x = sum_x / valid_samples;
    accel_offset_y = sum_y / valid_samples;
    accel_offset_z = sum_z / valid_samples;
    
    printf("[MPU6050] Calibración completada. Offsets: X=%.3f, Y=%.3f, Z=%.3f m/s²\n",
           accel_offset_x, accel_offset_y, accel_offset_z);
    
    return true;
}

bool MPU6050::read_raw_data(int16_t* accel_x, int16_t* accel_y, int16_t* accel_z,
                           int16_t* gyro_x, int16_t* gyro_y, int16_t* gyro_z) {
    uint8_t buffer[14];
    
    // Leer todos los registros de una vez (acelerómetro + temperatura + giroscopio)
    if (read_registers(MPU6050_ACCEL_XOUT_H, buffer, 14) < 0) {
        return false;
    }
    
    // Combinar bytes high y low para cada eje
    *accel_x = (buffer[0] << 8) | buffer[1];
    *accel_y = (buffer[2] << 8) | buffer[3];
    *accel_z = (buffer[4] << 8) | buffer[5];
    // buffer[6] y buffer[7] son temperatura (no la usamos)
    *gyro_x = (buffer[8] << 8) | buffer[9];
    *gyro_y = (buffer[10] << 8) | buffer[11];
    *gyro_z = (buffer[12] << 8) | buffer[13];
    
    return true;
}

bool MPU6050::read_sensor_data(SensorData& data) {
    int16_t ax, ay, az, gx, gy, gz;
    
    if (!read_raw_data(&ax, &ay, &az, &gx, &gy, &gz)) {
        return false;
    }
    
    // Convertir acelerómetro a m/s² y aplicar calibración
    data.accel_x = ((float)ax / cfg::ACCEL_SCALE_FACTOR * cfg::GRAVITY) - accel_offset_x;
    data.accel_y = ((float)ay / cfg::ACCEL_SCALE_FACTOR * cfg::GRAVITY) - accel_offset_y;
    data.accel_z = ((float)az / cfg::ACCEL_SCALE_FACTOR * cfg::GRAVITY) - accel_offset_z;
    
    // Convertir giroscopio a °/s (escala por defecto ±250°/s)
    const float gyro_scale = 131.0f; // LSB/°/s para rango ±250°/s
    data.gyro_x = (float)gx / gyro_scale;
    data.gyro_y = (float)gy / gyro_scale;
    data.gyro_z = (float)gz / gyro_scale;
    
    // Calcular magnitud vectorial de aceleración
    data.magnitude = sqrt(data.accel_x * data.accel_x + 
                         data.accel_y * data.accel_y + 
                         data.accel_z * data.accel_z);
    
    // Timestamp
    data.timestamp = to_ms_since_boot(get_absolute_time());
    
    return true;
}

bool MPU6050::is_significant_movement(const SensorData& data, float threshold) {
    return data.magnitude > threshold;
}

const char* MPU6050::get_event_type(float magnitude) {
    if (magnitude >= cfg::EARTHQUAKE_THRESHOLD) {
        return "earthquake";
    } else if (magnitude >= cfg::VIBRATION_THRESHOLD) {
        return "vibration";
    } else {
        return "normal";
    }
}
