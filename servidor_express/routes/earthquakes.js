const express = require('express');
const router = express.Router();
const { getDatabase } = require('../database/init');
const { analyzeAftershockProbability } = require('../services/analysis');
const { sendWhatsAppNotification } = require('../services/notifications');

// Recibir datos de evento sísmico desde el Pico
router.post('/event', async (req, res) => {
  try {
    const {
      device_id,
      magnitude,
      acceleration_x,
      acceleration_y,
      acceleration_z,
      total_acceleration,
      timestamp
    } = req.body;

    // Validar datos requeridos
    if (!device_id || !magnitude || !total_acceleration) {
      return res.status(400).json({
        error: 'Datos incompletos',
        required: ['device_id', 'magnitude', 'total_acceleration']
      });
    }

    const db = getDatabase();
    const thresholds = await getThresholds();
    
    // Determinar tipo de evento
    const event_type = total_acceleration >= thresholds.earthquake ? 'earthquake' : 'vibration';
    
    // Insertar evento en la base de datos
    const stmt = db.prepare(`
      INSERT INTO earthquake_events 
      (device_id, magnitude, acceleration_x, acceleration_y, acceleration_z, total_acceleration, event_type, timestamp)
      VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    `);

    const result = await new Promise((resolve, reject) => {
      stmt.run([
        device_id, magnitude, acceleration_x, acceleration_y, acceleration_z, 
        total_acceleration, event_type, timestamp || new Date().toISOString()
      ], function(err) {
        if (err) reject(err);
        else resolve({ id: this.lastID });
      });
    });

    console.log(`📊 Nuevo evento ${event_type}: ID ${result.id}, Magnitud: ${magnitude}`);

    // Si es un terremoto, realizar análisis y enviar notificaciones
    if (event_type === 'earthquake') {
      // Análisis de probabilidad de réplicas
      const aftershockProbability = await analyzeAftershockProbability(result.id);
      
      // Enviar notificación por WhatsApp
      await sendWhatsAppNotification(result.id, {
        event_type,
        magnitude,
        total_acceleration,
        aftershock_probability: aftershockProbability
      });
    }

    res.status(201).json({
      success: true,
      event_id: result.id,
      event_type,
      magnitude,
      total_acceleration,
      message: `Evento ${event_type} registrado exitosamente`
    });

  } catch (error) {
    console.error('Error procesando evento sísmico:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Obtener todos los eventos
router.get('/', async (req, res) => {
  try {
    const { limit = 50, offset = 0, event_type, device_id } = req.query;
    const db = getDatabase();

    let query = 'SELECT * FROM earthquake_events WHERE 1=1';
    let params = [];

    if (event_type) {
      query += ' AND event_type = ?';
      params.push(event_type);
    }

    if (device_id) {
      query += ' AND device_id = ?';
      params.push(device_id);
    }

    query += ' ORDER BY timestamp DESC LIMIT ? OFFSET ?';
    params.push(parseInt(limit), parseInt(offset));

    const events = await new Promise((resolve, reject) => {
      db.all(query, params, (err, rows) => {
        if (err) reject(err);
        else resolve(rows);
      });
    });

    res.json({
      success: true,
      events,
      pagination: {
        limit: parseInt(limit),
        offset: parseInt(offset),
        total: events.length
      }
    });

  } catch (error) {
    console.error('Error obteniendo eventos:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Obtener evento específico
router.get('/:id', async (req, res) => {
  try {
    const { id } = req.params;
    const db = getDatabase();

    const event = await new Promise((resolve, reject) => {
      db.get('SELECT * FROM earthquake_events WHERE id = ?', [id], (err, row) => {
        if (err) reject(err);
        else resolve(row);
      });
    });

    if (!event) {
      return res.status(404).json({
        error: 'Evento no encontrado',
        message: `No existe un evento con ID ${id}`
      });
    }

    // Obtener análisis de réplicas si existe
    const aftershockAnalysis = await new Promise((resolve, reject) => {
      db.get(
        'SELECT * FROM aftershock_analysis WHERE main_event_id = ? ORDER BY analysis_timestamp DESC LIMIT 1',
        [id],
        (err, row) => {
          if (err) reject(err);
          else resolve(row);
        }
      );
    });

    res.json({
      success: true,
      event,
      aftershock_analysis: aftershockAnalysis
    });

  } catch (error) {
    console.error('Error obteniendo evento:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Función auxiliar para obtener umbrales
async function getThresholds() {
  const db = getDatabase();
  
  const earthquake = await new Promise((resolve, reject) => {
    db.get("SELECT config_value FROM system_config WHERE config_key = 'earthquake_threshold'", (err, row) => {
      if (err) reject(err);
      else resolve(parseFloat(row?.config_value || 15.0));
    });
  });

  const vibration = await new Promise((resolve, reject) => {
    db.get("SELECT config_value FROM system_config WHERE config_key = 'vibration_threshold'", (err, row) => {
      if (err) reject(err);
      else resolve(parseFloat(row?.config_value || 5.0));
    });
  });

  return { earthquake, vibration };
}

module.exports = router;
