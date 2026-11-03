# Excavadora IoT - NodeMCU ESP8266

Proyecto PlatformIO para NodeMCU ESP8266 con:
- WiFiManager y portal de configuración Wi-Fi.
- Dos SN74HC595 en cascada.
- Control manual por monitor serial.
- API HTTPS y recepción de eventos en tiempo real mediante WSS.
- Ejecución de demos dirigida por los movimientos enviados por la API.

## Hardware

| NodeMCU | Función |
|---|---|
| D0 | CLOCK de ambos SN74HC595 |
| D1 | LATCH de ambos SN74HC595 |
| D2 | DATA del primer SN74HC595 |
| Q7S SN1 | DATA del segundo SN74HC595 |
| D6 | Botón de reset Wi-Fi a GND |

El firmware envía primero el byte del segundo registro y después el del primero.

## Instalación

1. Abre la carpeta en VS Code con PlatformIO.
2. Compila y carga el firmware.
3. Abre el monitor serial a 115200 baudios.
4. Si no hay credenciales guardadas, conecta al AP `EXCAVADORA-XXXXXX`, contraseña `Excavadora2026`, y abre `192.168.4.1`.

## Teclas locales

- SN1: V, C, X, Z, L, W, S, J
- SN2: D, I, K, A
- Demos: Q, E, R (solicitan a la API que inicie una demo)
- `!`: borrar credenciales Wi-Fi
- También se pueden borrar manteniendo D6 a GND durante 5 segundos.

## API y WebSocket

- API: `https://iot-alcala.duckdns.org/api`
- WebSocket: `wss://iot-alcala.duckdns.org/ws`
- ID del vehículo: `7`
- Al arrancar se consulta `GET /vehiculos/7` y `GET /vehiculos/7/comandos`.
- Los IDs de comando se obtienen dinámicamente desde el JSON de comandos; el firmware no supone que el código del comando sea su ID.
- Los cambios manuales del ESP8266 se envían a `POST /vehiculos/7/estado` con origen `ESP8266`.
- El ESP8266 ignora los eventos `estado_vehiculo` cuyo origen sea `ESP8266`, para no ejecutar dos veces su propio movimiento.
- Los eventos de otros orígenes, como `POSTMAN` o la aplicación, se ejecutan físicamente.
- Los eventos `demo/ejecutar_movimiento` activan el comando indicado durante `duracion_ms`. La API controla el orden y el tiempo de pausa entre movimientos.
- Al recibir `demo/finalizada` o `demo/cancelada`, el firmware apaga todas las salidas.
- No se llama a `POST /historial` por separado.

### Formato de eventos esperado

Cambio de estado:
```json
{
  "tipo": "estado_vehiculo",
  "accion": "actualizar",
  "datos": {
    "id_vehiculo": 7,
    "id_comando": 29,
    "conectado": true,
    "origen": "POSTMAN"
  }
}
```

Movimiento de demo:
```json
{
  "tipo": "demo",
  "accion": "ejecutar_movimiento",
  "datos": {
    "id_vehiculo": 7,
    "id_demo": 3,
    "id_comando": 29,
    "orden": 3,
    "duracion_ms": 2000,
    "pausa_ms": 600
  }
}
```

Finalización:
```json
{
  "tipo": "demo",
  "accion": "finalizada",
  "datos": {
    "id_vehiculo": 7,
    "id_demo": 3
  }
}
```

### Nombres de demos

El firmware busca los nombres reales:
- `DEMO_8_MOV`
- `DEMO_POSICION`
- `DEMO_ACCION`

Los IDs se consultan desde `GET /vehiculos/7/demos`, no están fijados en el firmware.

## IMPORTANTE: el endpoint de demos debe responder rápidamente

El firmware debe seguir ejecutando `websocket.loop()` mientras llegan los movimientos. Si `POST /vehiculos/{id_vehiculo}/demos/{id_demo}/ejecutar` mantiene abierta la petición HTTP durante toda la demo (como en la implementación original que compartiste), el ESP8266 queda bloqueado esperando la respuesta y no puede procesar oportunamente los mensajes WSS.

Por ello, el endpoint debe validar la demo, iniciar su secuencia en una tarea de fondo y responder inmediatamente. La tarea de fondo enviará los mensajes WebSocket en el orden y con los tiempos configurados. Esto también permite que Q/E/R en el monitor serial soliciten una demo sin bloquear la recepción WSS.

## Seguridad TLS

Para facilitar las pruebas, el cliente WSS se conecta sin validar el certificado del servidor, y `ApiClient.cpp` usa `client.setInsecure()`. La comunicación va cifrada, pero no se comprueba la identidad del servidor. Para producción configura la CA raíz adecuada tanto en HTTPS como en WSS.

## Comportamiento y seguridad

- Solo se mantiene un movimiento físico activo a la vez.
- Un evento de apagado solo detiene el comando que coincide con el que está activo.
- Un nuevo movimiento remoto interrumpe una demo activa.
- Si se pierde Wi-Fi, las salidas se apagan.
- `VELOCIDAD` sigue siendo una salida local porque no está incluida en los comandos proporcionados por la API.
- Verifica que tus optoacopladores se activen con nivel HIGH. Si son activos en LOW, invierte la lógica.
- OE de los SN74HC595 debe estar fijado a GND y MR a HIGH, según el circuito.

## Historial

El firmware manda `conectado:true` al activar y `conectado:false` al desactivar. El backend debe insertar historial solo cuando `conectado` sea `true`; al desactivar debe actualizar el estado sin crear otro registro.
