# Diagnóstico gráfico do Cloud360 para Xbox 360

Esta pasta contém o diagnóstico XSDK v0.5.2 para investigar a saída do aplicativo e o retorno ao Aurora. O código da dashboard integrada e do player fica em outro projeto. [AuroraX para Xbox 360](../../../Platform/Xbox360/README.md).

## Compilar

O workflow **Cloud360 Xbox 360 SAFE graphics diagnostic** compila os dois executáveis e verifica o formato XEX2 e o conteúdo do ZIP. Para compilar localmente com o XSDK configurado, rode `bash tools/build_xex.sh` na pasta `IPTV/Cloud360`. O pacote gerado é `Cloud360-Xbox360-SAFE-v0.5.2.zip`.

| Arquivo | Função |
| --- | --- |
| `cloud360_probe.xex` | Testa a inicialização e a saída sem assumir a GPU. |
| `default.xex` | Abre a interface de texto, lê a lista M3U local e registra as etapas de apresentação e saída. |
| `playlist.m3u` e `LEIA-ME-TESTE.txt` | Lista de demonstração e procedimento para o console. |

## Teste no Xbox

A versão 0.5.1 apresentou corrupção visual ao retornar ao Aurora. A validação da v0.5.2 em hardware está pendente. Execute primeiro `cloud360_probe.xex` em uma pasta de testes e confira a imagem e o controle ao voltar ao Aurora. Se o retorno estiver normal, prossiga com a interface de diagnóstico conforme o roteiro. [Procedimento completo e registros esperados](../docs/GPU-RESTORE-v052.md).

O diagnóstico compila com `CLOUD360_WITH_FFMPEG=OFF` e `CLOUD360_WITH_LOGOS=OFF`. A reprodução de mídia da dashboard tem seu próprio roteiro de compilação e teste. [Player da dashboard](../../../Platform/Xbox360/README.md#reprodução-no-projeto-xdk).
