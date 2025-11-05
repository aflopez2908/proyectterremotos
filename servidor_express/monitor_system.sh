#!/bin/bash

# Script de Monitoreo del Sistema de Detección Sísmica
# Monitorea el API Express, la base de datos y el estado del sistema

API_URL="http://localhost:3000/api"
LOG_FILE="/tmp/earthquake_monitor.log"
ALERT_THRESHOLD=15.0
CHECK_INTERVAL=30

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Función para logging
log_message() {
    local level=$1
    local message=$2
    local timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo -e "[${timestamp}] [${level}] ${message}" | tee -a "$LOG_FILE"
}

# Función para mostrar encabezado
show_header() {
    clear
    echo -e "${BLUE}╔════════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║                🌍 Monitor Sistema Sísmico IoT 🌍               ║${NC}"
    echo -e "${BLUE}║                     $(date '+%Y-%m-%d %H:%M:%S')                      ║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════════════╝${NC}"
    echo
}

# Verificar estado del API
check_api_health() {
    local response=$(curl -s -o /dev/null -w "%{http_code}" "$API_URL/health" 2>/dev/null)
    
    if [ "$response" = "200" ]; then
        echo -e "${GREEN}✅ API Status: ONLINE${NC}"
        return 0
    else
        echo -e "${RED}❌ API Status: OFFLINE (HTTP: $response)${NC}"
        log_message "ERROR" "API health check failed - HTTP: $response"
        return 1
    fi
}

# Verificar base de datos
check_database() {
    local db_check=$(curl -s "$API_URL/earthquakes?limit=1" 2>/dev/null)
    
    if echo "$db_check" | jq -e '.events' >/dev/null 2>&1; then
        echo -e "${GREEN}✅ Database: CONNECTED${NC}"
        return 0
    else
        echo -e "${RED}❌ Database: ERROR${NC}"
        log_message "ERROR" "Database connection failed"
        return 1
    fi
}

# Obtener estadísticas generales
get_system_stats() {
    local stats=$(curl -s "$API_URL/analysis/stats/general?days=1" 2>/dev/null)
    
    if echo "$stats" | jq -e '.' >/dev/null 2>&1; then
        local total_events=$(echo "$stats" | jq -r '.total_events // 0')
        local earthquakes=$(echo "$stats" | jq -r '.earthquakes // 0')
        local vibrations=$(echo "$stats" | jq -r '.vibrations // 0')
        local avg_magnitude=$(echo "$stats" | jq -r '.average_magnitude // 0')
        
        echo -e "${BLUE}📊 Estadísticas (24h):${NC}"
        echo "   • Total eventos: $total_events"
        echo "   • Terremotos: $earthquakes"
        echo "   • Vibraciones: $vibrations"
        echo "   • Magnitud promedio: $avg_magnitude"
    else
        echo -e "${YELLOW}⚠️ No se pudieron obtener estadísticas${NC}"
    fi
}

# Verificar eventos recientes
check_recent_events() {
    local events=$(curl -s "$API_URL/earthquakes?limit=5" 2>/dev/null)
    
    if echo "$events" | jq -e '.events' >/dev/null 2>&1; then
        echo -e "${BLUE}🕐 Últimos 5 eventos:${NC}"
        
        echo "$events" | jq -r '.events[] | "\(.timestamp) | \(.event_type) | Mag: \(.magnitude) | Acc: \(.total_acceleration)m/s²"' | while read -r line; do
            local event_type=$(echo "$line" | cut -d'|' -f2 | xargs)
            local acceleration=$(echo "$line" | cut -d'|' -f4 | grep -o '[0-9]*\.[0-9]*' | head -1)
            
            if [ -n "$acceleration" ] && (( $(echo "$acceleration > $ALERT_THRESHOLD" | bc -l 2>/dev/null || echo 0) )); then
                echo -e "   ${RED}🚨 $line${NC}"
            elif [ "$event_type" = "earthquake" ]; then
                echo -e "   ${YELLOW}🌍 $line${NC}"
            else
                echo -e "   ${GREEN}📳 $line${NC}"
            fi
        done
    else
        echo -e "${YELLOW}⚠️ No se pudieron obtener eventos recientes${NC}"
    fi
}

