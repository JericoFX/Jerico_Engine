# Consulta upstream — Scene100

Fecha: 17/10/2026. Alcance: absorción de CommonFeaturesAtom (AtomLyIntegration/CommonFeatures), con Atom_Feature_Common como dependencia conservada. Investigación focal de Luna, revisada por el padre; ninguna solución upstream se importa automáticamente.

| Fuente primaria | Revisión consultada | Aplicabilidad y decisión |
| --- | --- | --- |
| [O3DE PR3098](https://github.com/o3de/o3de/pull/3098), [merge](https://github.com/o3de/o3de/commit/2cfbaa956ab9d2805a5fa657f68379890abf69a9) | Fusionado, 2cfbaa956ab9d2805a5fa657f68379890abf69a9 | Unificación previa de documentos de Material Editor/SMC en AtomToolsFramework. Referencia para evitar otro sistema de documentos; no resuelve el registro de componentes Scene. Descartar su adaptación en este corte. |
| [O3DE PR866](https://github.com/o3de/o3de/pull/866), [merge](https://github.com/o3de/o3de/commit/50f5976e59d212e48dc641504d0bc80691c1d874) | Fusionado, 50f5976e59d212e48dc641504d0bc80691c1d874 | Movimiento del controlador de cámara a AtomToolsFramework. Referencia acotada al viewport; no cambia el owner ni bootstrap Scene. No importar. |
| [O3DE PR20109](https://github.com/o3de/o3de/pull/20109) | Abierto al consultar, sin merge SHA | Cambios de latencia/determinismo de grafos y Asset Processor. Reservar para un corte causal de grafos/AP; no adoptar para absorber estos componentes. |
| [Issue13818](https://github.com/o3de/o3de/issues/13818) y [RFC124](https://github.com/o3de/sig-graphics-audio/issues/124) | Issue/RFC, no PR | Contexto sobre materiales y separación core/AtomLy. No demuestran una causa local ni constituyen una solución importable. |

La búsqueda focal no encontró un PR inequívocamente aplicable al registro de CommonFeaturesAtom; no demuestra que no exista. Se sigue la composición nativa existente y se verifican los contratos locales. [RFC51](https://github.com/o3de/sig-graphics-audio/issues/51) queda como contexto de la generalización previa de herramientas, fuera de implementación Scene100.

El grafo local vigente muestra Scene -> RenderMaterials -> RenderIntegration y EditorScene -> EditorMaterials -> EditorRenderIntegration. Absorber los cuerpos directamente en RI/ERI cerraría ciclos. La composición seleccionada usa bibliotecas estáticas Scene y contratos públicos sin retorno a los cuerpos, con las factorías dentro de los hosts existentes; no añade una DLL de producto ni otro sistema de materiales.
