# Integración con Next.js Frontend

Este documento explica cómo integrar el API Express con un frontend Next.js para crear un dashboard de monitoreo sísmico en tiempo real.

## Estructura del Proyecto Next.js

```
earthquake-dashboard/
├── pages/
│   ├── api/
│   │   └── proxy/          # Proxy para evitar CORS
│   ├── dashboard.js        # Dashboard principal
│   ├── events.js          # Lista de eventos
│   └── analytics.js       # Análisis y estadísticas
├── components/
│   ├── SeismicChart.js    # Gráfico de actividad sísmica
│   ├── EventCard.js       # Tarjeta de evento individual
│   ├── RealTimeAlert.js   # Alertas en tiempo real
│   └── StatsSummary.js    # Resumen de estadísticas
├── lib/
│   ├── api.js             # Cliente API
│   └── websocket.js       # WebSocket para tiempo real
└── styles/
    └── globals.css        # Estilos globales
```

## 1. Cliente API (lib/api.js)

```javascript
const API_BASE_URL = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:3000/api';

class EarthquakeAPI {
  async get(endpoint) {
    const response = await fetch(`${API_BASE_URL}${endpoint}`);
    if (!response.ok) {
      throw new Error(`API Error: ${response.statusText}`);
    }
    return response.json();
  }

  async post(endpoint, data) {
    const response = await fetch(`${API_BASE_URL}${endpoint}`, {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify(data),
    });
    if (!response.ok) {
      throw new Error(`API Error: ${response.statusText}`);
    }
    return response.json();
  }

  // Eventos sísmicos
  async getEvents(limit = 50, offset = 0, eventType = null) {
    let url = `/earthquakes?limit=${limit}&offset=${offset}`;
    if (eventType) url += `&event_type=${eventType}`;
    return this.get(url);
  }

  async getEvent(id) {
    return this.get(`/earthquakes/${id}`);
  }

  // Análisis y estadísticas
  async getGeneralStats(days = 30) {
    return this.get(`/analysis/stats/general?days=${days}`);
  }

  async getActivityTrends(hours = 24) {
    return this.get(`/analysis/trends/activity?hours=${hours}`);
  }

  async getAftershockAnalysis(eventId) {
    return this.get(`/analysis/aftershocks/${eventId}`);
  }

  async getSimplePrediction() {
    return this.get(`/analysis/prediction/simple`);
  }

  // Notificaciones
  async sendNotification(eventId, phoneNumber, message) {
    return this.post('/notifications/send', {
      event_id: eventId,
      phone_number: phoneNumber,
      message: message
    });
  }

  async getNotificationHistory(limit = 50) {
    return this.get(`/notifications/history?limit=${limit}`);
  }

  async updateEmergencyContacts(contacts) {
    return this.post('/notifications/contacts', { contacts });
  }

  // Estado del sistema
  async getHealth() {
    return this.get('/health');
  }
}

export default new EarthquakeAPI();
```

## 2. Dashboard Principal (pages/dashboard.js)

