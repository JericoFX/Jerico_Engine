# JericoEngine — JE

Base de desarrollo O3DE adaptada hacia JericoEngine. **Rama única de trabajo: `development`.**

## Dirección vigente

Código propio/adaptado con nomenclatura `JE::`; subsistemas necesarios integrados dentro del engine a partir de sus implementaciones Gem. Sin AWS, multiplayer ni plataformas online. Sólo DirectX 12 y Vulkan como backends gráficos: Windows ambos, Linux Vulkan.

Prioridad: editor/UI Jerico y BSP útil, materiales/formatos propios, colisión, geología nativa y herramientas modulares, con resultados visibles y optimización SIGGRAPH desde las primeras escenas. Luau/Jolt/animación/UI/audio/VFX/IA/OpenXR y demás capacidades locales existentes se conservan. Ideas de otros engines permitidas con procedencia/licencia y prueba de utilidad.

**Estado actual: auditoría y planificación; no se afirma migración de source, build, rendimiento ni independencia de dependencias ya completados.**

## Entradas

[CODEX_START](CODEX_START.md) · [GOAL](GOAL.md) · [AGENTS](AGENTS.md) · [Documentación](docs/README.md) · [Roadmap](docs/ROADMAP.md) · [Contrato JE](docs/architecture/JE_IDENTITY_AND_NATIVE_MODULES.md) · [Auditoría de dependencias](docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md).

El default administrativo de GitHub puede diferir; trabajar explícitamente en development. No crear ramas adicionales ni borrar las históricas. No ejecutar instrucciones de descarga upstream sin revisar endpoints/cache/licencias: .lfsconfig y package defaults heredados requieren adaptación, no uso ciego.

## Procedencia y licencias

El source inicial corresponde a O3DE commit `241c78b8064898a709cf1c5dd3535c6406763ef3`. Copyright de sus contribuidores y condiciones originales se conservan en [LICENSE.txt](LICENSE.txt), [LICENSE_APACHE2.TXT](LICENSE_APACHE2.TXT), [LICENSE_MIT.TXT](LICENSE_MIT.TXT) y avisos de terceros. Jerico no se atribuye autoría exclusiva del código O3DE. La migración a JE no cambia por sí misma obligaciones legales ni relicencia CryEngine 5.7.

Las instrucciones/documentación originales permanecen en el historial de esa base y en el upstream oficial `o3de/o3de`. Los requisitos efectivos del build, toolchain y bibliotecas se inspeccionan según N0; no se reutilizan comandos de un README antiguo como prueba de compatibilidad actual.
