# Procedencia, licencias e independencia de servicios

Estado: **revisión inicial de fuentes; no aprobación jurídica de distribución**. Prioridad del propietario: base legalmente modificable. Repo privado no equivale a permiso para copiar software ajeno.

## 1. Base legal observada

`LICENSE.txt` del O3DE base permite elegir Apache-2.0 o MIT para el código cubierto por esa licencia, e indica condiciones separadas para terceros. Se conservaron LICENSE.txt, LICENSE_APACHE2.TXT y LICENSE_MIT.TXT sin modificación.

Base técnica comprobada contra el upstream oficial: commit `241c78b8064898a709cf1c5dd3535c6406763ef3`, tree `db4b7d4a5d784d2283c95903f461e311d69704c8`. Esto acredita identidad del snapshot de código Git, no descarga/completitud/licencias de todos los objetos LFS o paquetes transitivos.

O3DE tiene historia de Lumberyard, pero no es una autorización para usar todo CryEngine 5.7 ni paquetes originales de Lumberyard bajo sus condiciones antiguas. Código con nombres Cry/Amazon dentro del snapshot licenciado debe evaluarse por su procedencia concreta. No borrar atribuciones para ocultar esa historia.

No decidir la licencia de las contribuciones originales de Jerico mediante un replace global del LICENSE de O3DE. Cualquier distribución comercial requiere cerrar el inventario aplicable y revisión jurídica cuando corresponda.

## 2. Frontera de rescate desde repositorios anteriores

| Material | Tratamiento |
|---|---|
| Requisitos originales del propietario, UX deseada y decisiones | Reexpresar en las nuevas especificaciones con referencia histórica |
| Métricas/logs/capturas de la antigua ejecución | Conservar como evidencia histórica en el origen; nunca acreditar O3DE con ellos |
| Código original de Jerico previo a Cry, tests y matemáticas | Candidato, no aprobado en bloque; verificar autoría, imports, fragmentos derivados, fixtures y licencia |
| Código Jerico añadido dentro de CryEngine2 | Cuarentena hasta auditoría por archivo; el namespace propio no demuestra independencia |
| CryEngine 5.7, shaders traducidos desde Cry, adaptaciones de MNM/SVOGI | No importar por defecto; implementación independiente desde fuentes permitidas |
| Assets, texturas, escenas y binarios de Cry/Penumbra/otros motores | No copiar sin derechos demostrados; tests nuevos con fixtures sintéticos o licenciados |
| Papers SIGGRAPH/GDC, diapositivas, implementaciones de referencia | Leer conceptos; permisos de paper, código, datos y patentes se revisan por separado; no republicar PDFs automáticamente |
| Addons Blender propietarios, Ucupaint/AutoTrack/Auto-Rig Pro | Requisitos/experiencia como referencia; no copiar source/nodes/assets ni asumir licencia por disponer de un ZIP |
| UE4/idTech u otro código propietario | Sin traslado de implementación; estudiar publicaciones permitidas, no lavar código por traducción |

No merge de historias de Cry ni cherry-picks entre los repositorios como mecanismo de rescate. No declarar clean-room certificado: esta auditoría consultó documentación del proyecto anterior, parte de la cual describe implementaciones derivadas. Una implementación nueva debe registrar fuentes permitidas y su proceso real.

## 3. Inventario inicial observado

