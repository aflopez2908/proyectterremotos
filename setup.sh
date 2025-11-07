#!/bin/bash

# 🚀 Script de Inicio Completo del Sistema de Detección Sísmica
# ============================================================

echo "🚀 ===== SISTEMA DE DETECCIÓN SÍSMICA ====="
echo "📅 $(date)"
echo "=============================================="

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Función para imprimir con colores
print_status() {
    echo -e "${GREEN}✅ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠️  $1${NC}"
}

print_error() {
    echo -e "${RED}❌ $1${NC}"
}

print_info() {
    echo -e "${BLUE}ℹ️  $1${NC}"
}

# Verificar que estamos en el directorio correcto
if [ ! -f "main.cpp" ] || [ ! -d "servidor_express" ] || [ ! -d "earthquake-dashboard" ]; then
    print_error "Debes ejecutar este script desde el directorio raíz del proyecto"
    exit 1
fi

print_info "Directorio del proyecto verificado"

# 1. Verificar e instalar dependencias del servidor Express
echo
echo "📦 1. CONFIGURANDO SERVIDOR EXPRESS..."
cd servidor_express

if [ ! -f "package.json" ]; then
    print_error "No se encontró package.json en servidor_express"
    exit 1
fi

if [ ! -d "node_modules" ]; then
    print_warning "Instalando dependencias del servidor..."
    npm install
    if [ $? -ne 0 ]; then
        print_error "Falló la instalación de dependencias del servidor"
        exit 1
    fi
    print_status "Dependencias del servidor instaladas"
else
    print_status "Dependencias del servidor ya instaladas"
fi

# Verificar archivo .env
if [ ! -f ".env" ]; then
    print_warning "Creando archivo .env por defecto..."
    cat > .env << EOF
# Configuración del servidor
PORT=3000

# Configuración del Pico
PICO_IP=192.168.1.100
PICO_PORT=80

# WhatsApp (opcional)
WHATSAPP_ENABLED=false

# Base de datos
DATABASE_PATH=./database.sqlite
EOF
    print_status "Archivo .env creado"
else
    print_status "Archivo .env ya existe"
fi

cd ..

# 2. Verificar e instalar dependencias del dashboard
echo
echo "🌐 2. CONFIGURANDO DASHBOARD NEXT.JS..."
cd earthquake-dashboard

if [ ! -f "package.json" ]; then
    print_error "No se encontró package.json en earthquake-dashboard"
    exit 1
fi

if [ ! -d "node_modules" ]; then
    print_warning "Instalando dependencias del dashboard..."
    npm install
    if [ $? -ne 0 ]; then
        print_error "Falló la instalación de dependencias del dashboard"
        exit 1
    fi
    print_status "Dependencias del dashboard instaladas"
else
    print_status "Dependencias del dashboard ya instaladas"
fi

# Verificar archivo .env.local
if [ ! -f ".env.local" ]; then
    print_warning "Creando archivo .env.local por defecto..."
    cat > .env.local << EOF
# URL del API Express
NEXT_PUBLIC_API_URL=http://localhost:3000/api
EOF
    print_status "Archivo .env.local creado"
else
    print_status "Archivo .env.local ya existe"
fi

cd ..

# 3. Compilar código del Pico
echo
echo "🔧 3. COMPILANDO CÓDIGO DEL PICO..."

if [ ! -d "build" ]; then
    print_warning "Creando directorio build..."
    mkdir build
fi

cd build

# Verificar si cmake ya se ejecutó
if [ ! -f "Makefile" ]; then
    print_info "Ejecutando cmake..."
    cmake ..
    if [ $? -ne 0 ]; then
        print_error "Falló la configuración de cmake"
        print_info "Verifica que PICO_SDK_PATH esté configurado:"
        print_info "export PICO_SDK_PATH=~/pico-sdk"
        exit 1
    fi
    print_status "Configuración de cmake completada"
fi

print_info "Compilando código del Pico..."
make -j4
if [ $? -ne 0 ]; then
    print_error "Falló la compilación del código del Pico"
    exit 1
fi

print_status "Código del Pico compilado exitosamente"
print_info "Archivo generado: $(pwd)/serv_http_esp8266.uf2"

cd ..

# 4. Mostrar instrucciones finales
echo
echo "🎯 4. INSTRUCCIONES DE USO"
echo "=========================="

cat << EOF

📋 PASOS PARA COMPLETAR LA CONFIGURACIÓN:

1. 🔌 HARDWARE:
   - Conecta MPU6050 al Pico (SDA→GP16, SCL→GP17)
   - Conecta ESP8266 al Pico (TX→GP4, RX→GP5)  
   - Conecta buzzer al Pico (→GP15)
   
2. ⚙️  CONFIGURACIÓN:
   - Edita Config.h con tu WiFi:
     * WIFI_SSID[] = "TU_WIFI";
     * WIFI_PASS[] = "TU_PASSWORD";
     * API_HOST[] = "192.168.1.XXX"; (IP de este PC)

3. 📱 FLASHEAR PICO:
   - Mantén BOOTSEL presionado
   - Conecta USB
   - Copia build/serv_http_esp8266.uf2 al Pico
   
4. 🚀 INICIAR SERVICIOS:

   Terminal 1 - Servidor Express:
   cd servidor_express && npm run dev
   
   Terminal 2 - Dashboard:
   cd earthquake-dashboard && npm run dev
   
5. 🌐 ACCEDER AL SISTEMA:
   - Dashboard: http://localhost:3001
   - API: http://localhost:3000
   - Pico: http://[IP_DEL_PICO]

📊 VERIFICACIÓN:
- El dashboard debe mostrar "Pico Online" en verde
- Los eventos sísmicos aparecerán en tiempo real
- Puedes controlar el buzzer remotamente

🔧 TROUBLESHOOTING:
- Revisa logs en monitor serie del Pico
- Verifica conectividad de red
- Confirma que el ESP8266 se conecte a WiFi

EOF

print_status "¡Sistema listo para usar!"
print_info "Próximos pasos: configurar WiFi y flashear el Pico"

echo
echo "🎉 ===== CONFIGURACIÓN COMPLETADA ====="
