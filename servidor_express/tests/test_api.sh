#!/bin/bash

# Script de prueba para el API de detección de terremotos
# Simula datos enviados desde el Raspberry Pi Pico

API_URL="http://localhost:3000"

echo "🧪 Iniciando pruebas del API de detección sísmica..."
echo "======================================================"

# Función para mostrar respuesta JSON de forma legible
show_response() {
    echo "📡 Respuesta:"
    echo "$1" | python3 -m json.tool 2>/dev/null || echo "$1"
    echo ""
}

# 1. Verificar estado del servidor
echo "1️⃣ Verificando estado del servidor..."
response=$(curl -s "$API_URL/api/health")
show_response "$response"

# 2. Simular vibración menor
echo "2️⃣ Enviando datos de vibración menor..."
response=$(curl -s -X POST "$API_URL/api/earthquakes/event" \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "pico_sensor_01", 
    "magnitude": 3.2,
    "acceleration_x": 1.1,
    "acceleration_y": 0.8,
    "acceleration_z": 2.9,
    "total_acceleration": 3.2
  }')
show_response "$response"

# 3. Simular terremoto moderado
echo "3️⃣ Enviando datos de terremoto moderado..."
response=$(curl -s -X POST "$API_URL/api/earthquakes/event" \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "pico_sensor_01",
    "magnitude": 6.5,
    "acceleration_x": 2.3,
    "acceleration_y": 1.8,
    "acceleration_z": 15.2,
    "total_acceleration": 15.4
  }')
show_response "$response"

# 4. Simular terremoto fuerte
echo "4️⃣ Enviando datos de terremoto fuerte..."
response=$(curl -s -X POST "$API_URL/api/earthquakes/event" \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "pico_sensor_01",
    "magnitude": 7.8,
    "acceleration_x": 5.2,
    "acceleration_y": 3.1,
    "acceleration_z": 22.5,
    "total_acceleration": 23.1
  }')
show_response "$response"

# 5. Obtener lista de eventos
echo "5️⃣ Obteniendo lista de eventos..."
response=$(curl -s "$API_URL/api/earthquakes?limit=10")
show_response "$response"

# 6. Obtener estadísticas generales
echo "6️⃣ Obteniendo estadísticas generales..."
response=$(curl -s "$API_URL/api/analysis/stats/general?days=1")
show_response "$response"

# 7. Obtener predicción simple
echo "7️⃣ Obteniendo predicción de actividad..."
response=$(curl -s "$API_URL/api/analysis/prediction/simple")
show_response "$response"

# 8. Probar notificación manual
echo "8️⃣ Enviando notificación de prueba..."
response=$(curl -s -X POST "$API_URL/api/notifications/send" \
  -H "Content-Type: application/json" \
  -d '{
    "event_id": 2,
    "phone_number": "+1234567890",
    "message": "Mensaje de prueba del sistema de detección sísmica"
  }')
show_response "$response"

# 9. Obtener historial de notificaciones
echo "9️⃣ Obteniendo historial de notificaciones..."
response=$(curl -s "$API_URL/api/notifications/history?limit=5")
show_response "$response"

# 10. Verificar información de la API
echo "🔟 Información general de la API..."
response=$(curl -s "$API_URL/")
show_response "$response"

echo "✅ Pruebas completadas!"
echo "📊 Revisar los logs del servidor para ver el procesamiento"
echo "🌐 Abrir http://localhost:3000 en el navegador para ver la API"
