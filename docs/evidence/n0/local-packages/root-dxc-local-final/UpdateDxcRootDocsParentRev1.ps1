#Requires -Version 7.0
#Requires -PSEdition Core
$ErrorActionPreference = 'Stop'
$taskRepo = 'F:\JericoEngine\JericoEngine'
$taskParagraph = 'Último configure registrado: **Root61 argv con freeze15.674, exit1/29,346s por astc-encoder-3.2-rev2-windows ausente offline**, SHA17249bfa438afb34e21449865d9c9297471174ae0cea9b2f9def2ee206038295, requerido por ImageProcessingAtom.Editor.Static. DXC resolvió desde el paquete JE aceptado:15miembros byte exactos más stamp0B; engine-packages6.573→6.589. Revisión actual: freeze15.674/15.674, cache36/wheelhouse44 sin mismatches;41manifiestos postflight sin deltas inesperados. Pico86.941.696B sólo CMake padre, sin hijos ni engine. [Resultado](LINK/local-packages/root-dxc-local-final/attempt1/configure-result.json), [fatal real](LINK/local-packages/root-dxc-local-final/attempt1/configure.stderr.log), [postflight](LINK/local-packages/root-dxc-local-final/attempt1/input-postflight-deltas.json) y [revisión del padre](LINK/local-packages/root-dxc-local-final/parent-native-result-review.json). El receipt conservado no identifica el launcher; el despacho posterior de Luna quedó NOT_RUN por attempt1 existente, sin repetir CMake. Se reutiliza la observación coherente para localizar el siguiente bloqueo, sin atribuirla a ese despacho. No hubo generación completa, build ni frame; los intentos anteriores se preservan como históricos. Incluye candidatos Python/WhiteBox/SMC/SQLite aún no compilados, sin acreditar un baseline de HEAD. ASTC se clasifica por fuente como autoría bajo PAL_TRAIT_BUILD_HOST_TOOLS; separación efectiva del juego por grafo/link/deploy sigue pendiente.'
$taskNext = 'Siguiente ejecución: auditar y construir ASTC3.2 desde fuente permitida con el contrato de Atom, Release/Debug y perfiles/formatos originales; integrar sólo tras consumidores y controles negativos nativos. Preservar DXC ya aceptado y no desactivar ShaderBuilder, DX12/Vulkan ni codecs para evitar dependencias. Continuar Root una vez por entrada nueva con offline argv y freeze revisados; priorizar targets originales Azslc/AssetProcessor/Editor/launcher y WhiteBox.Editor.Tests cuando se genere la solución. No Java ni regeneración de la gramática AZSL versionada.'
foreach ($taskRelative in @('docs/work/ACTIVE_SLICE.md','docs/work/HANDOFF.md','docs/evidence/n0/N0_EXECUTION_2026_10_10.md')) {
    $taskPath = Join-Path $taskRepo $taskRelative
    $taskBytes = [IO.File]::ReadAllBytes($taskPath)
    $taskText = [Text.Encoding]::UTF8.GetString($taskBytes)
    $taskOriginal = $taskText
    $taskPattern = '(?m)^(?:Publicación anterior comprobada: e2b09411f3da26018681e6ce9d03a5c76b319639\. )?Último configure ejecutado: [^\r\n]+'
    $taskMatches = [regex]::Matches($taskText, $taskPattern)
    if ($taskMatches.Count -ne 1 -or $taskMatches[0].Value -notmatch 'freeze nuevo 15\.597') { throw "Expected prior Root paragraph not unique: $taskRelative" }
    $taskLink = if ($taskRelative.StartsWith('docs/work/')) { '../evidence/n0' } else { '.' }
    $taskReplacement = $taskParagraph.Replace('LINK/', "$taskLink/")
    if ($taskRelative.StartsWith('docs/work/')) { $taskReplacement = 'Publicación anterior comprobada: e2b09411f3da26018681e6ce9d03a5c76b319639. ' + $taskReplacement }
    $taskText = $taskText.Replace($taskMatches[0].Value, $taskReplacement)
    if ($taskRelative -eq 'docs/work/ACTIVE_SLICE.md') {
        $taskNextPattern = '(?m)^Siguiente ejecución: auditar el archive DXC real[^\r\n]+'
        $taskNextMatch = [regex]::Matches($taskText, $taskNextPattern)
        if ($taskNextMatch.Count -ne 1) { throw 'Expected ACTIVE next step missing' }
        $taskText = $taskText.Replace($taskNextMatch[0].Value, $taskNext)
        $taskText = $taskText.Replace('el fatal vigente DXC figura arriba', 'el fatal vigente ASTC figura arriba').Replace('continuar desde el fatal vigente DXC.', 'continuar desde el fatal vigente ASTC.')
    }
    if ($taskRelative -eq 'docs/evidence/n0/N0_EXECUTION_2026_10_10.md') {
        $taskNextPattern = '(?m)^1\. Aprovisionar LZ4 1\.9\.4 realmente requerido[^\r\n]+'
        $taskNextMatch = [regex]::Matches($taskText, $taskNextPattern)
        if ($taskNextMatch.Count -ne 1) { throw 'Expected N0 historical next step missing' }
        $taskText = $taskText.Replace($taskNextMatch[0].Value, '1. Aprovisionar ASTC3.2 realmente requerido por ImageProcessingAtom.Editor.Static, con fuente permitida auditada, API/perfiles/formatos y CRT originales; catálogo sólo tras consumidores y rechazos nativos GREEN. DXC ya está aceptado e integrado. Continuar Root una vez por entrada nueva. LZ4/OpenSSL y sus pruebas son históricos aceptados, no el bloqueo actual; todavía no hay build del engine.')
    }
    if ($taskText -ceq $taskOriginal) { throw 'No documentation change' }
    [IO.File]::WriteAllBytes($taskPath, [Text.Encoding]::UTF8.GetBytes($taskText))
    [pscustomobject]@{path=$taskRelative;changed=$true}
}
