const express = require('express');
const router = express.Router();
const { getDatabase } = require('../database/init');

// Análisis de probabilidad de réplicas
router.get('/aftershocks/:eventId', async (req, res) => {
  try {
    const { eventId } = req.params;
    const db = getDatabase();

    // Obtener evento principal
    const mainEvent = await new Promise((resolve, reject) => {
      db.get('SELECT * FROM earthquake_events WHERE id = ?', [eventId], (err, row) => {
        if (err) reject(err);
        else resolve(row);
      });
    });

    if (!mainEvent) {
      return res.status(404).json({
        error: 'Evento no encontrado',
        message: `No existe un evento con ID ${eventId}`
      });
    }

    // Obtener análisis existente
    const analysis = await new Promise((resolve, reject) => {
      db.get(
        'SELECT * FROM aftershock_analysis WHERE main_event_id = ? ORDER BY analysis_timestamp DESC LIMIT 1',
        [eventId],
        (err, row) => {
          if (err) reject(err);
          else resolve(row);
        }
      );
    });

    res.json({
      success: true,
      main_event: mainEvent,
      aftershock_analysis: analysis,
      message: analysis ? 'Análisis encontrado' : 'No hay análisis disponible para este evento'
    });

  } catch (error) {
    console.error('Error obteniendo análisis de réplicas:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Estadísticas generales
router.get('/stats/general', async (req, res) => {
  try {
    const db = getDatabase();
    const { days = 30 } = req.query;

    // Estadísticas de eventos en los últimos N días
    const stats = await new Promise((resolve, reject) => {
      db.all(`
        SELECT 
          event_type,
          COUNT(*) as count,
          AVG(total_acceleration) as avg_acceleration,
          MAX(total_acceleration) as max_acceleration,
          DATE(timestamp) as date
        FROM earthquake_events 
        WHERE timestamp >= datetime('now', '-${parseInt(days)} days')
        GROUP BY event_type, DATE(timestamp)
        ORDER BY date DESC
      `, (err, rows) => {
        if (err) reject(err);
        else resolve(rows);
      });
    });

    // Resumen total
    const summary = await new Promise((resolve, reject) => {
      db.all(`
        SELECT 
          event_type,
          COUNT(*) as total_events,
          AVG(total_acceleration) as avg_acceleration,
          MAX(total_acceleration) as max_acceleration,
          MIN(total_acceleration) as min_acceleration
        FROM earthquake_events 
        WHERE timestamp >= datetime('now', '-${parseInt(days)} days')
        GROUP BY event_type
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
    console.error('Error obteniendo estadísticas:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Tendencias de actividad sísmica
router.get('/trends/activity', async (req, res) => {
  try {
    const db = getDatabase();
    const { hours = 24 } = req.query;

    const trends = await new Promise((resolve, reject) => {
      db.all(`
        SELECT 
          strftime('%Y-%m-%d %H:00:00', timestamp) as hour_group,
          event_type,
          COUNT(*) as event_count,
          AVG(total_acceleration) as avg_intensity
        FROM earthquake_events 
        WHERE timestamp >= datetime('now', '-${parseInt(hours)} hours')
        GROUP BY hour_group, event_type
        ORDER BY hour_group DESC
      `, (err, rows) => {
        if (err) reject(err);
        else resolve(rows);
      });
    });

    res.json({
      success: true,
      period_hours: parseInt(hours),
      trends,
      generated_at: new Date().toISOString()
    });

  } catch (error) {
    console.error('Error obteniendo tendencias:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

// Predicción simple de actividad
router.get('/prediction/simple', async (req, res) => {
  try {
    const db = getDatabase();

    // Obtener eventos recientes para análisis
    const recentEvents = await new Promise((resolve, reject) => {
      db.all(`
        SELECT * FROM earthquake_events 
        WHERE timestamp >= datetime('now', '-7 days')
        ORDER BY timestamp DESC
      `, (err, rows) => {
        if (err) reject(err);
        else resolve(rows);
      });
    });

    // Análisis simple basado en patrones
    const earthquakes = recentEvents.filter(e => e.event_type === 'earthquake');
    const vibrations = recentEvents.filter(e => e.event_type === 'vibration');

    let riskLevel = 'low';
    let probability = 0;
    let factors = [];

    // Factores que aumentan el riesgo
    if (earthquakes.length > 0) {
      const lastEarthquake = earthquakes[0];
      const hoursSinceLastEarthquake = (new Date() - new Date(lastEarthquake.timestamp)) / (1000 * 60 * 60);
      
      if (hoursSinceLastEarthquake < 72) {
        probability += 30;
        factors.push(`Terremoto reciente hace ${hoursSinceLastEarthquake.toFixed(1)} horas`);
        riskLevel = 'medium';
      }
    }

    if (vibrations.length > 10) {
      probability += 20;
      factors.push(`Alta actividad de vibraciones: ${vibrations.length} eventos en 7 días`);
      if (riskLevel === 'low') riskLevel = 'medium';
    }

    if (earthquakes.length > 2) {
      probability += 25;
      factors.push(`Múltiples terremotos recientes: ${earthquakes.length} eventos`);
      riskLevel = 'high';
    }

    // Limitar probabilidad al 100%
    probability = Math.min(probability, 100);

    res.json({
      success: true,
      prediction: {
        risk_level: riskLevel,
        probability_percentage: probability,
        factors,
        confidence: probability > 50 ? 'medium' : 'low',
        recommendation: getRiskRecommendation(riskLevel, probability)
      },
      analysis_period: '7 days',
      generated_at: new Date().toISOString()
    });

  } catch (error) {
    console.error('Error generando predicción:', error);
    res.status(500).json({
      error: 'Error interno del servidor',
      message: error.message
    });
  }
});

function getRiskRecommendation(riskLevel, probability) {
  switch (riskLevel) {
    case 'high':
      return 'Riesgo elevado de actividad sísmica. Manténgase alerta y revise planes de emergencia.';
    case 'medium':
      return 'Actividad sísmica moderada detectada. Monitoreo continuo recomendado.';
    case 'low':
    default:
      return 'Actividad sísmica normal. Continúe con el monitoreo rutinario.';
  }
}

module.exports = router;
