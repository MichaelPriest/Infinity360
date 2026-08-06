# AuroraX

Protótipo open-source de uma dashboard para Xbox 360 com interface portátil para desenvolvimento no PC. O núcleo já oferece navegação por controle, biblioteca com varredura recursiva de `default.xex`, favoritos, atualização, detalhes, configurações, sistema e gerenciador de arquivos.

> O projeto não inclui exploits, chaves, conteúdo protegido ou meios de contornar segurança. A execução real de XEX, leitura de temperatura e renderização no console devem ser conectadas às APIs do XDK pelo proprietário de um ambiente autorizado.

## Compilar e executar

```sh
cmake -S . -B build
cmake --build build
./build/aurorax
```

Crie jogos de teste como `Games/Nome do jogo/default.xex`. Comandos: `left`, `right`, `up`, `down`, `a`, `b`, `x`, `y` e `back` (ou `q`).

## Interface gráfica

Uma demonstração visual completa e responsiva está disponível em `Web/index.html`. Abra o arquivo em um navegador moderno ou execute `python3 -m http.server 8080 -d Web` e acesse `http://localhost:8080`. A demonstração inclui tela inicial, biblioteca pesquisável, filtros, ordenação, favoritos, detalhes dos jogos, aplicativos, emuladores, arquivos, sistema e simulação das ações do controle.

## Roadmap do console

- Adaptador XInput/XDK e renderizador Direct3D 9 para tiles, capas e animações.
- Provider SMC para temperaturas, ventoinha, CPU/GPU/RAM e armazenamento.
- Launcher de XEX com confirmação, retorno à dashboard e histórico.
- Capas e metadados locais, FTP, SMB, atualização via HTTP e downloads em segundo plano.
- Gerenciador de arquivos, capturas, temas, perfis, conquistas e controle parental.
- Plugins opcionais para emuladores, homebrew, discos, DLC/TU e multidisco.
