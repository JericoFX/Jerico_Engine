# Integración nativa de geometría y materiales

Implementaciones trasladadas a `Engine/Geometry`, `Engine/Render/Materials` y
`Editor/Materials`, con namespaces JE, factories compuestas por los hosts y
consumidores actualizados. WhiteBox deja de ser un Gem activable. Sus recursos
históricos se conservan; UUID, serialización y scan-folder identity se mantienen.

La validación se ejecutó en el checkout de desarrollo con el baseline local de
retiro de Python y las correcciones HDR pendientes. Esta publicación delimita la
absorción de geometría/materiales: no acredita que un checkout limpio del commit
reproduzca por sí solo ese baseline, ni publica las dependencias adquiridas.
Los hunks de retiro global de Python y de LUT se conservan fuera de este commit.

## Evidencia

- `build-result.json`: siete productos y cuatro targets de pruebas, Profile,
  exit 0; 188,807 segundos. No es una medición de compilación limpia.
- `ctest-summary.json`: cuatro CTests, con 609 casos en sus JUnit originales
  (1 Geometry, 577 Authoring, 1 Physics y 30 CommonFeatures Editor), sin fallos.
- `input-revalidation.json`: 17.139 entradas comprobadas; cuatro diferencias de
  documentación, sin cambios de código respecto a ese build. Posteriormente se
  retiraron únicamente espacios finales/líneas vacías de cuatro archivos:
  `publication-whitespace-fix.json`. No se repitió el build por ese cambio de
  formato, que conserva el texto de cada línea de código.
- `exports.json`: exports explícitos. Geometry/Authoring suman 5.665.792 bytes,
  3.451.904 menos que la versión con exportación automática. El conjunto
  Geometry/materiales/CommonFeatures todavía supera al original en 1.929.728
  bytes: esto no prueba una reducción global del engine ni del juego final.
- `face-material-ui.json` y tres imágenes originales: selección y asignación de
  metal a una cara, Undo/Redo, Save/reopen y modo juego Vulkan. El diff del prefab
  sólo modifica el slot de una superficie y sus cinco triángulos; conserva UV y
  geometría. La fixture original quedó intacta. Las capturas no son un benchmark.

La evidencia detallada y los prefabs antes/después permanecen bajo
`build/n0-validation/native-face-material-vulkan-attempt1`. El build y los JUnit
originales están bajo `native-geometry-materials-integration-rev1/native-attempt10`.

## Pendiente

Preservar UV manuales durante operaciones geométricas que hoy se rechazan;
validación standalone, colisión, VR/headset y Linux; paridad visual DX12/Vulkan;
distribución offline y métricas completas. Material Editor abre con recursos
originales recuperados y verificados por hash, pero esos payloads LFS no se
redistribuyen aquí. Los fallos previos de thumbnail/DiffuseProbeGrid siguen
abiertos. Otros Gems y el baseline local aún deben integrarse/publicarse. N0,
N1 y el GOAL completo no se consideran terminados.
