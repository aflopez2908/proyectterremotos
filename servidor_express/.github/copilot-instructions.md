<!-- Use this file to provide workspace-specific custom instructions to Copilot. For more details, visit https://code.visualstudio.com/docs/copilot/copilot-customization#_use-a-githubcopilotinstructionsmd-file -->

# Sistema de Detección de Terremotos IoT

Este proyecto es una API Express.js que forma parte de un sistema integral de detección sísmica con IoT.

## Arquitectura del Sistema

- **Raspberry Pi Pico + ESP8266**: Sensor MPU6050 para detección de vibraciones y terremotos
- **Express API**: Procesamiento de datos, análisis y notificaciones
- **Next.js Frontend**: Dashboard web para visualización en tiempo real
- **SQLite Database**: Almacenamiento de eventos sísmicos y análisis
- **WhatsApp Integration**: Notificaciones automáticas de emergencia

## Funcionalidades Clave

1. **Recepción de Datos**: Endpoint `/api/earthquakes/event` para recibir datos del Pico
2. **Análisis de Réplicas**: Algoritmo de predicción de réplicas basado en patrones históricos
3. **Notificaciones WhatsApp**: Sistema automático de alertas de emergencia
4. **Base de Datos**: SQLite con tablas para eventos, análisis y notificaciones
5. **API REST**: Endpoints completos para el frontend Next.js

## Consideraciones de Desarrollo

- Usar async/await para operaciones de base de datos
- Implementar validación de datos de entrada
- Manejar errores apropiadamente con try/catch
- Seguir convenciones REST para los endpoints
- Documentar parámetros de configuración en .env
- Implementar cooldown para notificaciones duplicadas
- Usar transacciones de BD para operaciones críticas

## Integración IoT

El sistema está diseñado para recibir datos JSON del Pico con formato:
```json
{
  "device_id": "pico_sensor_01",
  "magnitude": 6.5,
  "acceleration_x": 2.3,
  "acceleration_y": 1.8,
  "acceleration_z": 15.2,
  "total_acceleration": 15.4,
  "timestamp": "2024-11-05T15:30:00Z"
}
```

## Configuración de Producción

- Configurar WhatsApp Business API real
- Implementar autenticación JWT
- Añadir rate limiting
- Configurar HTTPS
- Implementar logs de auditoría
- Configurar backup automático de BD
