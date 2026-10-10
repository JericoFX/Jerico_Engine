# JE — procedencia y dependencias privadas, sin plataformas externas

**Directiva 2026-10-10. Estado: política y auditoría inicial, no aprobación jurídica/comercial completa.** Ver GOAL, contrato JE y `docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md`.

## Frontera de producto

AWS y multiplayer están EXCLUIDOS: no cuentas/servicios/SDKs Amazon, S3/CloudFront como descarga obligatoria o fallback, GameLift/session/matchmaking/replication/servidores/cloud telemetry ni plataforma online de distribución. No se conservan como optional/deferred. Los content packs/campañas existentes son datos locales; no autorizan comercio/identidad/entitlements remotos.

No añadir plataformas para sustituir AWS. Integrar bibliotecas locales necesarias al motor con versions/hashes/licencias y sin instalación/activación de Gems por el diseñador. El build offline requiere fuentes/paquetes admitidos preaprovisionados y toolchain; no se inventa que el engine no depende del OS/driver/SDK gráfico. Qt/Jolt/Luau/ufbx/Manifold/OpenMesh/Recast/audio/codecs conservan procedencia y pueden existir como bibliotecas privadas dentro de la distribución.

## O3DE no relicencia CryEngine

La licencia raíz O3DE permite Apache-2.0 o MIT para el código cubierto, con condiciones separadas para terceros. Preservar LICENSE/NOTICE/SPDX/créditos y modificaciones debidamente identificadas. `JE::` es identidad de código propio/adaptado, no certificado de autoría original ni permiso para eliminar avisos.

No copiar CryEngine 5.7, source/shaders traducidos de Cry, MNM/SVOGI, binarios/SDKs/assets restringidos, Unreal propietario o addons por tener acceso. Requisitos e ideas observables pueden documentarse; código/math/test de Jerico antiguo necesitan auditoría por archivo/derivación/fixture y licencia. No merge/cherry-pick del historial Cry a esta base. Historial/evidencia previa se conserva en su repo, no acredita resultados aquí.

Ideas de otros motores y publicaciones SIGGRAPH/GDC están permitidas. Por candidata: URL/origen exacto/commit o versión, sección estudiada, qué comportamiento se busca, qué no se copia, código/datos/licencias/patentes pertinentes, coste/beneficio y pruebas. Código de un motor realmente abierto puede reutilizarse si licencia/avisos/condiciones encajan, no solo por nombre de proyecto. Paper accesible no licencia automáticamente su código, assets o redistribución del PDF.

## Inventario obligatorio

Por dependencia: owner JE, fuente/artefacto, commit/versión/hash, licencia efectiva y avisos, transitivas, uso tool/build/runtime, linking estático/dinámico, descarga efectiva, servidor/fallback, exposición a APIs/ABI, deployment y prueba offline. Distinguir catálogo disponible, target configurado, objeto enlazado y módulo realmente cargado. No afirmar ausente por leer gem.json ni presente en runtime solo por hallarla en un catálogo.

La revisión de fuente encontró:
- `.lfsconfig` apunta a CloudFront; no enviar tokens/contenido privado a ese destino.
- `cmake/3rdPartyPackages.cmake` añade variables de entorno antes del default CloudFront e incluye S3. Hay que reemplazar el conjunto efectivo y su fallback, no solo anteponer un mirror.
- AzFramework depende públicamente de AzNetworking; éste menciona OpenSSL/RapidXML/codegen. Clasificar IPC de tools y transporte de juego antes de retirar.
- Recast tiene pin v1.6.0 y hash/commit, pero usa FetchContent; obtenerlo localmente no es lo mismo que ya sea offline.
- WhiteBox requiere Manifold/OpenMesh/Atom/editor/legacy y tests físicos: integrar esas partes bajo JE respetando licencias y eliminando sólo rutas no necesitadas.

Referencia histórica por snapshot sin cambios: Qt/PySide Windows declarados 6.11.2, AWS SDKs en catálogo y OpenSSL antiguo. No declarar esos artefactos descargados o el producto seguro. Auditar versiones efectivas/vulnerabilidades y linkage usados por el producto antes de distribución; no update ciego de branches.

## Qt, SDKs y namespaces

Qt permite opciones comerciales y libres sujetas a condiciones; algunos módulos no están bajo LGPL. Elegir módulos/version/linking y conservar cumplimiento por dependencia real. Integrar no exige static link: no reconfigurar Qt a estático sin resolver requisitos de redistribución/relinking correspondientes. No inventar licencia comercial ni afirmar que distribuir engine obliga automáticamente a liberar todo código del juego.

Tipos Qt, Jolt, Vulkan, DirectX y std conservan sus nombres auténticos dentro de límites privados. No exponer AZ/Gem como API final propia ni disfrazar todo O3DE de dependencia exenta: lo que adoptamos como motor se migra a JE. No renombrar APIs del sistema ni los archivos `.r*` por confundir nomenclatura con wire format.

No Autodesk FBX SDK ni ATL/MFC impuestos. Si un target lo pide, localizarlo y sustituir la dependencia mínima por vía permitida; no descargar un SDK restringido para fingir baseline completo.

## Seguridad y cierre

Inputs fuente/scripts/paquetes/saves son no confiables: bounds/tipos/paths/decompression limits, candidatos y rollback, fuzz donde corresponda. Hash verifica integridad, no firma; crypto/keys sólo con implementación verificada y librerías admitidas, sin servicios online introducidos por el empaquetado. No escribir criptografía propia ni prometer imposibilidad de extracción con claves locales.

Prueba final: grafo efectivo de build/link/deployment y observación runtime, red bloqueada, cache local con hashes, missing-package fail-closed, source-free editor/runtime/cook según fase y tres pares gráficos. Sin listeners/telemetría/red de juego; IPC local de tools acotado y ausente en Shipping cuando no se necesita. SBOM/legal notices/excepciones/source offers según corresponda, revisión profesional para distribución cuando haga falta. No certificado de cumplimiento solo por esta auditoría.

Fuentes primarias: LICENSE raíz del snapshot O3DE; https://doc.qt.io/qt-6/licensing.html ; archivos/SHAs del informe DEP-01..DEP-12. No se publican URLs temporales con tokens ni PDFs ajenos.
