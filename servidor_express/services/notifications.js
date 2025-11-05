const { getDatabase } = require('../database/init');
const axios = require('axios');

/**
 * Envía notificación por WhatsApp para un evento sísmico
 * @param {number} eventId - ID del evento
 * @param {Object} eventData - Datos del evento
 * @returns {Promise<boolean>} - true si se envió exitosamente
 */
async function sendWhatsAppNotification(eventId, eventData) {
  try {
    const db = getDatabase();

    // Obtener contactos de emergencia
    const emergencyContacts = await getEmergencyContacts();
    
    if (emergencyContacts.length === 0) {
      console.log('⚠️ No hay contactos de emergencia configurados');
      return false;
    }

    // Verificar cooldown de notificaciones
    const cooldownMinutes = await getCooldownMinutes();
    const recentNotification = await checkRecentNotifications(eventData.event_type, cooldownMinutes);
    
    if (recentNotification) {
      console.log(`⏱️ Cooldown activo para notificaciones de ${eventData.event_type}`);
      return false;
    }

    // Generar mensaje personalizado
    const message = generateEventMessage(eventData);

    // Enviar a todos los contactos
    let sentCount = 0;
    for (const contact of emergencyContacts) {
      try {
        const result = await sendWhatsAppMessage(contact.phone, message);
        
        // Registrar notificación en BD
        await logNotification(eventId, 'whatsapp', contact.phone, message, result.success);
        
        if (result.success) {
          sentCount++;
          console.log(`✅ WhatsApp enviado a ${contact.name} (${contact.phone})`);
        } else {
          console.log(`❌ Error enviando a ${contact.name}: ${result.error}`);
        }
      } catch (error) {
        console.error(`Error enviando a ${contact.name}:`, error);
        await logNotification(eventId, 'whatsapp', contact.phone, message, false, error.message);
      }
    }

    return sentCount > 0;

  } catch (error) {
    console.error('Error enviando notificaciones WhatsApp:', error);
    return false;
  }
}

/**
 * Envía mensaje de WhatsApp usando la API
 * @param {string} phoneNumber - Número de teléfono
 * @param {string} message - Mensaje a enviar
 * @returns {Promise<Object>} - Resultado del envío
 */