# Verificar dispositivos conectados
check_connected_devices() {
    local devices=$(curl -s "$API_URL/earthquakes?limit=100" 2>/dev/null | jq -r '.events[].device_id' 2>/dev/null | sort | uniq -c | sort -nr)
    
    if [ -n "$devices" ]; then
        echo -e "${BLUE}📱 Dispositivos activos:${NC}"
        echo "$devices" | while read -r count device; do
            local last_seen=$(curl -s "$API_URL/earthquakes?device_id=$device&limit=1" 2>/dev/null | jq -r '.events[0].timestamp // "unknown"')
            echo "   • $device: $count eventos (último: $last_seen)"
        done
    else
        echo -e "${YELLOW}⚠️ No se encontraron dispositivos${NC}"
    fi
}

# Verificar alertas activas
check_active_alerts() {
    local notifications=$(curl -s "$API_URL/notifications/history?limit=10" 2>/dev/null)
    
    if echo "$notifications" | jq -e '.notifications' >/dev/null 2>&1; then
        local recent_alerts=$(echo "$notifications" | jq -r '.notifications[] | select(.sent_at > (now - 3600)) | .message' 2>/dev/null)
        
        if [ -n "$recent_alerts" ]; then
            echo -e "${RED}🚨 Alertas recientes (1h):${NC}"
            echo "$recent_alerts" | head -3 | while read -r alert; do
                echo "   • $alert"
            done
        else
            echo -e "${GREEN}✅ Sin alertas recientes${NC}"
        fi
    fi
}

# Test de conectividad de dispositivos
test_device_connectivity() {
    echo -e "${BLUE}🔌 Probando conectividad de dispositivos...${NC}"
    
    # Simular datos de prueba
    local test_data='{
        "device_id": "MONITOR_TEST",
        "timestamp": "'$(date -Iseconds)'",
        "event_type": "test",
        "acceleration_x": 0.1,
        "acceleration_y": 0.1,
        "acceleration_z": 9.81,
        "total_acceleration": 9.82,
        "gyro_x": 0.0,
        "gyro_y": 0.0,
        "gyro_z": 0.0,
        "magnitude": 0.0,
        "is_significant": false
    }'
    
    local response=$(curl -s -w "%{http_code}" -X POST \
        -H "Content-Type: application/json" \
        -d "$test_data" \
        "$API_URL/earthquakes/event" 2>/dev/null)
    
    local http_code="${response: -3}"
    
    if [ "$http_code" = "200" ]; then
        echo -e "${GREEN}✅ Test de conectividad: EXITOSO${NC}"
    else
        echo -e "${RED}❌ Test de conectividad: FALLIDO (HTTP: $http_code)${NC}"
        log_message "ERROR" "Device connectivity test failed - HTTP: $http_code"
    fi
}

# Verificar performance del sistema
check_system_performance() {
    echo -e "${BLUE}⚡ Performance del sistema:${NC}"
    
    # CPU y memoria del proceso Node.js
    local node_pid=$(pgrep -f "node.*server.js" | head -1)
    if [ -n "$node_pid" ]; then
        local cpu_usage=$(ps -p "$node_pid" -o %cpu --no-headers | xargs)
        local mem_usage=$(ps -p "$node_pid" -o %mem --no-headers | xargs)
        echo "   • API Server PID: $node_pid"
        echo "   • CPU Usage: ${cpu_usage}%"
        echo "   • Memory Usage: ${mem_usage}%"
    else
        echo -e "${RED}   ❌ API Server no encontrado${NC}"
    fi
    
    # Espacio en disco
    local disk_usage=$(df -h . | awk 'NR==2 {print $5}' | sed 's/%//')
    if [ "$disk_usage" -gt 90 ]; then
        echo -e "${RED}   ⚠️ Disco: ${disk_usage}% usado (CRÍTICO)${NC}"
    elif [ "$disk_usage" -gt 80 ]; then
        echo -e "${YELLOW}   ⚠️ Disco: ${disk_usage}% usado${NC}"
    else
        echo -e "${GREEN}   ✅ Disco: ${disk_usage}% usado${NC}"
    fi
    
    # Tamaño de la base de datos
    if [ -f "database/earthquake_monitor.db" ]; then
        local db_size=$(du -h database/earthquake_monitor.db | cut -f1)
        echo "   • Tamaño DB: $db_size"
    fi
}

