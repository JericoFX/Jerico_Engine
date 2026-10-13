# Workspace nativo de grafos y distribución SDK

Estado del corte: autoría y reapertura verificadas en el Editor; instalación,
importación, enlace y carga de tres módulos del SDK verificadas. N0 y el GOAL
completo siguen activos.

MaterialGraph, sus configuraciones de nodo, PassGraph y PassNode usan el workspace
de `JE::Editor`, el Inspector y el historial del Editor. Shader se abre como
documento existente. Se trasladan datos de grafos, recursos, registro y presets
de preview a los owners nativos; no se activan MaterialEditor, MaterialCanvas ni
PassCanvas para estas sesiones. Los targets fuente de compatibilidad se conservan.

| Comprobación | Resultado real | Evidencia |
|---|---|---|
| Layout con documento/widgets reales, dos tamaños | Mismo test final: RED original; GREEN. 148 casos únicos, cero fallos, errores, disabled o skips; ocho fases exit 0 | [Manifiesto UI/AP/tests](overnight-ap-ui-layout/manifest-rev2.json) |
| Save/reopen de Pass/nodo y materiales | Dos procesos nuevos del Editor, Windows DX12 y Vulkan; UUID, Comment y título conservados. 38 inputs previos y dos documentos privados intactos | Capturas originales 1920x1032 y recibos del manifiesto |
| Configuración de distribución | Tres owners físicos distintos; 231 opciones productivas idénticas; seis aplicaciones excluidas de ALL, exports e instalación SDK | `sdk-profile-configure-attempt6` dentro del archivo de evidencia |
| Instalación completa Profile | Exit 0, 56,953 s; 11.063 archivos, 2.859.198.385 B; 30.906 filas contenidas/presentes. Bytes y hashes idénticos a install4 | `sdk-profile-install-attempt5` |
| Consumidor del SDK instalado | Configure 0/71,250 s; build 0/7,154 s; loader 0/0,523 s. Importa `JE::EditorMaterials`, `JE::EditorCamera` y `JE::EditorRenderIntegration` | `sdk-profile-import-smoke-attempt6`, revisión del padre incluida |
| Limpieza | Cuatro prefijos privados obsoletos retirados: 44.336 archivos, 11.437.827.904 B lógicos. Build original, SDK vigente y evidencia conservados | [Recibo](sdk-install-import/obsolete-sdk-prefix-cleanup-result.json) |

Los logs, argv, resultados nativos, manifiestos, fuentes privadas, pins y recibos
RED/GREEN del SDK están en [sdk-validation-raw-rev1.zip](sdk-install-import/sdk-validation-raw-rev1.zip).
Su [manifiesto](sdk-install-import/archive-manifest-rev1.json) registra 308 entradas,
65.638.443 B descomprimidos y SHA-256
`A5BF658ACAC502A2D34C031DC4A2959B99D3FC8E1ED522C9438CA803E546E3BF`.
Cada entrada se reabrió y verificó por hash. No contiene los binarios del SDK.

Consumer4 falla C1112 al declarar el target fuera del contexto de flags del
engine. Consumer5 corrige únicamente la fixture con `ly_add_target` en el proyecto
añadido por el engine y confirma preprocesador conforme, MD y C++20; luego falla
C1083 en un include relativo. El header sí estaba instalado y era byte idéntico.
Consumer6 conserva las cuatro fuentes y todo el SDK, usando prefijo y build
cortos; compila, enlaza y carga. Esto discrimina el problema de rutas, aunque no
aísla su mecanismo exacto porque ambas ubicaciones cambiaron.

El loader utiliza sólo SDK/bin/Windows/profile/Default y Windows en PATH; las
tres DLL junto al EXE coinciden con el SDK. Resuelve exports de factories, sin
activarlas. El postflight conserva 1.563 pins de instalación, 11.998 fuentes,
139 inputs de proyecto, nueve payloads y cuatro fuentes privadas. Las 44 adiciones
de configuración corresponden al mismo tooling ya auditado; el resto del SDK
no cambia. Qt/OpenSSL locales siguen siendo inputs externos autorizados. Python
de configuración/codegen es tooling externo; esta prueba no introduce Python
en el Editor, juego ni módulos cargados.

Límites abiertos: generar y aplicar pipelines Pass, corregir el doble clic de
archivos sin SourceUUID, cook global del AP y SDK reproducible en equipo limpio,
separación y presupuestos Shipping, Beauty/movimiento, DPI físico, Linux Vulkan
y VR. El SDK conserva 558 pointers LFS; el batch global AP falla en 347 fuentes
(337 OIDs). Los jobs frescos del material nativo son evidencia focal de AP.
No se descargaron ni materializaron esos pendientes. La captura y carga del SDK
no acreditan eliminación total de Gems, rendimiento ni aceptación global.
