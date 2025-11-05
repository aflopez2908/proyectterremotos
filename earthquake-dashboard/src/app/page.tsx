'use client'

import { useState, useEffect } from 'react'
import { AlertTriangle, Wifi, WifiOff, Volume2, MessageSquare, Activity, TrendingUp } from 'lucide-react'
import axios from 'axios'

const API_URL = process.env.NEXT_PUBLIC_API_URL || 'http://localhost:3000/api'

interface SeismicEvent {
  id: number
  device_id: string
  timestamp: string
  magnitude: number
  acceleration_x: number
  acceleration_y: number
  acceleration_z: number
  event_type: string
  is_significant: boolean
}

interface PicoStatus {
  success: boolean
  pico_ip: string
  pico_port: number
  timestamp: string
}

export default function Dashboard() {
  const [picoOnline, setPicoOnline] = useState(false)
  const [recentEvents, setRecentEvents] = useState<SeismicEvent[]>([])
  const [morseText, setMorseText] = useState('')
  const [loading, setLoading] = useState(false)
  const [message, setMessage] = useState('')
  const [stats, setStats] = useState({
    total_events: 0,
    earthquakes: 0,
    vibrations: 0,
    average_magnitude: 0
  })

  // Verificar estado del Pico cada 30 segundos
  useEffect(() => {
    const checkPicoStatus = async () => {
      try {
        const response = await axios.get<PicoStatus>(`${API_URL}/pico/status`)
        setPicoOnline(response.data.success)
      } catch (error) {
        setPicoOnline(false)
      }
    }

    checkPicoStatus()
    const interval = setInterval(checkPicoStatus, 30000)
    return () => clearInterval(interval)
  }, [])

  // Cargar eventos recientes
  useEffect(() => {
    const loadRecentEvents = async () => {
      try {
        const response = await axios.get(`${API_URL}/earthquakes?limit=10`)
        
        // El API devuelve { success: true, events: [...] }
        if (response.data && response.data.success && Array.isArray(response.data.events)) {
          setRecentEvents(response.data.events)
        } else if (Array.isArray(response.data)) {
          // Fallback para APIs que devuelven array directo
          setRecentEvents(response.data)
        } else {
          console.warn('Respuesta de eventos no tiene el formato esperado:', response.data)
          setRecentEvents([])
        }
      } catch (error) {
        console.error('Error cargando eventos:', error)
        setRecentEvents([]) // Asegurar que sea un array vacío en caso de error
      }
    }

    loadRecentEvents()
    const interval = setInterval(loadRecentEvents, 10000) // Actualizar cada 10 segundos
    return () => clearInterval(interval)
  }, [])

  // Cargar estadísticas
  useEffect(() => {
    const loadStats = async () => {
      try {
        const response = await axios.get(`${API_URL}/analysis/stats/general?days=7`)
        
        // Validar que la respuesta tenga la estructura esperada
        if (response.data && typeof response.data === 'object') {
          setStats({
            total_events: response.data.total_events || 0,
            earthquakes: response.data.earthquakes || 0,
            vibrations: response.data.vibrations || 0,
            average_magnitude: response.data.average_magnitude || 0
          })
        }
      } catch (error) {
        console.error('Error cargando estadísticas:', error)
        // Mantener valores por defecto en caso de error
      }
    }

    loadStats()
  }, [])

  const handleBuzzer = async () => {
    setLoading(true)
    setMessage('')
    
    try {
      const response = await axios.post(`${API_URL}/pico/buzzer`)
      setMessage(response.data.message)
    } catch (error: any) {
      setMessage(error.response?.data?.error || 'Error activando buzzer')
    } finally {
      setLoading(false)
    }
  }

  const handleMorse = async () => {
    if (!morseText.trim()) {
      setMessage('Por favor ingresa un texto para enviar en Morse')
      return
    }

    setLoading(true)
    setMessage('')
    
    try {
      const response = await axios.post(`${API_URL}/pico/morse`, {
        text: morseText.trim()
      })
      setMessage(response.data.message)
      setMorseText('')
    } catch (error: any) {
      setMessage(error.response?.data?.error || 'Error enviando mensaje Morse')
    } finally {
      setLoading(false)
    }
  }

  const formatDate = (timestamp: string) => {
    return new Date(timestamp).toLocaleString('es-ES')
  }

  const getEventTypeColor = (type: string, magnitude: number) => {
    if (type === 'earthquake' || magnitude > 15) return 'text-red-600 bg-red-100'
    if (magnitude > 5) return 'text-yellow-600 bg-yellow-100'
    return 'text-green-600 bg-green-100'
  }

  return (
    <div className="min-h-screen bg-gray-50 p-4">
      <div className="max-w-7xl mx-auto">
        {/* Header */}
        <div className="bg-white rounded-lg shadow-sm p-6 mb-6">
          <div className="flex items-center justify-between">
            <div>
              <h1 className="text-3xl font-bold text-gray-900 flex items-center gap-2">
                <Activity className="w-8 h-8 text-blue-600" />
                Sistema de Detección Sísmica
              </h1>
              <p className="text-gray-600 mt-1">Raspberry Pi Pico + MPU6050</p>
            </div>
            <div className="flex items-center gap-2">
              {picoOnline ? (
                <div className="flex items-center gap-2 text-green-600">
                  <Wifi className="w-5 h-5" />
                  <span className="font-medium">Pico Online</span>
                </div>
              ) : (
                <div className="flex items-center gap-2 text-red-600">
                  <WifiOff className="w-5 h-5" />
                  <span className="font-medium">Pico Offline</span>
                </div>
              )}
            </div>
          </div>
        </div>

        {/* Estadísticas rápidas */}
        <div className="grid grid-cols-1 md:grid-cols-4 gap-4 mb-6">
          <div className="bg-white p-6 rounded-lg shadow-sm">
            <div className="flex items-center justify-between">
              <div>
                <p className="text-sm font-medium text-gray-600">Total Eventos</p>
                <p className="text-2xl font-bold text-gray-900">{stats.total_events}</p>
              </div>
              <TrendingUp className="w-8 h-8 text-blue-600" />
            </div>
          </div>
          
          <div className="bg-white p-6 rounded-lg shadow-sm">
            <div className="flex items-center justify-between">
              <div>
                <p className="text-sm font-medium text-gray-600">Terremotos</p>
                <p className="text-2xl font-bold text-red-600">{stats.earthquakes}</p>
              </div>
              <AlertTriangle className="w-8 h-8 text-red-600" />
            </div>
          </div>
          
          <div className="bg-white p-6 rounded-lg shadow-sm">
            <div className="flex items-center justify-between">
              <div>
                <p className="text-sm font-medium text-gray-600">Vibraciones</p>
                <p className="text-2xl font-bold text-yellow-600">{stats.vibrations}</p>
              </div>
              <Activity className="w-8 h-8 text-yellow-600" />
            </div>
          </div>
          
          <div className="bg-white p-6 rounded-lg shadow-sm">
            <div className="flex items-center justify-between">
              <div>
                <p className="text-sm font-medium text-gray-600">Magnitud Promedio</p>
                <p className="text-2xl font-bold text-gray-900">{stats.average_magnitude.toFixed(1)}</p>
              </div>
              <TrendingUp className="w-8 h-8 text-gray-600" />
            </div>
          </div>
        </div>

        <div className="grid grid-cols-1 lg:grid-cols-2 gap-6">
          {/* Control del Pico */}
          <div className="bg-white rounded-lg shadow-sm p-6">
            <h2 className="text-xl font-semibold text-gray-900 mb-4">Control del Pico</h2>
            
            {/* Buzzer */}
            <div className="mb-6">
              <h3 className="text-lg font-medium text-gray-800 mb-2 flex items-center gap-2">
                <Volume2 className="w-5 h-5" />
                Buzzer
              </h3>
              <button
                onClick={handleBuzzer}
                disabled={loading || !picoOnline}
                className="bg-red-500 hover:bg-red-600 disabled:bg-gray-300 text-white px-6 py-2 rounded-lg font-medium transition-colors"
              >
                {loading ? 'Activando...' : 'Activar Buzzer'}
              </button>
            </div>

            {/* Morse */}
            <div className="mb-4">
              <h3 className="text-lg font-medium text-gray-800 mb-2 flex items-center gap-2">
                <MessageSquare className="w-5 h-5" />
                Mensaje Morse
              </h3>
              <div className="flex gap-2">
                <input
                  type="text"
                  value={morseText}
                  onChange={(e) => setMorseText(e.target.value)}
                  placeholder="Texto a enviar (ej. SOS)"
                  maxLength={64}
                  className="flex-1 px-3 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2 focus:ring-blue-500"
                />
                <button
                  onClick={handleMorse}
                  disabled={loading || !picoOnline || !morseText.trim()}
                  className="bg-green-500 hover:bg-green-600 disabled:bg-gray-300 text-white px-6 py-2 rounded-lg font-medium transition-colors"
                >
                  {loading ? 'Enviando...' : 'Enviar'}
                </button>
              </div>
            </div>

            {/* Mensaje de estado */}
            {message && (
              <div className={`mt-4 p-3 rounded-lg ${
                message.includes('Error') || message.includes('error') 
                  ? 'bg-red-100 text-red-700' 
                  : 'bg-green-100 text-green-700'
              }`}>
                {message}
              </div>
            )}

            {!picoOnline && (
              <div className="mt-4 p-3 bg-yellow-100 text-yellow-700 rounded-lg">
                ⚠️ El Pico no está disponible. Verifica la conexión WiFi y que el dispositivo esté encendido.
              </div>
            )}
          </div>

          {/* Eventos recientes */}
          <div className="bg-white rounded-lg shadow-sm p-6">
            <h2 className="text-xl font-semibold text-gray-900 mb-4">Eventos Recientes</h2>
            
            <div className="space-y-3 max-h-96 overflow-y-auto">
              {!Array.isArray(recentEvents) || recentEvents.length === 0 ? (
                <p className="text-gray-500 text-center py-4">No hay eventos registrados</p>
              ) : (
                recentEvents.map((event) => (
                  <div
                    key={event.id}
                    className="border border-gray-200 rounded-lg p-4 hover:bg-gray-50 transition-colors"
                  >
                    <div className="flex items-center justify-between mb-2">
                      <span className={`px-2 py-1 rounded text-xs font-medium ${getEventTypeColor(event.event_type, event.magnitude)}`}>
                        {event.event_type === 'earthquake' ? '🚨 Terremoto' : '📳 Vibración'}
                      </span>
                      <span className="text-sm text-gray-500">
                        {formatDate(event.timestamp)}
                      </span>
                    </div>
                    
                    <div className="grid grid-cols-2 gap-4 text-sm">
                      <div>
                        <span className="font-medium text-gray-600">Magnitud:</span>
                        <span className="ml-1 font-bold">{event.magnitude.toFixed(2)} m/s²</span>
                      </div>
                      <div>
                        <span className="font-medium text-gray-600">Device:</span>
                        <span className="ml-1">{event.device_id}</span>
                      </div>
                    </div>
                    
                    <div className="mt-2 text-xs text-gray-500">
                      X: {event.acceleration_x.toFixed(2)} | 
                      Y: {event.acceleration_y.toFixed(2)} | 
                      Z: {event.acceleration_z.toFixed(2)}
                    </div>
                  </div>
                ))
              )}
            </div>
          </div>
        </div>
      </div>
    </div>
  )
}