| Elemento | Evidencia de snapshot | Estado y próxima verificación |
|---|---|---|
| O3DE | SHA/tree y LICENSE raíz | Identidad/licencia raíz comprobadas; transitivas pendientes |
| Qt/PySide Windows | Qt 6.11.2 y PySide6 6.11.2 en BuiltInPackages_windows.cmake | Declaración comprobada; descargas/build y módulos/licencias exactos pendientes |
| Qt licensing | LICENSE raíz menciona LGPLv3, enlace histórico Qt5 | No usar ese enlace antiguo como expediente de Qt6; inventariar la versión efectiva, módulos, linking y obligaciones de fuente/relinking |
| DXC Windows | 1.8.2505.1-o3de-rev3 en catálogo | Mantener pin inicial; comprobar artefacto/Source y toolchain, no incorporar update Mac por error |
| OpenSSL Windows | 1.1.1o-rev1 declarado | Auditar uso real y soporte/vulnerabilidades antes de distribución; rama update es candidata, no patch aceptado |
| AWSNativeSDK | 1.11.288-rev1 declarado | Catálogo no demuestra linkage; seguir targets/imports y excluir si no se consume |
| AwsIotDeviceSdkCpp | 1.15.2-rev1 declarado | Igual: inventariar uso y retirar del producto cuando se demuestre cierre |
| MiniAudio Gem | Manifest 0.1.0, licencia Gem Apache/MIT | Biblioteca y codecs tienen expediente propio pendiente |
| RecastNavigation | Manifest menciona Apache/MIT y zlib para Recast; FetchContent | Fijar hash real, licencia y dependencias CMake |
| OpenParticleSystem | Manifest Preview 0.0.0 | No congelar formatos privados de Preview como ABI Jerico sin migración |
| Jolt/Luau/ufbx/KTX2/Basis/OpenXR | Destinos de diseño | No descargados, integrados ni auditados exhaustivamente por este commit |

Este inventario NO es un SBOM completo. N0 debe generar inventario directo/transitivo con nombre, versión, URL, SHA/hash, licencia SPDX, evidencia de licencia, modificados, avisos, uso tool/runtime, linkage/distribución, responsable y estado de admisión. Dependencia sin origen/licencia verificable no se incorpora al producto; el resto del trabajo independiente puede continuar.

## 4. AWS: cuatro verificaciones distintas

1. **Servicios:** ejecutar Editor/Runtime/cook sin cuentas, endpoints AWS, credenciales ni telemetría obligatoria.
2. **SDK/enlace:** grafo de CMake, bibliotecas/imports efectivos y cierre de deployment, no grep de nombres como prueba final.
3. **Tools:** revisar descargas, paquetes, Python, asset jobs y scripts de build.
4. **Hosting:** `.lfsconfig` del base apunta a `https://d3df09qsjufr6g.cloudfront.net/api/v1`; también hay que auditar servidores de 3P. Poder jugar offline no acredita una instalación independiente de Amazon.

Mantener la política de no cuenta/no servicios. Para adquirir el baseline, registrar cualquier uso de un mirror público existente como dependencia de distribución transitoria; no llamar a esa etapa AWS-free. Destino: almacenamiento privado/controlado admitido, hashes comprobados, mirror/caché reproducible y bootstrap sin Amazon obligatorio.

No enviar tokens GitHub o archivos privados al endpoint heredado simplemente porque `.lfsconfig` lo sugiera. Inspeccionar primero `git lfs env`, configuración efectiva y pre-push. Los archivos de esta publicación son Markdown normal y no necesitan nuevos objetos LFS. Para fuentes binarias futuras: identificar dónde se leen los objetos públicos históricos y dónde se suben los privados nuevos; no reescribir historia ni LFS masivamente. Verificar materialización y `git lfs fsck` cuando se hayan obtenido los objetos necesarios.

## 5. Requisitos excluidos y seguridad

No imponer Autodesk FBX SDK ni ATL/MFC nuevos. N0 debe detectar si algún target baseline aún los pide; documentar el target y resolverlo con una vía permitida, sin descargar SDKs restringidos para ocultar el bloqueo. No afirmar ausencia hasta medir el grafo/build.

No activar cloud deployment, compras, secretos, puertos públicos, telemetría o cambios de seguridad del sistema. MCP/RemoteTools locales con capacidades y validación explícitas. Importadores, scripts, paquetes y saves son inputs no confiables: bounds/tipos/path traversal, fuzzing y publicación candidate-first.

## 6. Cierre de distribución

Antes de publicar: inventario/SBOM efectivo del producto, licencias/NOTICE/SPDX y fuentes/ofertas exigidas, materiales/artifacts con procedencia, revisión de módulos Qt usados y linking, dependencias sin soporte o vulnerabilidades relevantes resueltas, zero credenciales y prueba source-free/offline. Hashes acreditan integridad, no autenticidad por sí solos.

Fuentes primarias: https://github.com/o3de/o3de/blob/241c78b8064898a709cf1c5dd3535c6406763ef3/LICENSE.txt ; https://doc.qt.io/qt-6/licensing.html . Los términos se verifican para cada versión realmente redistribuida, no únicamente en estas páginas generales.
