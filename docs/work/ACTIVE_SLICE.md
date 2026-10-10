# Vertical activo — N0: baseline JE permitido y preparación de BSP visible

Fecha 2026-10-10. Estado **READY TO EXECUTE / ENGINE NOT TESTED**. development solamente. Esta revisión documenta la última orden, no anuncia que ya se quitaron dependencias ni se migró código.

## Resultado de N0

Inspeccionar/configurar la base con dependencias privadas permitidas y locales, sin AWS/multiplayer productivos. Mostrar escena base con Editor/Asset Processor/runtime y registrar métricas. Pares obligatorios: Windows-DX12, Windows-Vulkan, Linux-Vulkan. Par no disponible queda NOT TESTED y no bloquea desarrollo visible del disponible; no se declara N0 portable completo.

## Lectura y tareas concretas

Leer GOAL/AGENTS, `docs/architecture/JE_IDENTITY_AND_NATIVE_MODULES.md`, `docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md` y N0_FOUNDATION_PLAN.

Resolver listas efectivas de package/LFS sin CloudFront/S3/fallback oculto; separar transporte de juego de IPC local; mapear targets servidor/multiplayer y RHI de producto. No usar red de Amazon para obtener un primer baseline ni borrar carpetas por grep. Fijar toolchain/deps hashes/licencias y conservar assets/avisos.

Registrar fuente/configuración/build/códigos de salida, fixture/imagen real, coste cold/steady CPU/GPU/RAM/VRAM y limits. Lectura CMake no es linker/runtime. No generar placeholders JE para marcar avance.

## Siguiente vertical prioritario

N1 UI/BSP: cuarto/corredor/apertura puerta/ventana sobre WhiteBox CSG auditado; UV/material/Undo/Save/reopen y runtime. Integración del owner a JE dentro del engine sin project Gem install. Después N2 materiales y N4 collision, N8 geología adelantado; no esperar todos los sistemas de juego.

## Prohibiciones

Nada AWS/multiplayer/servidores/backend online. Solo DX12/Vulkan como render. No alias JE=AZ como solución final. Geología está activa; BSP→OpenVDB y RT de render no. Buoyancy continúa DESIGN ONLY. No copiar source/shaders de Cry ni borrar otros repositorios/ramas/evidencia.
