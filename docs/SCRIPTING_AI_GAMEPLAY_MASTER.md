# Luau, gameplay, HTN y persistencia

Estado: contrato de destino para N3/N6/N9/N10. Fuentes históricas: SCRIPTING_GRAPHCORE_GAMEPLAY_MASTER y WORLD_AI_STREAMING_MASTER de JericoEngine2 en el SHA fijado en GOAL. No se copia implementación de Cry ni se restaura MNM.

## 1. Scripting y reflection

Luau `--!strict` es la VM de gameplay y UI Jerico. El binding C++ valida contexto, thread, handle, generación y capacidad; un assert de debug no es el único control de seguridad de una entrada inválida. Error recuperable debe producir diagnóstico acotado, no acceso inválido en Release.

Una reflexión/registro de propiedades/comandos alimenta Inspector, tipos/autocomplete, bindings, nodos, persistencia y MCP. No mantener listas manuales contradictorias. Read/write permissions y cambios que requieren physics/main/render safe points son parte del metadata.

`self` se llena al crear la instancia de script para una entidad/contexto, no a partir de un nombre escrito. Sus referencias tipadas apuntan al mismo sistema de identidad del editor. Unload/reload/destrucción cancela subscriptions/timers; closure vieja no puede escribir sobre un entity ID reciclado.

Ejemplo de intención, no firmas C++ ya existentes:

```luau
--!strict
-- El editor crea/reutiliza self.references con tipo y GUID estables.
function Script:onPressed(activator)
    local lamp = self.references.Lamp
    if lamp == nil then
        return
    end
    lamp:setEnabled(false)
end
```

Las firmas reales se generan del registro probado. No copiar este ejemplo para inventar otra API de referencia. El drag Outliner→IDE debe reutilizar el binding compatible y conservar Scene/PrefabInternal, GUID, scopes y undo; no emitir paths o cadenas frágiles.

## 2. Commands, Events, Store y triggers

Commands solicitan una operación; el owner decide y publica un Event del resultado confirmado. Script no modifica directamente transforms de cuerpos dinámicos ni renderer internals. Acceso por GUID/proximidad consulta la autoridad canónica, con orden/filtros definido y handles seguros.

Store registra cambios nativos confirmados de propiedades incluidas por el autor. Inspector permite opt-in y muestra qué se guardará. Estado de proyecto/juego/sesión separado de preferencias de usuario. No duplicar `Store.set` tras cada mutación ni serializar tablas/closures/userdata arbitrarios.

Trigger = fuente + filtro + política + destino: shapes/volumen/proximidad, click/puntero, acción input, evento, condición o activación manual. Registrar activator/receptor, Once, cooldown, enabled y filtros de especie/capacidad. Once tiene persistencia definida; reentrancia, desactivar durante callback, destroy y load no disparan dos veces.

## 3. Input, Player y posesión

Acciones semánticas compartidas por teclado/mouse/gamepad/XR; contexts/prioridad, rebinding/conflictos, processors/deadzones/curvas, hot-plug y cleanup al perder foco. Glyphs reflejan origen real; haptics se cancelan al perder sesión/dispositivo. User settings y replay usan la misma semántica.

Player/Enemy/Neutral son roles de entidades con capacidades, no tipos duplicados de World. Posesión separa input/cámara/UI del cuerpo/controlador. Camera/Sequencer solicita control explícito y lo devuelve al finalizar/cancelar/destruir; una secuencia de cámara no inmoviliza al jugador por accidente.

## 4. Items, inventario, narrativa y perfiles visuales

ItemDefinition/Registry y Item Studio definen identidad, pickup instantáneo o físico, stack/slots/capacidades/metadata. Inventory posee transacciones; HUD, mano, mochila y preview 3D son representaciones, no inventarios distintos. Pickup y swap de mano se resuelven una vez y se prueban con referencias destruidas/full inventory.

Dialogue/choices, Quest/Objectives y Journal/codex tienen identidad, localización, eventos y participación en save. Narrative Studio valida referencias y estados inaccesibles. No convertir UI en autoridad de quests.

Entity Presentation Profiles son una capacidad general: cambian representación visual/animación/audio/VFX y anchors compatibles según perfil, sin duplicar IA ni estado de gameplay. Arachnophobia es un uso posible, no el nombre de un sistema especial por enemigo. Cualquier cambio de colisión se trata explícitamente como cambio de gameplay/physics, no como detalle cosmético silencioso.

## 5. Save/Load y reload

Modelo de proveedores con schema IDs/versiones, límites, migraciones y snapshot de valores permitidos. Restaurar en candidato, validar cierre/referencias y publicar transaccionalmente. `.rsave` no contiene punteros ni bytecode arbitrario no validado. Guardado atómico con backup/recovery cuando se exponga; error mantiene el último save válido. Separar integridad, autenticación y confidencialidad; no prometer protección contra el propietario de una máquina local.