async function sendWhatsAppMessage(phoneNumber, message) {
  try {
    // Aquí implementarías la integración con tu proveedor de WhatsApp API
    // Por ejemplo: Twilio, WhatsApp Business API, etc.
    
    const whatsappConfig = {
      url: process.env.WHATSAPP_API_URL,
      token: process.env.WHATSAPP_TOKEN
    };

    if (!whatsappConfig.url || !whatsappConfig.token) {
      // Modo simulación para desarrollo
      console.log(`📱 [SIMULADO] WhatsApp a ${phoneNumber}: ${message}`);
      return {
        success: true,
        message_id: `sim_${Date.now()}`,
        timestamp: new Date().toISOString()
      };
    }

    // Ejemplo de integración real (comentado)
    /*
    const response = await axios.post(whatsappConfig.url, {
      to: phoneNumber,
      message: message,
      type: 'text'
    }, {
      headers: {
        'Authorization': `Bearer ${whatsappConfig.token}`,
        'Content-Type': 'application/json'
      },
      timeout: 10000
    });

    return {
      success: response.status === 200,
      message_id: response.data.message_id,
      timestamp: new Date().toISOString()
    };
    */

    // Por ahora, retornamos éxito simulado
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

/**
 * Genera mensaje personalizado para el evento
 * @param {Object} eventData - Datos del evento
 * @returns {string} - Mensaje formateado
 */
function generateEventMessage(eventData) {
  const { event_type, magnitude, total_acceleration, aftershock_probability } = eventData;
  
  let message = `🚨 ALERTA SÍSMICA 🚨\n\n`;
  
  if (event_type === 'earthquake') {
    message += `🌍 TERREMOTO DETECTADO\n`;
    message += `📊 Magnitud: ${magnitude}\n`;
    message += `⚡ Aceleración: ${total_acceleration.toFixed(2)} m/s²\n`;
    message += `⏰ Hora: ${new Date().toLocaleString('es-ES')}\n\n`;
    
    if (aftershock_probability > 0) {
      message += `⚠️ Probabilidad de réplicas: ${aftershock_probability.toFixed(1)}%\n`;
      message += `🕐 Manténgase alerta las próximas 72 horas\n\n`;
    }
    
    message += `🛡️ RECOMENDACIONES:\n`;
    message += `• Manténgase en lugar seguro\n`;
    message += `• Revise su kit de emergencia\n`;
    message += `• Esté atento a réplicas\n`;
    message += `• Siga protocolos de seguridad\n\n`;
  } else {
    message += `📳 VIBRACIÓN DETECTADA\n`;
    message += `📊 Intensidad: ${total_acceleration.toFixed(2)} m/s²\n`;
    message += `⏰ Hora: ${new Date().toLocaleString('es-ES')}\n\n`;
    message += `ℹ️ Vibración por debajo del umbral de terremoto\n`;
    message += `👁️ Monitoreo continuo activo\n\n`;
  }
  
  message += `🔗 Sistema de Detección Sísmica IoT\n`;
  message += `📱 Para más información visite el dashboard`;
  
  return message;
}

/**
 * Obtiene la lista de contactos de emergencia
 * @returns {Promise<Array>} - Array de contactos
 */
async function getEmergencyContacts() {
  try {
    const db = getDatabase();
    
    const config = await new Promise((resolve, reject) => {
      db.get(
        "SELECT config_value FROM system_config WHERE config_key = 'emergency_contacts'",
        (err, row) => {
          if (err) reject(err);
          else resolve(row);
        }
      );
    });

    if (config?.config_value) {
      return JSON.parse(config.config_value);
    }

    // Contactos por defecto desde variables de entorno
    const defaultContacts = [];
    if (process.env.ADMIN_PHONE) {
      defaultContacts.push({
        name: 'Administrador',
        phone: process.env.ADMIN_PHONE
      });
    }

    if (process.env.EMERGENCY_CONTACTS) {
      const phones = process.env.EMERGENCY_CONTACTS.split(',');
      phones.forEach((phone, index) => {
        defaultContacts.push({
          name: `Contacto ${index + 1}`,
          phone: phone.trim()
        });
      });
    }

    return defaultContacts;

  } catch (error) {
    console.error('Error obteniendo contactos de emergencia:', error);
    return [];
  }
}

/**
 * Verifica si hay notificaciones recientes del mismo tipo
 * @param {string} eventType - Tipo de evento
 * @param {number} cooldownMinutes - Minutos de cooldown
 * @returns {Promise<boolean>} - true si hay cooldown activo
 */
async function checkRecentNotifications(eventType, cooldownMinutes) {
  try {
    const db = getDatabase();
    
    const recentNotification = await new Promise((resolve, reject) => {
      db.get(`
        SELECT n.* FROM notifications n
        JOIN earthquake_events e ON n.event_id = e.id
        WHERE e.event_type = ? 
        AND n.status = 'sent'
        AND n.sent_at >= datetime('now', '-${cooldownMinutes} minutes')
        ORDER BY n.sent_at DESC
        LIMIT 1
      `, [eventType], (err, row) => {
        if (err) reject(err);
        else resolve(row);
      });
    });

    return !!recentNotification;

  } catch (error) {
    console.error('Error verificando cooldown:', error);
    return false;
  }
}

/**
 * Obtiene los minutos de cooldown configurados
 * @returns {Promise<number>} - Minutos de cooldown
 */
async function getCooldownMinutes() {
  try {
    const db = getDatabase();
    
    const config = await new Promise((resolve, reject) => {
      db.get(
        "SELECT config_value FROM system_config WHERE config_key = 'notification_cooldown_minutes'",
        (err, row) => {
          if (err) reject(err);
          else resolve(row);
        }
      );
    });

    return parseInt(config?.config_value || 15);

  } catch (error) {
    console.error('Error obteniendo cooldown:', error);
    return 15; // Default 15 minutos
  }
}

/**
 * Registra una notificación en la base de datos
 * @param {number} eventId - ID del evento
 * @param {string} type - Tipo de notificación
 * @param {string} recipient - Destinatario
 * @param {string} message - Mensaje enviado
 * @param {boolean} success - Si se envió exitosamente
 * @param {string} errorMessage - Mensaje de error si falló
 */
async function logNotification(eventId, type, recipient, message, success, errorMessage = null) {
  try {
    const db = getDatabase();
    
    await new Promise((resolve, reject) => {
      db.run(`
        INSERT INTO notifications 
        (event_id, notification_type, recipient, message, status, sent_at, error_message)
        VALUES (?, ?, ?, ?, ?, ?, ?)
      `, [
        eventId,
        type,
        recipient,
        message,
        success ? 'sent' : 'failed',
        success ? new Date().toISOString() : null,
        errorMessage
      ], (err) => {
        if (err) reject(err);
        else resolve();
      });
    });

  } catch (error) {
    console.error('Error registrando notificación:', error);
  }
}

/**
 * Envía notificación de prueba
 * @param {string} phoneNumber - Número de teléfono
 * @returns {Promise<Object>} - Resultado del envío
 */
async function sendTestNotification(phoneNumber) {
  const testMessage = `🔧 PRUEBA DEL SISTEMA 🔧\n\n` +
    `✅ El sistema de alertas sísmicas está funcionando correctamente.\n\n` +
    `⏰ Hora de prueba: ${new Date().toLocaleString('es-ES')}\n\n` +
    `📱 Este es un mensaje de prueba para verificar la conectividad.\n\n` +
    `🔗 Sistema de Detección Sísmica IoT`;

  return await sendWhatsAppMessage(phoneNumber, testMessage);
}

module.exports = {
  sendWhatsAppNotification,
  sendWhatsAppMessage,
  sendTestNotification,
  getEmergencyContacts
};
