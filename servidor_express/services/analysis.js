const { getDatabase } = require('../database/init');

/**
 * Analiza la probabilidad de réplicas después de un terremoto
 * @param {number} mainEventId - ID del evento principal
 * @returns {Promise<number>} - Probabilidad de réplicas (0-100)
 */
async function analyzeAftershockProbability(mainEventId) {
  try {
    const db = getDatabase();

    // Obtener el evento principal
    const mainEvent = await new Promise((resolve, reject) => {
      db.get('SELECT * FROM earthquake_events WHERE id = ?', [mainEventId], (err, row) => {
        if (err) reject(err);
        else resolve(row);
      });
    });

    if (!mainEvent || mainEvent.event_type !== 'earthquake') {
      return 0;
    }

    // Obtener eventos históricos similares
    const historicalEvents = await new Promise((resolve, reject) => {
      db.all(`
        SELECT * FROM earthquake_events 
        WHERE event_type = 'earthquake' 
        AND total_acceleration BETWEEN ? AND ?
        AND timestamp < ?
        ORDER BY timestamp DESC
        LIMIT 20
      `, [
        mainEvent.total_acceleration * 0.8,
        mainEvent.total_acceleration * 1.2,
        mainEvent.timestamp
      ], (err, rows) => {
        if (err) reject(err);
        else resolve(rows);
      });
    });

    // Calcular probabilidad basada en múltiples factores
    let probability = 0;
    const factors = [];

    // Factor 1: Magnitud del evento principal
    if (mainEvent.total_acceleration > 20) {
      probability += 40;
      factors.push('Alta magnitud del evento principal');
    } else if (mainEvent.total_acceleration > 15) {
      probability += 25;
      factors.push('Magnitud moderada del evento principal');
    } else {
      probability += 10;
      factors.push('Magnitud baja del evento principal');
    }

    // Factor 2: Actividad sísmica reciente
    const recentEvents = await new Promise((resolve, reject) => {
      db.all(`
        SELECT COUNT(*) as count FROM earthquake_events 
        WHERE event_type = 'earthquake' 
        AND timestamp >= datetime(?, '-7 days')
        AND timestamp < ?
      `, [mainEvent.timestamp, mainEvent.timestamp], (err, row) => {
        if (err) reject(err);
        else resolve(row[0].count);
      });
    });

    if (recentEvents > 2) {
      probability += 30;
      factors.push(`Actividad sísmica reciente alta: ${recentEvents} eventos`);
    } else if (recentEvents > 0) {
      probability += 15;
      factors.push(`Actividad sísmica reciente moderada: ${recentEvents} eventos`);
    }

    // Factor 3: Patrón histórico (análisis simplificado)
    if (historicalEvents.length > 0) {
      const avgInterval = calculateAverageInterval(historicalEvents);
      if (avgInterval < 30) { // días
        probability += 20;
        factors.push('Patrón histórico indica alta frecuencia');
      } else if (avgInterval < 90) {
        probability += 10;
        factors.push('Patrón histórico indica frecuencia moderada');
      }
    }

    // Factor 4: Tiempo transcurrido (las réplicas son más probables inmediatamente después)
    const hoursElapsed = (new Date() - new Date(mainEvent.timestamp)) / (1000 * 60 * 60);
    if (hoursElapsed < 24) {
      probability += 15;
      factors.push('Evento muy reciente (< 24 horas)');
    } else if (hoursElapsed < 72) {
      probability += 8;
      factors.push('Evento reciente (< 72 horas)');
    }

    // Limitar probabilidad al 100%
    probability = Math.min(probability, 100);

    // Calcular tiempo de expiración (72 horas después del evento principal)
    const expiresAt = new Date(mainEvent.timestamp);
    expiresAt.setHours(expiresAt.getHours() + 72);

    // Guardar análisis en la base de datos
    await new Promise((resolve, reject) => {
      db.run(`
        INSERT INTO aftershock_analysis 
        (main_event_id, probability_percentage, factors, expires_at)
        VALUES (?, ?, ?, ?)
      `, [
        mainEventId,
        probability,
        JSON.stringify(factors),
        expiresAt.toISOString()
      ], (err) => {
        if (err) reject(err);
        else resolve();
      });
    });

    console.log(`🔍 Análisis de réplicas para evento ${mainEventId}: ${probability}% probabilidad`);

    return probability;

  } catch (error) {
    console.error('Error analizando probabilidad de réplicas:', error);
    return 0;
  }
}

