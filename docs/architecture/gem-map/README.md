# Gems: mapa de dependencias, registro y owners nativos

Abrir [el mapa completo](map.md). Contiene una vista general, quince familias, el registro por host y los destinos nativos de las absorciones identificadas. Los diagramas individuales están en archivos .mmd; las tablas enlazan los datos completos.

La captura corresponde al checkout autoritativo F:\JericoEngine\JericoEngine, rama development, base publicada 4e26562ae9dc05d4c013ffb9d959cc19b2435d68 más el candidato Bootstrap del freeze94, el 16/10/2026. Se conservaron 110 manifests fuente, 14 plantillas/fixtures separados, 4.066 referencias tipadas y 51 sitios de factory. Una dependencia declarada, un plan de carga y una llamada verificada tienen tipos y fases diferentes.

- [Inventario y estados](catalogue.json): las Gems presentes, owners nativos, wrappers y límites.
- [Todas las relaciones](edges.json): origen, destino, tipo, fase, archivo, línea y resolución.
- [Puntos de registro C++](factory-call-sites.json) y [planes de módulos](configured-module-composition.json).
- [Reconciliación de las once identidades históricas](historical-nominal11.json), sin añadir aceptaciones.
- [Cobertura y comprobaciones](validation.json), [fuentes de la captura](input-pins.json) y [recibo del productor](receipt.json).

Comprobación realizada: 110/110 manifests representados, IDs y extremos válidos, dieciocho diagramas con un máximo de 51 aristas por diagrama y fuentes sin deriva. La validación es estructural; no se ejecutó un renderer Mermaid ni se instaló ningún paquete. El mapa no acredita llamadas C++ indirectas universales, ejecución de todos los módulos ni aceptación de producto.

Al actualizar una integración, contrastar primero sus productores, consumidores y ciclo de vida con Graft vigente; revisar también los manifests relativos y la composición generada. Actualizar las relaciones afectadas con archivo, línea, fase y evidencia. Conservar como pendientes los enlaces que no puedan resolverse. La presencia de metadata histórica no habilita una Gem ni cambia el GOAL; Vulkan y VR/OpenXR siguen vigentes, y AWS/multiplayer/Python productivo continúan excluidos.