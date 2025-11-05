# 🌍 Sistema de Detección Sísmica - Guía de Integración Completa

## 📋 Resumen de la Migración

Hemos migrado exitosamente la funcionalidad del `web_page.hpp` (control local del Pico) hacia un sistema distribuido que utiliza Express API como intermediario y Next.js como frontend moderno.

### Flujo Original (web_page.hpp)
```
[Usuario] --> [web_page.hpp en Pico] --> [Buzzer/Morse directo]
```

### Flujo Nuevo (Sistema Distribuido)
```
[Next.js Frontend] --> [Express API] --> [Pico HTTP Server] --> [Buzzer/Morse]
        |                     |                                      |
        v                     v                                      v
  [Dashboard Web]       [Base de Datos]                      [Sensor MPU6050]
  [Estadísticas]        [Notificaciones]                     [Detección Sísmica]
```

## 🏗️ Arquitectura Implementada

### 1. **Raspberry Pi Pico** (Dispositivo IoT)
- **Función**: Servidor HTTP local + Control de hardware + Sensor MPU6050
- **Puertos**: 
  - HTTP Server: `:80`
  - UART/ESP8266: GP4/GP5
  - Buzzer: GP15
- **Endpoints locales**:
  - `GET /` - Página web local (mantenida para acceso directo)
  - `GET /buzzer` - Activar buzzer
  - `GET /morse?text=...` - Enviar mensaje Morse
  - `POST /sensor-data` - Recibir datos del MPU6050 (nuevo)

### 2. **Express API** (Servidor Central)
- **Función**: Intermediario, análisis de datos, notificaciones
- **Puerto**: `:3000`
- **Nuevos endpoints para control del Pico**:
  - `POST /api/pico/buzzer` - Activar buzzer vía API
  - `POST /api/pico/morse` - Enviar mensaje Morse vía API
  - `GET /api/pico/status` - Estado del Pico
  - `POST /api/pico/sensor-data` - Recibir datos del sensor
- **Endpoints sísmicos existentes**:
  - `POST /api/earthquakes/event` - Registrar evento sísmico
  - `GET /api/earthquakes` - Listar eventos
  - `GET /api/analysis/stats/general` - Estadísticas

### 3. **Next.js Frontend** (Dashboard Web)
- **Función**: Interfaz moderna para control y monitoreo
- **Puerto**: `:3001` (por defecto)
- **Características**:
  - Control remoto del buzzer
  - Envío de mensajes Morse
  - Monitoreo en tiempo real del estado del Pico
  - Visualización de eventos sísmicos
  - Estadísticas y gráficos

## 🔧 Configuración Paso a Paso

### 1. Configurar el Servidor Express

```bash
cd servidor_express

# Instalar dependencias (ya hecho)
npm install

# Configurar variables de entorno
nano .env
```

**Variables clave en .env:**
```env
PORT=3000
PICO_IP=192.168.1.100  # IP del Pico en tu red WiFi
PICO_PORT=80
```

### 2. Configurar el Dashboard Next.js

```bash
cd earthquake-dashboard

# Instalar dependencias (ya hecho)
npm install

# Configurar variables de entorno
nano .env.local
```

**Variables en .env.local:**
```env
NEXT_PUBLIC_API_URL=http://localhost:3000/api
NEXT_PUBLIC_PICO_IP=192.168.1.100
NEXT_PUBLIC_PICO_PORT=80
```

### 3. Configurar el Pico

El código del Pico ya está configurado para:
- Conectarse a WiFi (SSID: "Felipe", Password: "pipe1012")
- Servir HTTP en puerto 80
- Enviar datos del MPU6050 al Express API
- Responder a comandos remotos

**Configuración en Config.h:**
```cpp
inline constexpr char WIFI_SSID[] = "Felipe";
inline constexpr char WIFI_PASS[] = "pipe1012";
inline constexpr char API_HOST[] = "tu-api.com";  // Cambiar por IP del servidor
inline constexpr int  API_PORT = 3000;
```

## 🚀 Cómo Ejecutar el Sistema

### 1. Iniciar el Servidor Express
```bash
cd servidor_express
npm run dev
```
✅ Servidor corriendo en http://localhost:3000

### 2. Iniciar el Dashboard Next.js
```bash
cd earthquake-dashboard
npm run dev
```
✅ Dashboard disponible en http://localhost:3001

### 3. Programar y Conectar el Pico
```bash
cd /ruta/al/pico/proyecto
mkdir build && cd build
cmake ..
make
# Copiar archivo .uf2 al Pico en modo BOOTSEL
```

### 4. Verificar la Conexión

**Método 1: Probar desde el Dashboard**
1. Abrir http://localhost:3001
2. Verificar que aparezca "Pico Online" 
3. Probar los botones de buzzer y Morse

**Método 2: Probar con scripts**
```bash
cd servidor_express
./tests/test_pico_integration.sh
```

**Método 3: Acceso directo al Pico**
```bash
# Verificar IP del Pico
curl http://192.168.1.100/

# Activar buzzer directamente
curl http://192.168.1.100/buzzer

# Enviar Morse directamente  
curl "http://192.168.1.100/morse?text=SOS"
```