Hot reload Luau: typecheck/cook de candidato, límites de CPU/memoria, suscripciones con generación, cancelación y rollback. Shipping consume producto admitido, no ejecuta compilación fuente silenciosa. Tools Lua/Python heredados se clasifican separadamente; no convertir material functors en un segundo gameplay VM.

Replay de QA guarda acciones semánticas, seeds/checkpoints y versiones de build/contenido. No prometer determinismo bit a bit entre plataformas sin pruebas; detectar y explicar divergencia. Replay de gameplay no es el historial del debugger de IA.

## 6. Un GraphCore, semánticas separadas

Reutilizar GraphCanvas/GraphModel como frontend común: nodes/pins/edges/subgraphs, search/groups/comments, copy/paste, undo/versioning, validation y debugger. Material/VFX/animation/HTN/audio tienen compilador/evaluador de su dominio. Un framework visual común no justifica una VM universal nueva.

Gameplay Graph baja a Commands/Events/orquestación tipada del mismo runtime. HTN Graph produce el mismo modelo canónico que texto admitido; no segundo behavior tree tickeado. No prometer round-trip arbitrario perfecto entre texto editado y graph sin parser/sincronización demostrados.

## 7. Navigation y HTN

Navigation responde cómo llegar; HTN decide qué intentar. Recast/Detour es candidato desde O3DE. Verificar provider de geometría/colliders y desacoplar PhysX al migrar Jolt. Navmesh de suelo no prueba navegación por paredes/techos de arañas: esos modos requieren links/superficies y locomoción expresamente implementados.

Percepción → memoria/conocimiento → facts/relaciones/grupos/zonas → planner HTN → acciones primitivas → Navigation/Jolt/Animation/Luau → eventos de resultado. Una autoridad por etapa. No movimiento oculto por planner.

Las condiciones especulativas del planner son puras; no dañan, guardan ni aprenden durante backtracking. Acciones cancelables con timeout/resultados; queries caras asíncronas fuera de expansión bloqueante. Budget por entidad y grupo, wakeups por eventos/dirty facts; no polling completo de todo el mundo por frame.

Relaciones: rol, especie/arquetipo, facción, grupo/manada/colonia, territorio, overrides por individuo y contexto. No comparar strings de nombres de entidad para decidir hostilidad. Zonas/nidos son entidades/referencias tipadas, con pertenencia y prioridades observables.

## 8. Aprendizaje explicable y depuración

Memoria individual y de colonia almacena valores acotados, evidencia, confianza y decay. Actualizar después de resultados confirmados de acciones/eventos. Compartir información requiere regla de comunicación/proximidad/grupo; no omnisciencia entre todos los enemigos. Guardar esos datos en los proveedores de Save; no serializar internals opacos del planner.

Escenarios obligatorios: araña detecta comida/cuadrúpedo, reserva presa, ataca y arrastra al nido por Jolt/Nav; perros atraen araña ciega con sonido y coordinan ataque; araña/colonia elevan sospecha tras emboscada y cambian el plan. Competencia por la misma presa debe resolverse sin dobles reservas. El player es un actor más en el modelo de relaciones, no sustituto de todos los targets.

Editor selecciona actor y muestra brain, sensores, facts, objetivo/branch/acción actual, alternativas rechazadas, causa, target, reservas, memorias y quién bloquea la acción. Vista simple Goal/Method con branches ordenadas y lista WHEN/DO; expandir a nodos conserva semántica. Breakpoints y source IDs apuntan a la ejecución real. Texto/iconos además de colores.

## 9. Pruebas de aceptación

- N3: luz enlazada por drag tipado; click filtrado; propiedad persistente; Undo/reopen/cook/runtime y handle destruido/reciclado.
- N6: inventory/mano/UI comparten estado; guardar/cargar restituye items, progreso y Once; corrupción/cancel rollback.
- N9: dos agents y target compiten, nav repath por obstáculo, acción cancelada y unload sin acceso inválido; presupuesto del planner medido.
- N9: los escenarios de caza/bait/contramedida se explican en debugger y sobreviven save/load; la planificación no cambia el mundo especulativamente.
- N10: periférico perdido, foco perdido, cambio de posesión y secuencia cancelada no dejan input/haptics/cámara atascados.

Código C/HTN anterior y repos Sandruski/htn-planner/urosidoki/htn_planner permanecen candidatos de procedencia/licencia, no dependencias automáticamente admitidas. Leer las fuentes antes de fijar backend exacto; conservar un solo contrato de planner.