```jsx
import { useState, useEffect } from 'react';
import SeismicChart from '../components/SeismicChart';
import EventCard from '../components/EventCard';
import RealTimeAlert from '../components/RealTimeAlert';
import StatsSummary from '../components/StatsSummary';
import api from '../lib/api';

export default function Dashboard() {
  const [events, setEvents] = useState([]);
  const [stats, setStats] = useState(null);
  const [prediction, setPrediction] = useState(null);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);

  useEffect(() => {
    loadDashboardData();
    
    // Actualizar cada 30 segundos
    const interval = setInterval(loadDashboardData, 30000);
    return () => clearInterval(interval);
  }, []);

  const loadDashboardData = async () => {
    try {
      setLoading(true);
      const [eventsData, statsData, predictionData] = await Promise.all([
        api.getEvents(10),
        api.getGeneralStats(7),
        api.getSimplePrediction()
      ]);
      
      setEvents(eventsData.events || []);
      setStats(statsData);
      setPrediction(predictionData.prediction);
      setError(null);
    } catch (err) {
      setError('Error cargando datos del dashboard');
      console.error('Dashboard error:', err);
    } finally {
      setLoading(false);
    }
  };

  if (loading) {
    return (
      <div className="flex justify-center items-center h-screen">
        <div className="animate-spin rounded-full h-32 w-32 border-b-2 border-blue-500"></div>
      </div>
    );
  }

  return (
    <div className="min-h-screen bg-gray-100">
      <header className="bg-white shadow">
        <div className="max-w-7xl mx-auto py-6 px-4">
          <h1 className="text-3xl font-bold text-gray-900">
            🌍 Dashboard Sísmico IoT
          </h1>
        </div>
      </header>

      <main className="max-w-7xl mx-auto py-6 sm:px-6 lg:px-8">
        {error && (
          <div className="mb-4 bg-red-100 border border-red-400 text-red-700 px-4 py-3 rounded">
            {error}
          </div>
        )}

        {/* Alerta en tiempo real */}
        <RealTimeAlert events={events} />

        {/* Resumen de estadísticas */}
        <StatsSummary stats={stats} prediction={prediction} />

        {/* Grid principal */}
        <div className="mt-8 grid grid-cols-1 lg:grid-cols-2 gap-8">
          {/* Gráfico de actividad */}
          <div className="bg-white overflow-hidden shadow rounded-lg">
            <div className="px-4 py-5 sm:p-6">
              <h3 className="text-lg leading-6 font-medium text-gray-900 mb-4">
                Actividad Sísmica Reciente
              </h3>
              <SeismicChart events={events} />
            </div>
          </div>

          {/* Eventos recientes */}
          <div className="bg-white overflow-hidden shadow rounded-lg">
            <div className="px-4 py-5 sm:p-6">
              <h3 className="text-lg leading-6 font-medium text-gray-900 mb-4">
                Eventos Recientes
              </h3>
              <div className="space-y-4 max-h-96 overflow-y-auto">
                {events.map(event => (
                  <EventCard key={event.id} event={event} />
                ))}
              </div>
            </div>
          </div>
        </div>

        {/* Análisis de predicción */}
        {prediction && (
          <div className="mt-8 bg-white overflow-hidden shadow rounded-lg">
            <div className="px-4 py-5 sm:p-6">
              <h3 className="text-lg leading-6 font-medium text-gray-900 mb-4">
                Análisis Predictivo
              </h3>
              <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
                <div className="text-center">
                  <div className={`text-3xl font-bold ${
                    prediction.risk_level === 'high' ? 'text-red-600' :
                    prediction.risk_level === 'medium' ? 'text-yellow-600' : 'text-green-600'
                  }`}>
                    {prediction.probability_percentage.toFixed(1)}%
                  </div>
                  <div className="text-sm text-gray-500">Probabilidad</div>
                </div>
                <div className="text-center">
                  <div className={`text-xl font-semibold ${
                    prediction.risk_level === 'high' ? 'text-red-600' :
                    prediction.risk_level === 'medium' ? 'text-yellow-600' : 'text-green-600'
                  }`}>
                    {prediction.risk_level.toUpperCase()}
                  </div>
                  <div className="text-sm text-gray-500">Nivel de Riesgo</div>
                </div>
                <div className="text-center">
                  <div className="text-lg text-gray-700">
                    {prediction.confidence.toUpperCase()}
                  </div>
                  <div className="text-sm text-gray-500">Confianza</div>
                </div>
              </div>
              <div className="mt-4 text-sm text-gray-600">
                <strong>Recomendación:</strong> {prediction.recommendation}
              </div>
            </div>
          </div>
        )}
      </main>
    </div>
  );
}
```

## 3. Componente de Evento (components/EventCard.js)

```jsx
import { formatDistanceToNow } from 'date-fns';
import { es } from 'date-fns/locale';

export default function EventCard({ event }) {
  const getEventIcon = (type) => {
    return type === 'earthquake' ? '🌍' : '📳';
  };

  const getEventColor = (type, acceleration) => {
    if (type === 'earthquake') {
      if (acceleration > 20) return 'border-red-500 bg-red-50';
      if (acceleration > 15) return 'border-orange-500 bg-orange-50';
      return 'border-yellow-500 bg-yellow-50';
    }
    return 'border-blue-500 bg-blue-50';
  };

  return (
    <div className={`border-l-4 p-4 rounded-lg ${getEventColor(event.event_type, event.total_acceleration)}`}>
      <div className="flex items-center justify-between">
        <div className="flex items-center space-x-3">
          <span className="text-2xl">{getEventIcon(event.event_type)}</span>
          <div>
            <div className="font-semibold">
              {event.event_type === 'earthquake' ? 'Terremoto' : 'Vibración'}
            </div>
            <div className="text-sm text-gray-600">
              Magnitud: {event.magnitude?.toFixed(1)} | 
              Aceleración: {event.total_acceleration.toFixed(2)} m/s²
            </div>
          </div>
        </div>
        <div className="text-right text-sm text-gray-500">
          <div>{event.device_id}</div>
          <div>
            {formatDistanceToNow(new Date(event.timestamp), { 
              addSuffix: true, 
              locale: es 
            })}
          </div>
        </div>
      </div>
    </div>
  );
}
```

## 4. Gráfico Sísmico (components/SeismicChart.js)

