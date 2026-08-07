# Primeira build Xbox 360

Este diretório contém um bootstrap nativo mínimo para validar vídeo e controle antes de portar toda a interface. Ele desenha cinco tiles em 720p, move o foco com o direcional e encerra com `BACK`.

## Requisitos

- Windows e um Xbox 360 XDK obtido e utilizado de forma autorizada.
- Um Developer Command Prompt no qual a variável `XEDK` esteja configurada.
- Um console de desenvolvimento autorizado ou um console no qual você esteja legalmente autorizado a executar seu próprio homebrew.

O repositório não fornece XDK, bibliotecas proprietárias, chaves, exploits ou assinatura de executáveis.

## Compilar

Abra o Developer Command Prompt, navegue até este diretório e execute:

```bat
build-xex.bat
```

O script chama MSBuild 4.0 e o resultado esperado é `dist\Xbox360\default.xex`. Se a versão do seu XDK não aceitar o `.vcxproj`, crie um projeto vazio **Xbox 360 Application** na IDE fornecida pelo ambiente, adicione `XboxMain.cpp`, defina `_XBOX`, use `default` como nome de saída e vincule as bibliotecas de D3D e XInput fornecidas pelo XDK.

## Teste seguro

Copie `default.xex` para uma pasta isolada como `HDD1:\Homebrew\AuroraX-Test\` e inicie manualmente com uma dashboard que já funcione. Não configure esta primeira build como dashboard padrão. Ela não escreve na NAND, não altera `launch.ini`, não controla a ventoinha e não inicia outros XEX.