## 📊 Funcionalidades Migradas

### ✅ Completadas

1. **Control de Buzzer**
   - ✅ Desde Dashboard Next.js
   - ✅ Vía API Express
   - ✅ Acceso directo al Pico (mantenido)

2. **Mensajes Morse** 
   - ✅ Desde Dashboard Next.js con input de texto
   - ✅ Vía API Express con validación
   - ✅ Acceso directo al Pico (mantenido)

3. **Monitoreo de Estado**
   - ✅ Estado online/offline del Pico
   - ✅ Verificación automática cada 30s
   - ✅ Feedback visual en el dashboard

4. **Sistema Sísmico**
   - ✅ Simulación del sensor MPU6050
   - ✅ Detección de eventos sísmicos
   - ✅ Envío automático de datos al API
   - ✅ Almacenamiento en base de datos
   - ✅ Visualización en dashboard

### 🔄 Nuevas Características Añadidas

1. **Dashboard Web Moderno**
   - Interfaz responsive con Tailwind CSS
   - Iconos con Lucide React
   - Estadísticas en tiempo real
   - Lista de eventos recientes

2. **API RESTful Completa**
   - Endpoints para control del Pico
   - Validación de datos
   - Manejo de errores
   - Logs detallados

3. **Base de Datos SQLite**
   - Almacenamiento de eventos sísmicos
   - Estadísticas y análisis
   - Historial para predicción de réplicas

## 🧪 Pruebas del Sistema

### Pruebas Automatizadas
```bash
# Probar todas las rutas del API
cd servidor_express
./tests/test_pico_integration.sh

# Verificar base de datos
./tests/test_api.sh
```

### Pruebas Manuales

1. **Control Básico:**
   - ✅ Activar buzzer desde dashboard
   - ✅ Enviar "SOS" en Morse
   - ✅ Verificar estado del Pico

2. **Eventos Sísmicos:**
   - ✅ Simular evento sísmico (magnitud > 10)
   - ✅ Verificar aparición en dashboard
   - ✅ Comprobar notificaciones (si configuradas)

3. **Conectividad:**
   - ✅ Dashboard funciona sin Pico online
   - ✅ Mensajes de error apropiados
   - ✅ Reconexión automática

## 🚨 Solución de Problemas

### Pico Aparece Offline

1. **Verificar Red WiFi:**
   ```bash
   # Ping al Pico
   ping 192.168.1.100
   
   # Curl directo
   curl http://192.168.1.100/
   ```

2. **Verificar Configuración:**
   - SSID y password en Config.h
   - IP correcta en .env del servidor
   - Puerto 80 disponible

### API No Responde

1. **Verificar Servidor Express:**
   ```bash
   # Estado del proceso
   ps aux | grep node
   
   # Logs en tiempo real
   tail -f logs/server.log
   ```

2. **Verificar Puerto:**
   ```bash
   # Ver puertos en uso
   netstat -tlnp | grep 3000
   
   # Curl al API
   curl http://localhost:3000/api/health
   ```

### Dashboard No Carga

1. **Verificar Next.js:**
   ```bash
   # Reinstalar dependencias
   cd earthquake-dashboard
   rm -rf node_modules package-lock.json
   npm install
   
   # Verificar variables de entorno
   cat .env.local
   ```

## 📈 Próximos Pasos

### Funcionalidades Pendientes

1. **Integración MPU6050 Real:**
   - Reemplazar simulación con sensor físico
   - Calibración y filtrado de datos
   - Detección de patrones sísmicos avanzados

2. **Notificaciones WhatsApp:**
   - Configurar API de WhatsApp Business
   - Alertas automáticas por terremotos
   - Lista de contactos de emergencia

3. **Dashboard Avanzado:**
   - Gráficos en tiempo real con Recharts
   - Mapas de actividad sísmica
   - Predicción de réplicas con IA

4. **Escalabilidad:**
   - Soporte para múltiples sensores Pico
   - Base de datos PostgreSQL
   - API Gateway con rate limiting

### Mejoras de Rendimiento

1. **WebSockets:**
   - Datos en tiempo real sin polling
   - Notificaciones push instantáneas

2. **Caching:**
   - Redis para datos frecuentes
   - CDN para assets estáticos

3. **Monitoreo:**
   - Prometheus + Grafana
   - Alertas de sistema
   - Métricas de rendimiento

## 🔗 Enlaces Útiles

- **Dashboard**: http://localhost:3001
- **API Docs**: http://localhost:3000/api/health
- **Pico Direct**: http://192.168.1.100/
- **Logs Express**: `servidor_express/logs/`
- **DB Browser**: `servidor_express/database/earthquakes.db`

---

**🎉 ¡Sistema Successfully Migrado!**

La funcionalidad del `web_page.hpp` ahora está completamente integrada en un sistema distribuido moderno que mantiene la compatibilidad con el acceso directo al Pico mientras añade capacidades avanzadas de monitoreo, análisis y control remoto.
