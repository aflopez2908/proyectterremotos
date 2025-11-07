#ifndef MPU6050_H_
#define MPU6050_H_

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include <cmath>

// Registros del MPU6050
#define MPU6050_PWR_MGMT_1    0x6B
#define MPU6050_PWR_MGMT_2    0x6C
#define MPU6050_ACCEL_CONFIG  0x1C
#define MPU6050_ACCEL_XOUT_H  0x3B
#define MPU6050_ACCEL_XOUT_L  0x3C
#define MPU6050_ACCEL_YOUT_H  0x3D
#define MPU6050_ACCEL_YOUT_L  0x3E
#define MPU6050_ACCEL_ZOUT_H  0x3F
#define MPU6050_ACCEL_ZOUT_L  0x40
#define MPU6050_GYRO_XOUT_H   0x43
#define MPU6050_GYRO_XOUT_L   0x44
#define MPU6050_GYRO_YOUT_H   0x45
#define MPU6050_GYRO_YOUT_L   0x46
#define MPU6050_GYRO_ZOUT_H   0x47
#define MPU6050_GYRO_ZOUT_L   0x48
#define MPU6050_WHO_AM_I      0x75

struct SensorData {
    float accel_x;    // m/s²
    float accel_y;    // m/s²
    float accel_z;    // m/s²
    float gyro_x;     // °/s
    float gyro_y;     // °/s
    float gyro_z;     // °/s
    float magnitude;  // magnitud vectorial de aceleración
    uint64_t timestamp; // timestamp en ms
};

class MPU6050 {
private:
    i2c_inst_t* i2c;
    uint8_t address;
    float accel_offset_x, accel_offset_y, accel_offset_z;
    
    // Escribir un registro
    int write_register(uint8_t reg, uint8_t value);
    
    // Leer un registro
    int read_register(uint8_t reg, uint8_t* value);
    
    // Leer múltiples registros
    int read_registers(uint8_t reg, uint8_t* buffer, size_t len);

public:
    MPU6050(i2c_inst_t* i2c_instance, uint8_t addr = 0x68);
    
    // Inicializar el sensor
    bool init();
    
    // Calibrar offsets (debe hacerse con el sensor en reposo)
    bool calibrate(int samples = 100);
    
    // Leer datos del sensor
    bool read_raw_data(int16_t* accel_x, int16_t* accel_y, int16_t* accel_z,
                       int16_t* gyro_x, int16_t* gyro_y, int16_t* gyro_z);
    
    // Leer datos procesados
    bool read_sensor_data(SensorData& data);
    
    // Verificar si hay movimiento significativo
    bool is_significant_movement(const SensorData& data, float threshold);
    
    // Obtener tipo de evento basado en la magnitud
    const char* get_event_type(float magnitude);
    
    // Test de conectividad
    bool test_connection();
};

#endif // MPU6050_H_
