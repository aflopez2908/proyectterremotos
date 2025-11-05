const sqlite3 = require('sqlite3').verbose();
const path = require('path');

const DB_PATH = process.env.DB_PATH || './database/earthquakes.db';

// Asegurar que el directorio existe
const fs = require('fs');
const dbDir = path.dirname(DB_PATH);
if (!fs.existsSync(dbDir)) {
  fs.mkdirSync(dbDir, { recursive: true });
}

const db = new sqlite3.Database(DB_PATH, (err) => {
  if (err) {
    console.error('Error conectando a la base de datos:', err.message);
  } else {
    console.log('✅ Conectado a la base de datos SQLite');
  }
});

const initDatabase = () => {
  console.log('🔄 Inicializando base de datos...');
  
  return new Promise((resolve, reject) => {
    // Crear tablas en secuencia usando serialize
    db.serialize(() => {
      // Tabla principal de eventos sísmicos
      db.run(`
        CREATE TABLE IF NOT EXISTS earthquake_events (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          device_id TEXT NOT NULL,
          timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
          magnitude REAL NOT NULL,
          acceleration_x REAL,
          acceleration_y REAL,
          acceleration_z REAL,
          total_acceleration REAL NOT NULL,
          event_type TEXT CHECK(event_type IN ('vibration', 'earthquake')) NOT NULL,
          location TEXT,
          processed BOOLEAN DEFAULT FALSE,
          notification_sent BOOLEAN DEFAULT FALSE,
          created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
      `, (err) => {
        if (err) console.error('Error creando tabla earthquake_events:', err);
        else console.log('✅ Tabla earthquake_events creada');
      });

      // Tabla de análisis de réplicas
      db.run(`
        CREATE TABLE IF NOT EXISTS aftershock_analysis (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          main_event_id INTEGER NOT NULL,
          probability_percentage REAL NOT NULL,
          analysis_timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
          factors TEXT,
          expires_at DATETIME NOT NULL,
          FOREIGN KEY(main_event_id) REFERENCES earthquake_events(id)
        )
      `, (err) => {
        if (err) console.error('Error creando tabla aftershock_analysis:', err);
        else console.log('✅ Tabla aftershock_analysis creada');
      });

      // Tabla de notificaciones enviadas
      db.run(`
        CREATE TABLE IF NOT EXISTS notifications (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          event_id INTEGER NOT NULL,
          notification_type TEXT CHECK(notification_type IN ('whatsapp', 'email', 'sms')) NOT NULL,
          recipient TEXT NOT NULL,
          message TEXT NOT NULL,
          status TEXT CHECK(status IN ('pending', 'sent', 'failed')) DEFAULT 'pending',
          sent_at DATETIME,
          error_message TEXT,
          created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
          FOREIGN KEY(event_id) REFERENCES earthquake_events(id)
        )
      `, (err) => {
        if (err) console.error('Error creando tabla notifications:', err);
        else console.log('✅ Tabla notifications creada');
      });

      // Tabla de configuración del sistema
      db.run(`
        CREATE TABLE IF NOT EXISTS system_config (
          id INTEGER PRIMARY KEY AUTOINCREMENT,
          config_key TEXT UNIQUE NOT NULL,
          config_value TEXT NOT NULL,
          description TEXT,
          updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
        )
      `, (err) => {
        if (err) console.error('Error creando tabla system_config:', err);
        else console.log('✅ Tabla system_config creada');
      });

      // Insertar configuraciones por defecto al final
      db.run(`
        INSERT OR IGNORE INTO system_config (config_key, config_value, description) 
        VALUES 
        ('earthquake_threshold', '15.0', 'Umbral de aceleración para detectar terremotos (m/s²)'),
        ('vibration_threshold', '5.0', 'Umbral de aceleración para detectar vibraciones (m/s²)'),
        ('aftershock_window_hours', '72', 'Ventana de tiempo para análisis de réplicas (horas)'),
        ('notification_cooldown_minutes', '15', 'Tiempo mínimo entre notificaciones del mismo tipo')
      `, (err) => {
        if (err) {
          console.error('Error insertando configuraciones por defecto:', err);
          reject(err);
        } else {
          console.log('✅ Configuraciones por defecto insertadas');
          console.log('📊 Tablas de la base de datos inicializadas correctamente');
          resolve();
        }
      });
    });
  });
};

const getDatabase = () => db;

module.exports = {
  initDatabase,
  getDatabase
};
