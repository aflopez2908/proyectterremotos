const express = require('express');
const router = express.Router();
const axios = require('axios');
require('dotenv').config();

// Configuración del Pico
const PICO_IP = process.env.PICO_IP || '192.168.1.100'; // IP del Pico en la red local
const PICO_PORT = process.env.PICO_PORT || 80;

// Helper para hacer peticiones al Pico
async function sendToPico(endpoint, params = {}) {
  try {
    const url = `http://${PICO_IP}:${PICO_PORT}${endpoint}`;
    const queryString = new URLSearchParams(params).toString();
    const fullUrl = queryString ? `${url}?${queryString}` : url;
    
    console.log(`🔗 Enviando al Pico: ${fullUrl}`);
    
    const response = await axios.get(fullUrl, {
      timeout: 5000,
      headers: {
        'User-Agent': 'ExpressAPI/1.0'
      }
    });
    
    return {
      success: true,
      status: response.status,
      data: response.data
    };
  } catch (error) {
    console.error(`❌ Error comunicando con Pico:`, error.message);
    return {
      success: false,
      error: error.message,
      code: error.code
    };
  }
}

// Activar buzzer
router.post('/buzzer', async (req, res) => {
  try {
    console.log('🔔 Solicitud de activar buzzer recibida');
    
    const result = await sendToPico('/buzzer');
    
    if (result.success) {
      res.json({
        success: true,
        message: 'Buzzer activado correctamente',
        timestamp: new Date().toISOString(),
        pico_response: result.data
      });
    } else {
      res.status(500).json({
        success: false,
        error: 'No se pudo comunicar con el Pico',
        details: result.error,
        timestamp: new Date().toISOString()
      });
    }
  } catch (error) {
    console.error('Error en endpoint buzzer:', error);
    res.status(500).json({
      success: false,
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Enviar mensaje Morse
router.post('/morse', async (req, res) => {
  try {
    const { text } = req.body;
    
    if (!text || text.trim().length === 0) {
      return res.status(400).json({
        success: false,
        error: 'Se requiere el campo "text"',
        example: { text: "SOS" }
      });
    }
    
    if (text.length > 64) {
      return res.status(400).json({
        success: false,
        error: 'El texto no puede exceder 64 caracteres',
        received_length: text.length
      });
    }
    
    console.log(`📡 Solicitud de Morse recibida: "${text}"`);
    
    const result = await sendToPico('/morse', { text: text.trim() });
    
    if (result.success) {
      res.json({
        success: true,
        message: `Mensaje Morse enviado: "${text}"`,
        text: text.trim(),
        timestamp: new Date().toISOString(),
        pico_response: result.data
      });
    } else {
      res.status(500).json({
        success: false,
        error: 'No se pudo comunicar con el Pico',
        details: result.error,
        text: text.trim(),
        timestamp: new Date().toISOString()
      });
    }
  } catch (error) {
    console.error('Error en endpoint morse:', error);
    res.status(500).json({
      success: false,
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Obtener estado del Pico
router.get('/status', async (req, res) => {
  try {
    console.log('📊 Verificando estado del Pico');
    
    const result = await sendToPico('/');
    
    if (result.success) {
      res.json({
        success: true,
        message: 'Pico está en línea',
        pico_ip: PICO_IP,
        pico_port: PICO_PORT,
        timestamp: new Date().toISOString(),
        response_status: result.status
      });
    } else {
      res.status(503).json({
        success: false,
        error: 'Pico no está disponible',
        pico_ip: PICO_IP,
        pico_port: PICO_PORT,
        details: result.error,
        timestamp: new Date().toISOString()
      });
    }
  } catch (error) {
    console.error('Error verificando estado del Pico:', error);
    res.status(500).json({
      success: false,
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Endpoint para recibir datos del sensor MPU6050 desde el Pico
router.post('/sensor-data', async (req, res) => {
  try {
    const {
      device_id,
      acceleration_x,
      acceleration_y,
      acceleration_z,
      gyro_x,
      gyro_y,
      gyro_z,
      temperature,
      timestamp
    } = req.body;

    // Validar datos requeridos
    if (!device_id || acceleration_x === undefined || acceleration_y === undefined || acceleration_z === undefined) {
      return res.status(400).json({
        success: false,
        error: 'Datos incompletos del sensor',
        required: ['device_id', 'acceleration_x', 'acceleration_y', 'acceleration_z']
      });
    }

    // Calcular magnitud total de aceleración
    const total_acceleration = Math.sqrt(
      Math.pow(acceleration_x, 2) + 
      Math.pow(acceleration_y, 2) + 
      Math.pow(acceleration_z, 2)
    );

    console.log(`📊 Datos del sensor recibidos - Device: ${device_id}, Aceleración total: ${total_acceleration.toFixed(2)} m/s²`);

    // Determinar si es una detección sísmica (umbral de 10 m/s² como en el código Arduino)
    const isSeismicEvent = Math.abs(acceleration_x) > 10 || 
                          Math.abs(acceleration_y) > 10 || 
                          Math.abs(acceleration_z) > 10;

    if (isSeismicEvent) {
      console.log('🚨 ¡Evento sísmico detectado!');
      
      // Aquí podrías enviar a la ruta de earthquake event
      try {
        const earthquakeData = {
          device_id,
          magnitude: total_acceleration,
          acceleration_x,
          acceleration_y,
          acceleration_z,
          total_acceleration,
          timestamp: timestamp || new Date().toISOString()
        };

        // Simular envío al endpoint de earthquakes
        console.log('📡 Enviando datos sísmicos al sistema de análisis...');
        // await axios.post(`http://localhost:${process.env.PORT || 3000}/api/earthquakes/event`, earthquakeData);
      } catch (error) {
        console.error('Error enviando datos sísmicos:', error);
      }
    }

    res.json({
      success: true,
      message: 'Datos del sensor procesados correctamente',
      data: {
        device_id,
        acceleration: { x: acceleration_x, y: acceleration_y, z: acceleration_z },
        gyro: { x: gyro_x, y: gyro_y, z: gyro_z },
        temperature,
        total_acceleration: total_acceleration.toFixed(2),
        is_seismic_event: isSeismicEvent,
        timestamp: timestamp || new Date().toISOString()
      }
    });

  } catch (error) {
    console.error('Error procesando datos del sensor:', error);
    res.status(500).json({
      success: false,
      error: 'Error procesando datos del sensor',
      message: error.message
    });
  }
});

module.exports = router;
