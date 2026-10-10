# Vertical activo — N0: baseline JE permitido y preparación de BSP visible

Fecha 2026-10-10. Estado **N0 IN PROGRESS / ORIGINAL ENGINE BUILD BLOCKED**. development solamente. Checkout único `F:\JericoEngine\JericoEngine`, base `35e413726655b13e9523bd478e7de3e55d21bc92`. [Expediente y evidencia real](../evidence/n0/N0_EXECUTION_2026_10_10.md).

## Ejecución actual

Clone/fetch/pull ff-only y estado inicial limpio comprobados, documentos obligatorios leídos. Integración publicada anterior a esta ampliación `b54a874e3d0d312d78ad73f79920e676f26b932b`; GOAL persistente ACTIVE en este chat. Inventarios de toolchain/source/LFS/packages registrados; repos históricos intactos. Se eliminó el output propio vacío `python-v143`; dos outputs propios poblados e inútiles (280.206.279 bytes) siguen inventariados porque la revisión automática rechazó su borrado recursivo por política, sin motivo más específico. No se intentó eludir el rechazo.

Ya se reconstruyeron zlib 1.2.11, Freetype 2.11.1 y Python 3.10.13 con identidades/hashes JE y consumers PASS. Qt/PySide 6.11.2 exactos auditados; root instaló requisitos y cuatro herramientas desde wheelhouse sin índices, pip check/imports/ausencia boto3/botocore/s3transfer PASS. Negativo wheelhouse dedicado falla explícitamente, sin instalar requisitos. Mimalloc 3.5.2 fuente/pin/MIT auditados, metadata corregida y aceptado. Último configure con encoding final **exit 1 / 9,528 s por Lua-5.4.4-rev1-windows ausente**; no build. Fuente/receta Lua ya auditadas, producer siguiente. Banner Git Python del ancestor engine es deuda demostrada sin afectar version_info/ABI; corrección/rebuild pendiente. No baseline ni cierre transitivo.

Implementados resolver local sin default CloudFront/S3/fallback Git y `.lfsconfig` sin URL heredada, con exclusión de materialización. Suite CMake focal final 24/24 PASS; LFS devuelve un puntero real byte/hash idéntico, hooks/config intactos. Configure final exit 1 / 0,160 s con error específico zlib/SHA/rutas. Límites de cierre transitivo en evidencia. No equivalen a ausencia AWS/multiplayer en grafo/link/deploy/runtime; esa retirada sigue pendiente.

Núcleo productivo CSG migrado realmente a `JE::BSP::Csg`, sin AzCore/alias; conserva salida atómica, vacío válido y legacy geométrico. Añade UV por esquina e identidad de superficie de dos enteros de 64 bits, con costuras protegidas. RED/diagnóstico/GREEN reales: Release build exit 0 / 4,585 s, dos CTests exit 0 / 0,103 s, **117 checks PASS**; incluye habitación/corredor/puerta/ventana y negativos. Adapters White Box integrados por fuente **NOT VERIFIED** y todavía pierden los atributos; guard de vacío mantiene escena hasta teardown render/collider probado. Material Atom por cara, UV del editor y captura real pendientes.

Owner: padre integración/aceptación/Git, writers dependency/BSP congelados al validar, Luna único owner CMake/build/tests. Ninguna plataforma/consumidor se acredita por el fixture aislado.

## Resultado de N0

Inspeccionar/configurar la base con dependencias privadas permitidas y locales, sin AWS/multiplayer productivos. Mostrar escena base con Editor/Asset Processor/runtime y registrar métricas. Pares obligatorios: Windows-DX12, Windows-Vulkan, Linux-Vulkan. Par no disponible queda NOT TESTED y no bloquea desarrollo visible del disponible; no se declara N0 portable completo.

## Lectura y tareas concretas

Leer GOAL/AGENTS, `docs/architecture/JE_IDENTITY_AND_NATIVE_MODULES.md`, `docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md` y N0_FOUNDATION_PLAN.

Aprovisionar Lua 5.4.4 desde receta/fuente/pin/licencia auditados y seguir únicamente dependencias realmente requeridas; corregir banner Git de Python en la siguiente reconstrucción con nueva validación. Generar targets/compilar Editor/AP/launcher. Ruta existente AutomatedTesting ofrece Base.prefab y WhiteBox, pero incluye multiplayer y seis texturas TIF aún punteros LFS; corregir composición y fuentes permitidas antes de acreditar escena. Continuar clasificación efectiva, IPC y mapa servidor/multiplayer/RHI. No Amazon ni borrado por grep.

Registrar fuente/configuración/build/códigos de salida, fixture/imagen real, coste cold/steady CPU/GPU/RAM/VRAM y limits. Lectura CMake no es linker/runtime. No generar placeholders JE para marcar avance.

## Siguiente vertical prioritario

N1 UI/BSP: cuarto/corredor/apertura puerta/ventana sobre WhiteBox CSG auditado; UV/material/Undo/Save/reopen y runtime. Integración del owner a JE dentro del engine sin project Gem install. Después N2 materiales y N4 collision, N8 geología adelantado; no esperar todos los sistemas de juego.

## Prohibiciones

Nada AWS/multiplayer/servidores/backend online. Solo DX12/Vulkan como render. No alias JE=AZ como solución final. Geología está activa; BSP→OpenVDB y RT de render no. Buoyancy continúa DESIGN ONLY. No copiar source/shaders de Cry ni borrar otros repositorios/ramas/evidencia.
