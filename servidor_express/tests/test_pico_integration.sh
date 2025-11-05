#!/bin/bash

# Script de pruebas para las nuevas rutas del Pico
API_URL="http://localhost:3000/api"

echo "🧪 Probando API del sistema de detección sísmica"
echo "================================================"

# Test 1: Health check del servidor
echo -e "\n1️⃣ Health check del servidor..."
curl -s -X GET "$API_URL/health" | jq '.'

# Test 2: Estado del Pico
echo -e "\n2️⃣ Verificando estado del Pico..."
curl -s -X GET "$API_URL/pico/status" | jq '.'

# Test 3: Activar buzzer
echo -e "\n3️⃣ Activando buzzer del Pico..."
curl -s -X POST "$API_URL/pico/buzzer" | jq '.'

# Test 4: Enviar mensaje Morse
echo -e "\n4️⃣ Enviando mensaje Morse (SOS)..."
curl -s -X POST "$API_URL/pico/morse" \
  -H "Content-Type: application/json" \
  -d '{"text": "SOS"}' | jq '.'

# Test 5: Enviar mensaje Morse más largo
echo -e "\n5️⃣ Enviando mensaje Morse (HOLA MUNDO)..."
curl -s -X POST "$API_URL/pico/morse" \
  -H "Content-Type: application/json" \
  -d '{"text": "HOLA MUNDO"}' | jq '.'

# Test 6: Simular datos del sensor
echo -e "\n6️⃣ Simulando datos normales del sensor..."
curl -s -X POST "$API_URL/pico/sensor-data" \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "pico_sensor_01", 
    "acceleration_x": 1.2, 
    "acceleration_y": -0.8, 
    "acceleration_z": 9.8,
    "gyro_x": 0.1,
    "gyro_y": 0.05,
    "gyro_z": -0.02,
    "temperature": 24.5
  }' | jq '.'

# Test 7: Simular evento sísmico
echo -e "\n7️⃣ Simulando evento sísmico..."
curl -s -X POST "$API_URL/pico/sensor-data" \
  -H "Content-Type: application/json" \
  -d '{
    "device_id": "pico_sensor_01", 
    "acceleration_x": 12.5, 
    "acceleration_y": -8.3, 
    "acceleration_z": 15.7,
    "gyro_x": 45.2,
    "gyro_y": -23.1,
    "gyro_z": 67.8,
    "temperature": 25.1
  }' | jq '.'

# Test 8: Listar eventos registrados
echo -e "\n8️⃣ Listando eventos registrados..."
curl -s -X GET "$API_URL/earthquakes?limit=5" | jq '.'

echo -e "\n✅ Pruebas completadas!"
echo "📋 Para monitorear en tiempo real:"
echo "   tail -f logs/earthquake_system.log"
