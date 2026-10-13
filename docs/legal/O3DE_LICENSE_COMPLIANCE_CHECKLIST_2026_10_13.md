# JericoEngine — checklist de cumplimiento legal O3DE y terceros

**Fecha:** 2026-10-13. **Repositorio:** `JericoFX/Jerico_Engine`, única rama de publicación `development`.
**Estado:** PLAN DOCUMENTAL / AUDITORÍA DE REDISTRIBUCIÓN **PENDIENTE**. Este documento **no** certifica licencias, builds, dependencia ausente ni autorización comercial definitiva.

**Autoridad y contexto:** `GOAL.md`, `AGENTS.md`, `CODEX_START.md`, `docs/architecture/JE_IDENTITY_AND_NATIVE_MODULES.md`, `docs/legal/PROVENANCE_AND_DEPENDENCIES.md` y `docs/research/JE_DEPENDENCY_AUDIT_2026_10_10.md`. No sustituir esos contratos ni activar trabajo técnico ajeno al slice vigente.

## 0. Alcance y principios

- Auditar por **componente y archivo/artefacto efectivamente utilizado**: código O3DE y Gems absorbidas, paquetes 3P directos/transitivos, Qt/plugins, herramientas, SDK, shaders, codecs, tipografías, escenas, modelos, texturas, muestras, documentación incorporada y código generado.
- Mantener separados **source / build-tools / Editor / Asset Processor-cooker / SDK / Runtime / juego distribuido** y **Windows DX12 / Windows Vulkan / Linux Vulkan**. Estar en catálogo o en el repositorio no prueba que se compile, enlace, cargue o distribuya.
- `LICENSE.txt` de O3DE ofrece **Apache-2.0 por defecto o MIT a elección** para el material cubierto. Ambas permiten modificar, distribuir y comercializar sin obligación general de publicar JE; la elección **no** relicencia material de terceros. Determinar y registrar la licencia elegida y cumplir sus avisos y condiciones.
- Migrar `AZ::` / `Az*` a `JE::` / `JE_*`, integrar Gems en el núcleo y retirar AWS/multiplayer es compatible con una base permisiva **siempre que** se mantengan las obligaciones de procedencia. No reescribir copyright/SPDX/NOTICE para aparentar autoría propia ni ocultar dependencias reales.
- O3DE y Linux Foundation conservan sus marcas. JericoEngine es una identidad independiente: atribución técnica/legal correcta, sin logos ni mensajes que impliquen afiliación o respaldo oficial.
- No reutilizar código, shaders, SDK ni assets propietarios de CryEngine/Unreal u otras fuentes sin autorización verificable; una idea/paper público no licencia su implementación. Auditar también aportaciones procedentes de repos anteriores.
- **Sin AWS ni multiplayer** según GOAL: esta exclusión se comprueba en los artefactos reales, no solamente quitando nombres de Gems. No descargar paquetes/LFS por endpoints AWS heredados ni transmitir credenciales.

## 1. Lista de verificación para Codex (todo sin marcar hasta reunir evidencia)

### LIC-01 — Licencia base y procedencia

- [ ] Confirmar la base O3DE (commit/tree) usada por `development` y conservar el texto original de `LICENSE.txt`, `LICENSE_APACHE2.TXT`, `LICENSE_MIT.TXT` y los NOTICE/cabeceras aplicables.
- [ ] Registrar la elección Apache-2.0 o MIT para código O3DE cubierto; listar cualquier archivo/componente excluido de esa elección por licencia independiente.
- [ ] Revisar los archivos JE modificados desde O3DE: avisos de copyright, SPDX, atribución original y señalamiento de cambios exigible (especialmente si se usa Apache-2.0). No afirmar que el renombrado elimina esos derechos.

### LIC-02 — Inventario completo de código, Gems y transitivas

- [ ] Inventariar **todas las Gems efectivamente absorbidas o usadas** con fuente exacta, SHA, licencia, owner JE, targets, transitive deps, generated files, plugins/resources y destino (Editor/tools/SDK/Runtime).
- [ ] Inventariar bibliotecas vinculadas de forma estática/dinámica, DLL/SO cargadas, paquetes locales, hashes y código/recursos realmente presentes en los paquetes de distribución. Separar *declarado*, *configurado*, *enlazado*, *cargado* y *distribuido*.
- [ ] Ejecutar `scripts/license_scanner` si funciona en el checkout para recopilar avisos/SPDX; **reconciliar manualmente** con CMake, PackageInfo, source, linker/imports, deployment, LFS y archivos fuera del escáner. El escáner no demuestra por sí solo cobertura completa.
- [ ] Registrar licencia exacta, excepciones y obligaciones para cada transitiva; ausencia de LICENSE/SPDX o ambigüedad = `BLOCKED` hasta aclarar, nunca «dominio público» implícito.

