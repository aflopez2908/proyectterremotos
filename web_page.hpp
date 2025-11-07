#ifndef WEB_PAGE_HPP
#define WEB_PAGE_HPP

#include <cstddef>

namespace web {
inline constexpr const char kIndexHtml[] = R"WEBPAGE(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="utf-8">
    <title>Monitor Sísmico - Raspberry Pi Pico</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body{font-family:Arial,Helvetica,sans-serif;background:#2c3e50;color:#ecf0f1;margin:0;display:flex;align-items:center;justify-content:center;min-height:100vh}
        .card{background:rgba(255,255,255,0.1);padding:20px 24px;border-radius:12px;text-align:center;width:92%;max-width:640px}
        h1{margin:6px 0 10px;color:#3498db}
        p{margin:0 0 16px}
        .sensor-data{background:rgba(0,0,0,0.2);padding:15px;border-radius:8px;margin:15px 0;text-align:left}
        .sensor-value{font-weight:bold;color:#e74c3c}
        .status-online{color:#27ae60}
        .status-offline{color:#e74c3c}
        .refresh-btn{background:#3498db;border:none;padding:10px 16px;border-radius:8px;color:#fff;font-weight:700;cursor:pointer;margin-top:10px}
        .refresh-btn:hover{background:#2980b9}
    </style>
    <script>
        function updateSensorData() {
            fetch('/api/sensor')
                .then(response => response.json())
                .then(data => {
                    document.getElementById('accel-x').textContent = data.accel_x.toFixed(3);
                    document.getElementById('accel-y').textContent = data.accel_y.toFixed(3);
                    document.getElementById('accel-z').textContent = data.accel_z.toFixed(3);
                    document.getElementById('gyro-x').textContent = data.gyro_x.toFixed(2);
                    document.getElementById('gyro-y').textContent = data.gyro_y.toFixed(2);
                    document.getElementById('gyro-z').textContent = data.gyro_z.toFixed(2);
                    document.getElementById('magnitude').textContent = data.magnitude.toFixed(3);
                    document.getElementById('status').textContent = data.status;
                    document.getElementById('status').className = data.status === 'online' ? 'status-online' : 'status-offline';
                    document.getElementById('timestamp').textContent = new Date(data.timestamp).toLocaleTimeString();
                })
                .catch(error => {
                    console.error('Error fetching sensor data:', error);
                    document.getElementById('status').textContent = 'offline';
                    document.getElementById('status').className = 'status-offline';
                });
        }
        
        setInterval(updateSensorData, 1000); // Update every second
        window.onload = updateSensorData;
    </script>
</head>
<body>
    <div class="card">
        <h1>📊 Monitor Sísmico</h1>
        <p>Raspberry Pi Pico + MPU6050</p>
        
        <div class="sensor-data">
            <h3>Datos del Sensor</h3>
            <p><strong>Aceleración X:</strong> <span id="accel-x" class="sensor-value">--</span> m/s²</p>
            <p><strong>Aceleración Y:</strong> <span id="accel-y" class="sensor-value">--</span> m/s²</p>
            <p><strong>Aceleración Z:</strong> <span id="accel-z" class="sensor-value">--</span> m/s²</p>
            <p><strong>Giroscopio X:</strong> <span id="gyro-x" class="sensor-value">--</span> °/s</p>
            <p><strong>Giroscopio Y:</strong> <span id="gyro-y" class="sensor-value">--</span> °/s</p>
            <p><strong>Giroscopio Z:</strong> <span id="gyro-z" class="sensor-value">--</span> °/s</p>
            <p><strong>Magnitud:</strong> <span id="magnitude" class="sensor-value">--</span> m/s²</p>
            <p><strong>Estado:</strong> <span id="status" class="status-offline">--</span></p>
            <p><strong>Última actualización:</strong> <span id="timestamp">--</span></p>
        </div>
        
        <button class="refresh-btn" onclick="updateSensorData()">🔄 Actualizar</button>
    </div>
</body>
</html>
)WEBPAGE";

inline constexpr std::size_t kIndexHtmlLen = sizeof(kIndexHtml) - 1;
inline constexpr const char kIndexContentType[] = "text/html; charset=utf-8";
} // namespace web

#endif // WEB_PAGE_HPP
