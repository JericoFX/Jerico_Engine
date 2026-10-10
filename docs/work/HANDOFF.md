# Handoff — transición documental O3DE/JericoEngine

Fecha: 2026-10-10. Única rama: development. Estado de implementación nuevo: **NOT VERIFIED**.

## Hecho en esta publicación

Auditoría remota de las 23 ramas, sus HEADs y comparaciones; comprobación del SHA/tree base contra o3de/o3de; lectura del GOAL/AGENTS y contratos relevantes de JericoEngine2, atlas/idTech6/PWAS de JericoEngine1 y manifests/configuración O3DE. Redacción de objetivo, roles, arquitectura, masters, mapa de rescate, 83 requisitos, SIGGRAPH y plan N0.

El commit es documental: no contiene engine implementation, shaders, Gems migrados, dependencias instaladas ni pruebas de runtime. Sus documentos se autoidentifican como diseño o instrucciones de futura ejecución. Consultar VERIFICATION_REPORT para cobertura/limitaciones.

## Base y fuentes

O3DE `241c78b8064898a709cf1c5dd3535c6406763ef3`; árbol `db4b7d4a5d784d2283c95903f461e311d69704c8`. Historico JericoEngine2 `304d9d2e13704e4967e82a8d5a13a5a506be01d4`. El SHA de esta publicación se obtiene de `git log` tras sincronizar; no incrustar un hash autorreferencial inventado.

## Decisiones consolidadas

Development único; ramas antiguas lectura, sin merge/delete. O3DE/Atom/Vulkan/Qt6, módulos nativos; no Gem Jerico instalable ni engine paralelo. Jolt/Luau/OpenXR, EMotionFX/Maestro/UI/audio/VFX a auditar/adaptar; una autoridad de World/identity/assets/physics/render. Rescate de requisitos/SIGGRAPH, no código Cry. Qt6/PySide actuales declarados; no nueva migración Qt5 supuesta.

Vulkan Windows/Linux, DX12/RT diferidos; underwater y costa después; agua Cry cerrada históricamente no acredita O3DE. Geología integral inactiva; RMod/cables/edge/bake/daño/PWAS/EPLS vivos. Flotabilidad Enable Buoyancy DESIGN ONLY, sin permiso por esta transición. XR considerado en contratos desde N0, prueba de visor N10.

## Pendiente inmediato para Codex

Seguir CODEX_START y N0_FOUNDATION_PLAN. Verificar checkout/status/remotos y default branch. Al inicio el default era optimize_scriptcanvas_1; esta auditoría no dispuso de acción administrativa para cambiarlo. Ajustarlo con CLI autorizado y confirmar; development puede usarse explícitamente mientras tanto.

Auditar LFS CloudFront y 3P antes de materializar/subir privados. No se resolvió alojamiento independiente ni se descargaron todos los paquetes. Registro de licencias es inicial, no SBOM total ni aprobación legal.

Crear evidencia real del baseline, elegir plataforma disponible y medir. No continuar la reparación sky de Cry ni declarar O3DE rápido/compilado. Mantener el repo histórico intacto.

## Riesgos abiertos

Dependencias/transitivas y Qt/linking; AWS build-hosting; baseline development no estable probado; Preview OPS; RT oculto en probes; navegación/provider de física; UI migration/formatos; CMake/README de épocas distintas; coste de sustitución de PhysX y Lua gameplay. Cada riesgo tiene owner/etapa en masters y matriz, no se trata como hecho resuelto.