### LIC-03 — Copyleft, Qt 6, herramientas y componentes restringidos

- [ ] Auditar módulos **Qt 6 realmente utilizados**, plugins de plataforma, imageformats, contribuciones 3P, versión/build, licencia de cada módulo y método de enlace/distribución. No asumir que todo Qt es LGPL ni que enlace dinámico resuelve automáticamente el cumplimiento.
- [ ] Si hay LGPL: documentar derechos de sustitución/re-enlace aplicables, atribuciones, licencia, disponibilidad de fuente correspondiente y cambios en la propia biblioteca; revisar el tipo de producto y la distribución concreta. Si no se cumplen, evaluar licencia comercial u otra biblioteca.
- [ ] Localizar GPL/AGPL, LGPL, licencias con excepción de runtime, bibliotecas propietarias y componentes con cláusulas especiales; analizar *por uso y distribución*, incluidas herramientas solo de build. **No** integrar GPL/AGPL en núcleo/shipping sin revisión jurídica/arquitectónica expresa.
- [ ] Verificar condiciones independientes de Jolt, Luau, ufbx, Manifold, OpenMesh, Recast, codecs/audio, OpenSSL y restantes paquetes **efectivamente presentes**. No presuponer licencia a partir del nombre.

### LIC-04 — AWS, red y origen del software

- [ ] Inspeccionar `.lfsconfig`, CMake/FetchContent, package catalogs, manifests, hooks, scripts, bootstrap, cook y deploy: URLs reales, fallback, descargas, tokens, repositorios y licencias. Controlar tanto origen de paquetes como licencias de paquetes obtenidos.
- [ ] Probar con cache 3P/LFS permitida y red bloqueada que configure/build/cook/arranque no necesitan AWS/S3/CloudFront ni plataforma de cuentas/servicios; simular paquete ausente sin descarga silenciosa. Documentar con evidencia el límite de cada prueba.
- [ ] Clasificar el IPC local de Editor/AP frente a dependencias de red de juego; confirmar qué bibliotecas permanecen por consumidores legítimos antes de retirar o declarar ausentes.

### LIC-05 — Assets, contenido y otras fuentes

- [ ] Inventariar texturas, materiales, fuentes, ejemplos, modelos, audio, shaders y escenas que JE redistribuye; revisar autor, origen, permiso, licencias comerciales, atribución y derecho a modificar/redistribuir, por asset/pack.
- [ ] Excluir o licenciar expresamente contenido de terceros restrictivo (p. ej. ejemplos que dependan de licencias separadas de proveedores DCC/asset packs); los assets importados por usuarios **no** adquieren automáticamente licencia O3DE.
- [ ] Para ideas/código tomado de otros motores, papers, snippets o asistencia de IA, registrar procedencia y revisión de derechos; evitar copiar material propietario de CryEngine histórico solo por haber sido evaluado antes.

### LIC-06 — Entrega y avisos de licencia

- [ ] Producir un manifiesto SBOM/SPDX o equivalente con **artefactos, versiones, SHA256, source URL/commit, SPDX efectivo/excepciones, dependencia transitiva, tipo de enlace, ruta/propietario y distribución destino**.
- [ ] Generar `THIRD_PARTY_NOTICES` y adjuntar textos completos/copyright/NOTICE y mecanismos de cumplimiento exigibles **a cada paquete real** (Editor, herramientas redistribuidas, SDK, Runtime y juegos), sin incluir advertencias falsas sobre cosas eliminadas.
- [ ] Comprobar licencias/avisos dentro del instalador, ZIP/SDK y layout Shipping final (Windows y Linux cuando aplique), no solo en el checkout de fuentes; verificar también Qt plugins, source offers/instrucciones de sustitución si son necesarios.
- [ ] Separar derechos de marca, derechos de autor, patentes y condiciones de SDKs/drivers/redistribuibles; revisión humana de las restricciones materiales antes de publicar/comercializar.

### LIC-07 — Decisión de distribución y mantenimiento

- [ ] Resolver cada entrada del inventario como `PASS`, `BLOCKED` o `NOT_TESTED`, con evidencia rastreable, responsable, fecha, commit, plataforma y producto destino; no cerrar por grep, README o scanner aislado.
- [ ] Emitir un dictamen por **Editor**, **SDK/tools**, **Runtime** y **juego/asset packs**. Cualquier licencia sin resolver en un artefacto efectivamente distribuido mantiene **LEGAL RELEASE BLOCKED** para ese artefacto.
- [ ] Añadir control de regresión en CI/release: detectar dependencia o asset nuevo, pérdida de aviso/atribución, cambios de licencia o packaging, paquetes remotos inesperados y modificación de fuente LGPL. Generar nuevos informes por versión.