```jsx
import { LineChart, Line, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';
import { format } from 'date-fns';

export default function SeismicChart({ events }) {
  const chartData = events
    .slice(-20) // Últimos 20 eventos
    .map(event => ({
      time: format(new Date(event.timestamp), 'HH:mm'),
      acceleration: event.total_acceleration,
      magnitude: event.magnitude,
      type: event.event_type
    }));

  return (
    <ResponsiveContainer width="100%" height={300}>
      <LineChart data={chartData}>
        <CartesianGrid strokeDasharray="3 3" />
        <XAxis 
          dataKey="time" 
          fontSize={12}
        />
        <YAxis 
          label={{ value: 'Aceleración (m/s²)', angle: -90, position: 'insideLeft' }}
          fontSize={12}
        />
        <Tooltip 
          formatter={(value, name) => [
            `${value.toFixed(2)} m/s²`, 
            'Aceleración'
          ]}
          labelFormatter={(label) => `Hora: ${label}`}
        />
        <Line 
          type="monotone" 
          dataKey="acceleration" 
          stroke="#ef4444" 
          strokeWidth={2}
          dot={{ fill: '#ef4444', strokeWidth: 2, r: 4 }}
        />
      </LineChart>
    </ResponsiveContainer>
  );
}
```

## 5. Configuración Next.js (next.config.js)

```javascript
/** @type {import('next').NextConfig} */
const nextConfig = {
  reactStrictMode: true,
  swcMinify: true,
  env: {
    NEXT_PUBLIC_API_URL: process.env.NEXT_PUBLIC_API_URL || 'http://localhost:3000/api',
    NEXT_PUBLIC_WS_URL: process.env.NEXT_PUBLIC_WS_URL || 'ws://localhost:3001',
  },
  async rewrites() {
    return [
      {
        source: '/api/proxy/:path*',
        destination: 'http://localhost:3000/api/:path*',
      },
    ];
  },
};

module.exports = nextConfig;
```

## 6. Dependencias (package.json)

```json
{
  "dependencies": {
    "next": "^14.0.0",
    "react": "^18.0.0",
    "react-dom": "^18.0.0",
    "recharts": "^2.8.0",
    "date-fns": "^2.30.0",
    "socket.io-client": "^4.7.0"
  },
  "devDependencies": {
    "tailwindcss": "^3.3.0",
    "autoprefixer": "^10.4.0",
    "postcss": "^8.4.0"
  }
}
```

## 7. WebSocket para Tiempo Real (lib/websocket.js)

```javascript
import { io } from 'socket.io-client';

class WebSocketClient {
  constructor() {
    this.socket = null;
    this.listeners = new Map();
  }

  connect() {
    if (this.socket?.connected) return;

    this.socket = io(process.env.NEXT_PUBLIC_WS_URL, {
      transports: ['websocket'],
    });

    this.socket.on('connect', () => {
      console.log('🔌 WebSocket conectado');
    });

    this.socket.on('seismic_event', (data) => {
      this.emit('seismic_event', data);
    });

    this.socket.on('disconnect', () => {
      console.log('🔌 WebSocket desconectado');
    });
  }

  disconnect() {
    if (this.socket) {
      this.socket.disconnect();
      this.socket = null;
    }
  }

  on(event, callback) {
    if (!this.listeners.has(event)) {
      this.listeners.set(event, []);
    }
    this.listeners.get(event).push(callback);
  }

  off(event, callback) {
    const callbacks = this.listeners.get(event);
    if (callbacks) {
      const index = callbacks.indexOf(callback);
      if (index > -1) {
        callbacks.splice(index, 1);
      }
    }
  }

  emit(event, data) {
    const callbacks = this.listeners.get(event);
    if (callbacks) {
      callbacks.forEach(callback => callback(data));
    }
  }
}

export default new WebSocketClient();
```

## Instrucciones de Instalación

```bash
# Crear proyecto Next.js
npx create-next-app@latest earthquake-dashboard --typescript --tailwind --app

# Instalar dependencias adicionales
cd earthquake-dashboard
npm install recharts date-fns socket.io-client

# Configurar variables de entorno
echo "NEXT_PUBLIC_API_URL=http://localhost:3000/api" > .env.local

# Iniciar desarrollo
npm run dev
```

## Características del Dashboard

1. **Tiempo Real**: Actualización automática cada 30 segundos
2. **Gráficos Interactivos**: Visualización de actividad sísmica
3. **Alertas**: Notificaciones visuales para eventos importantes
4. **Responsive**: Diseño adaptable a móviles
5. **Análisis Predictivo**: Mostrar probabilidades de réplicas
6. **Historial**: Navegación por eventos pasados

Este dashboard proporciona una interfaz completa para monitorear la actividad sísmica en tiempo real y gestionar el sistema de alertas.
