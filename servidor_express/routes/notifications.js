const express = require('express');
const router = express.Router();
const { getDatabase } = require('../database/init');
const { sendWhatsAppNotification } = require('../services/notifications');

// Enviar notificación manual
router.post('/send', async (req, res) => {
  try {
    const { event_id, phone_number, message, notification_type = 'whatsapp' } = req.body;

    if (!event_id || !phone_number || !message) {
      return res.status(400).json({
        error: 'Datos incompletos',
        required: ['event_id', 'phone_number', 'message']
      });
    }

    const db = getDatabase();

    // Verificar que el evento existe
    const event = await new Promise((resolve, reject) => {
      db.get('SELECT * FROM earthquake_events WHERE id = ?', [event_id], (err, row) => {
        if (err) reject(err);
        else resolve(row);
      });
    });

    if (!event) {
      return res.status(404).json({
        error: 'Evento no encontrado',
        message: `No existe un evento con ID ${event_id}`
      });
    }

    // Registrar la notificación
    const notificationId = await new Promise((resolve, reject) => {
      db.run(`
        INSERT INTO notifications (event_id, notification_type, recipient, message, status)
        VALUES (?, ?, ?, ?, 'pending')
      `, [event_id, notification_type, phone_number, message], function(err) {
        if (err) reject(err);
        else resolve(this.lastID);
      });
    });

    // Enviar la notificación según el tipo
    let result;
    if (notification_type === 'whatsapp') {
      result = await sendWhatsAppMessage(phone_number, message);
    } else {
      throw new Error(`Tipo de notificación no soportado: ${notification_type}`);
    }

    // Actualizar estado de la notificación
    await new Promise((resolve, reject) => {
      db.run(`
        UPDATE notifications 
        SET status = ?, sent_at = ?, error_message = ?
        WHERE id = ?
      `, [
        result.success ? 'sent' : 'failed',
        result.success ? new Date().toISOString() : null,
        result.error || null,
        notificationId
      ], (err) => {
        if (err) reject(err);
        else resolve();
      });
    });

    res.json({
      success: result.success,
      notification_id: notificationId,
      message: result.success ? 'Notificación enviada exitosamente' : 'Error enviando notificación',
      error: result.error
    });

  } catch (error) {
    console.error('Error enviando notificación:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Obtener historial de notificaciones
router.get('/history', async (req, res) => {
  try {
    const { limit = 50, offset = 0, status, notification_type } = req.query;
    const db = getDatabase();

    let query = `
      SELECT n.*, e.event_type, e.magnitude, e.total_acceleration, e.timestamp as event_timestamp
      FROM notifications n
      JOIN earthquake_events e ON n.event_id = e.id
      WHERE 1=1
    `;
    let params = [];

    if (status) {
      query += ' AND n.status = ?';
      params.push(status);
    }

    if (notification_type) {
      query += ' AND n.notification_type = ?';
      params.push(notification_type);
    }

    query += ' ORDER BY n.created_at DESC LIMIT ? OFFSET ?';
    params.push(parseInt(limit), parseInt(offset));

    const notifications = await new Promise((resolve, reject) => {
      db.all(query, params, (err, rows) => {
        if (err) reject(err);
        else resolve(rows);
      });
    });

    res.json({
      success: true,
      notifications,
      pagination: {
        limit: parseInt(limit),
        offset: parseInt(offset),
        total: notifications.length
      }
    });

  } catch (error) {
    console.error('Error obteniendo historial de notificaciones:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Estadísticas de notificaciones
router.get('/stats', async (req, res) => {
  try {
    const { days = 30 } = req.query;
    const db = getDatabase();

    const stats = await new Promise((resolve, reject) => {
      db.all(`
        SELECT 
          notification_type,
          status,
          COUNT(*) as count,
          DATE(created_at) as date
        FROM notifications 
        WHERE created_at >= datetime('now', '-${parseInt(days)} days')
        GROUP BY notification_type, status, DATE(created_at)
        ORDER BY date DESC
      `, (err, rows) => {
        if (err) reject(err);
        else resolve(rows);
      });
    });

    const summary = await new Promise((resolve, reject) => {
      db.all(`
        SELECT 
          notification_type,
          status,
          COUNT(*) as total
        FROM notifications 
        WHERE created_at >= datetime('now', '-${parseInt(days)} days')
        GROUP BY notification_type, status
      `, (err, rows) => {
        if (err) reject(err);
        else resolve(rows);
      });
    });

    res.json({
      success: true,
      period_days: parseInt(days),
      summary,
      daily_stats: stats,
      generated_at: new Date().toISOString()
    });

  } catch (error) {
    console.error('Error obteniendo estadísticas de notificaciones:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Configurar contactos de emergencia
router.post('/contacts', async (req, res) => {
  try {
    const { contacts } = req.body;

    if (!Array.isArray(contacts)) {
      return res.status(400).json({
        error: 'Formato inválido',
        message: 'contacts debe ser un array de objetos con name y phone'
      });
    }

    // Validar formato de contactos
    for (const contact of contacts) {
      if (!contact.name || !contact.phone) {
        return res.status(400).json({
          error: 'Datos incompletos en contacto',
          message: 'Cada contacto debe tener name y phone'
        });
      }
    }

    const db = getDatabase();

    // Actualizar configuración de contactos
    await new Promise((resolve, reject) => {
      db.run(`
        INSERT OR REPLACE INTO system_config (config_key, config_value, description)
        VALUES ('emergency_contacts', ?, 'Lista de contactos de emergencia en formato JSON')
      `, [JSON.stringify(contacts)], (err) => {
        if (err) reject(err);
        else resolve();
      });
    });

    res.json({
      success: true,
      message: 'Contactos de emergencia actualizados',
      contacts
    });

  } catch (error) {
    console.error('Error configurando contactos:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Función auxiliar para enviar WhatsApp (simulada)
async function sendWhatsAppMessage(phoneNumber, message) {
  try {
    // Aquí iría la integración real con WhatsApp API
    console.log(`📱 WhatsApp enviado a ${phoneNumber}: ${message}`);
    
    // Simular envío exitoso
    return {
      success: true,
      message_id: `wa_${Date.now()}`,
      timestamp: new Date().toISOString()
    };
  } catch (error) {
    return {
      success: false,
      error: error.message
    };
  }
}

module.exports = router;