## 2. Política de decisión para componentes concretos

| Familia | Política por defecto | Condición para PASS |
| --- | --- | --- |
| Apache-2.0 / MIT / BSD / ISC / Zlib / Boost y similares | Normalmente admitidas | Texto/copyright/NOTICE, condiciones específicas y procedencia preservados. |
| LGPL (por ejemplo ciertos módulos Qt) | Revisión obligatoria | Obligaciones de LGPL y forma real de enlace/modificación/distribución satisfechas, documentadas y verificadas. |
| GPL / AGPL y excepciones especiales | Bloqueo preventivo en núcleo/shipping | Análisis legal de la obra/distribución concreta y excepción aplicable; herramientas externas pueden evaluarse por separado. |
| Propietaria / freeware / asset marketplace | Bloqueada por defecto | Licencia/permiso explícito para **este uso** y distribución, con pruebas. |
| Sin licencia identificada o procedencia dudosa | BLOCKED | Evidencia verificable de derechos y condiciones. |

**No confundir** «licencia abierta», «código disponible», «GitHub público», «Gem oficial» o «no requiere AWS» con un permiso general de redistribución.

## 3. Plantilla mínima de hallazgo y evidencia

Cada hallazgo (identificador `LIC-XX` y subidentificador) debe guardar:

| Campo | Contenido requerido |
| --- | --- |
| Identidad | Nombre, componente/asset, fuente repo+commit/tag, licencia SPDX + excepciones, copyright. |
| Uso JE | Owner, archivos/targets/Gem origen, consumers, tool/source/link/deploy/load, SHA256, directas y transitivas. |
| Destino | Windows DX12, Windows Vulkan, Linux Vulkan y SKU: Editor/SDK/AP-cook/Runtime/juego/assets. |
| Prueba | Ruta a logs/manifest/link map/instalador y SHA de checkout, proceso/fecha/resultado; enlaces estables sin secretos. |
| Resultado | `PASS` / `BLOCKED` / `NOT_TESTED`, condición legal pendiente y revisor humano cuando corresponda. |

Entregables futuros **cuando se ejecuten**, no inventados por este documento: inventario de dependencias y assets, matriz por distribución, avisos `THIRD_PARTY_NOTICES` basados en artefactos reales, expediente Qt/LGPL y registro de decisiones con evidencias en `docs/evidence/`. No generar un «PASS» vacío.

## 4. Criterio de aceptación

- **DOCS PASS:** este checklist existe, está enlazado y su estado sigue pendiente. No acredita build, SBOM, derechos de terceros ni producto.
- **AUDIT PASS (por distribución):** cierre de inventario directo/transitivo y assets realmente distribuidos, avisos, obligaciones copyleft, marcas y permisos acreditados, con artefacto/versión y revisión humana.
- **LEGAL RELEASE BLOCKED:** cualquier dependencia o asset distribuido sin derechos claros o condición incumplida. No bloquear como si estuviera en Shipping una herramienta exclusivamente interna no distribuida: clasificar y justificar antes.
- **PRODUCT PASS:** requiere además gates técnicos en GOAL/AGENTS/Definition of Done. Un informe legal no sustituye Editor/Runtime funcionando.
- Cambios posteriores de versión, proveedor, enlace, módulos Qt, material o packaging **invalidan** el gate legal afectado y exigen reauditoría.

## 5. Referencias primarias para verificar (no sustituyen leer cada licencia)

- O3DE `LICENSE.txt`: https://github.com/o3de/o3de/blob/development/LICENSE.txt
- Licencias incluidas en este checkout: `LICENSE_APACHE2.TXT`, `LICENSE_MIT.TXT`; base documentada en `GOAL.md`.
- O3DE, paquetes y scanner: https://www.docs.o3de.org/docs/user-guide/build/packages/
- Origen SPDX/paquetes O3DE: https://github.com/o3de/3p-package-source
- Qt 6, licencias por módulo: https://doc.qt.io/qt-6/licensing.html
- Marcas O3DE: https://o3de.org/brand-guidelines/
- Guía de marcas Linux Foundation: https://www.linuxfoundation.org/legal/trademark-usage

> Nota: lista técnica de cumplimiento; no equivale a asesoramiento jurídico ni a revisión profesional de una distribución comercial concreta.
