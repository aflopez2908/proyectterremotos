const express = require('express');
const cors = require('cors');
const helmet = require('helmet');
const morgan = require('morgan');
require('dotenv').config();

const earthquakeRoutes = require('./routes/earthquakes');
const analysisRoutes = require('./routes/analysis');
const notificationRoutes = require('./routes/notifications');
const picoRoutes = require('./routes/pico');
const { initDatabase } = require('./database/init');

const app = express();
const PORT = process.env.PORT || 3000;

// Middlewares de seguridad
app.use(helmet());
app.use(cors());
app.use(morgan('combined'));
app.use(express.json());
app.use(express.urlencoded({ extended: true }));

// Inicializar base de datos
initDatabase().then(() => {
  console.log('🎯 Base de datos lista');
}).catch(err => {
  console.error('❌ Error inicializando base de datos:', err);
});

// Rutas
app.use('/api/earthquakes', earthquakeRoutes);
app.use('/api/analysis', analysisRoutes);
app.use('/api/notifications', notificationRoutes);
app.use('/api/pico', picoRoutes);

// Ruta de estado del servidor
app.get('/api/health', (req, res) => {
  res.json({
    status: 'OK',
    timestamp: new Date().toISOString(),
    uptime: process.uptime(),
    message: 'Servidor de detección sísmica funcionando correctamente'
  });
});

// Ruta principal
app.get('/', (req, res) => {
  res.json({
    message: 'API de Detección de Terremotos',
    version: '1.0.0',
    endpoints: {
      earthquakes: '/api/earthquakes',
      analysis: '/api/analysis',
      notifications: '/api/notifications',
      health: '/api/health'
    }
  });
});

// Manejo de errores
app.use((err, req, res, next) => {
  console.error('Error:', err);
  res.status(500).json({
    error: 'Error interno del servidor',
    message: process.env.NODE_ENV === 'development' ? err.message : 'Something went wrong!'
  });
});

// Ruta 404
app.use((req, res) => {
  res.status(404).json({
    error: 'Endpoint no encontrado',
    message: `La ruta ${req.originalUrl} no existe`
  });
});

app.listen(PORT, () => {
  console.log(`🚀 Servidor ejecutándose en puerto ${PORT}`);
  console.log(`📊 API disponible en http://localhost:${PORT}`);
  console.log(`🔍 Health check: http://localhost:${PORT}/api/health`);
});

module.exports = app;