/**
 * Calcula el intervalo promedio entre eventos históricos
 * @param {Array} events - Array de eventos históricos
 * @returns {number} - Intervalo promedio en días
 */
function calculateAverageInterval(events) {
  if (events.length < 2) return Infinity;

  let totalInterval = 0;
  for (let i = 0; i < events.length - 1; i++) {
    const date1 = new Date(events[i].timestamp);
    const date2 = new Date(events[i + 1].timestamp);
    const interval = Math.abs(date1 - date2) / (1000 * 60 * 60 * 24); // días
    totalInterval += interval;
  }

  return totalInterval / (events.length - 1);
}

/**
 * Analiza tendencias de actividad sísmica
 * @param {number} days - Número de días para analizar
 * @returns {Promise<Object>} - Objeto con análisis de tendencias
 */
async function analyzeTrends(days = 30) {
  try {
    const db = getDatabase();

    // Obtener eventos del período
    const events = await new Promise((resolve, reject) => {
      db.all(`
        SELECT * FROM earthquake_events 
        WHERE timestamp >= datetime('now', '-${days} days')
        ORDER BY timestamp DESC
      `, (err, rows) => {
        if (err) reject(err);
        else resolve(rows);
      });
    });

    const earthquakes = events.filter(e => e.event_type === 'earthquake');
    const vibrations = events.filter(e => e.event_type === 'vibration');

    // Calcular tendencias
    const analysis = {
      period_days: days,
      total_events: events.length,
      earthquakes_count: earthquakes.length,
      vibrations_count: vibrations.length,
      earthquake_frequency: earthquakes.length / days,
      vibration_frequency: vibrations.length / days,
      average_earthquake_intensity: earthquakes.length > 0 
        ? earthquakes.reduce((sum, e) => sum + e.total_acceleration, 0) / earthquakes.length 
        : 0,
      trend_direction: calculateTrendDirection(events),
      risk_assessment: assessRiskLevel(earthquakes, vibrations, days)
    };

    return analysis;

  } catch (error) {
    console.error('Error analizando tendencias:', error);
    throw error;
  }
}

/**
 * Calcula la dirección de la tendencia (ascendente, descendente, estable)
 */
function calculateTrendDirection(events) {
  if (events.length < 10) return 'insufficient_data';

  const midpoint = Math.floor(events.length / 2);
  const firstHalf = events.slice(0, midpoint);
  const secondHalf = events.slice(midpoint);

  const firstHalfAvg = firstHalf.reduce((sum, e) => sum + e.total_acceleration, 0) / firstHalf.length;
  const secondHalfAvg = secondHalf.reduce((sum, e) => sum + e.total_acceleration, 0) / secondHalf.length;

  const difference = secondHalfAvg - firstHalfAvg;

  if (Math.abs(difference) < 1) return 'stable';
  return difference > 0 ? 'increasing' : 'decreasing';
}

/**
 * Evalúa el nivel de riesgo basado en la actividad reciente
 */
function assessRiskLevel(earthquakes, vibrations, days) {
  const earthquakeRate = earthquakes.length / days;
  const vibrationRate = vibrations.length / days;
  
  const highMagnitudeEvents = earthquakes.filter(e => e.total_acceleration > 20).length;

  if (earthquakeRate > 0.3 || highMagnitudeEvents > 0) {
    return 'high';
  } else if (earthquakeRate > 0.1 || vibrationRate > 2) {
    return 'medium';
  } else {
    return 'low';
  }
}

module.exports = {
  analyzeAftershockProbability,
  analyzeTrends
};