# Generar reporte de sistema
generate_report() {
    local report_file="/tmp/earthquake_system_report_$(date +%Y%m%d_%H%M%S).txt"
    
    echo "Generando reporte del sistema..."
    
    {
        echo "REPORTE DEL SISTEMA DE DETECCIÓN SÍSMICA"
        echo "========================================"
        echo "Fecha: $(date)"
        echo "Host: $(hostname)"
        echo ""
        
        echo "ESTADO DEL API"
        echo "-------------"
        check_api_health
        echo ""
        
        echo "ESTADO DE LA BASE DE DATOS"
        echo "-------------------------"
        check_database
        echo ""
        
        echo "ESTADÍSTICAS DEL SISTEMA"
        echo "-----------------------"
        get_system_stats
        echo ""
        
        echo "EVENTOS RECIENTES"
        echo "----------------"
        check_recent_events
        echo ""
        
        echo "DISPOSITIVOS CONECTADOS"
        echo "----------------------"
        check_connected_devices
        echo ""
        
        echo "PERFORMANCE DEL SISTEMA"
        echo "----------------------"
        check_system_performance
        echo ""
        
        echo "LOG DE ERRORES RECIENTES"
        echo "-----------------------"
        if [ -f "$LOG_FILE" ]; then
            tail -20 "$LOG_FILE" | grep ERROR || echo "Sin errores recientes"
        else
            echo "No hay archivo de log disponible"
        fi
        
    } > "$report_file"
    
    echo -e "${GREEN}✅ Reporte generado: $report_file${NC}"
}

# Modo interactivo
interactive_mode() {
    while true; do
        show_header
        
        echo -e "${BLUE}🔍 Verificando sistema...${NC}"
        echo
        
        check_api_health
        check_database
        echo
        
        get_system_stats
        echo
        
        check_recent_events
        echo
        
        check_connected_devices
        echo
        
        check_active_alerts
        echo
        
        check_system_performance
        echo
        
        echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
        echo -e "${YELLOW}Opciones: [q]uit [r]eport [t]est [Enter] para actualizar${NC}"
        
        read -t "$CHECK_INTERVAL" -n 1 choice
        
        case "$choice" in
            q|Q) 
                echo
                echo "👋 Saliendo del monitor..."
                exit 0
                ;;
            r|R)
                echo
                generate_report
                echo "Presiona Enter para continuar..."
                read
                ;;
            t|T)
                echo
                test_device_connectivity
                echo "Presiona Enter para continuar..."
                read
                ;;
            *)
                # Continuar el loop (actualizar)
                ;;
        esac
    done
}

# Modo de una sola ejecución
single_check() {
    show_header
    
    echo -e "${BLUE}🔍 Ejecutando verificación completa...${NC}"
    echo
    
    check_api_health
    check_database
    echo
    
    get_system_stats
    echo
    
    check_recent_events
    echo
    
    check_connected_devices
    echo
    
    check_active_alerts
    echo
    
    test_device_connectivity
    echo
    
    check_system_performance
    echo
    
    echo -e "${GREEN}✅ Verificación completa${NC}"
}

# Verificar dependencias
check_dependencies() {
    local missing_deps=()
    
    command -v curl >/dev/null 2>&1 || missing_deps+=("curl")
    command -v jq >/dev/null 2>&1 || missing_deps+=("jq")
    command -v bc >/dev/null 2>&1 || missing_deps+=("bc")
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        echo -e "${RED}❌ Dependencias faltantes: ${missing_deps[*]}${NC}"
        echo "Instalar con: sudo apt update && sudo apt install ${missing_deps[*]}"
        exit 1
    fi
}

# Función principal
main() {
    check_dependencies
    
    case "${1:-interactive}" in
        "check")
            single_check
            ;;
        "report")
            generate_report
            ;;
        "test")
            test_device_connectivity
            ;;
        "interactive"|"")
            interactive_mode
            ;;
        *)
            echo "Uso: $0 [check|report|test|interactive]"
            echo ""
            echo "Opciones:"
            echo "  check       - Verificación única del sistema"
            echo "  report      - Generar reporte completo"
            echo "  test        - Probar conectividad de dispositivos"
            echo "  interactive - Modo interactivo (por defecto)"
            exit 1
            ;;
    esac
}

# Crear directorio de logs si no existe
mkdir -p "$(dirname "$LOG_FILE")"

# Ejecutar función principal
main "$@"
