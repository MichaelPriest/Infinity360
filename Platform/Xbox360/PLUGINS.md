# Plugins e extensões AuroraX — API de catálogo v1

> Status: **código de descoberta local e integração às seções Apps/Emuladores
> implementado**, testes adicionados; sem compilação/homologação XDK em console.

## Princípio

A dashboard mantém sua **interface própria** conforme `AuroraX_Telas.pdf`.
O catálogo local de plugins **amplia** Aplicativos e Emuladores, sem trocar
a home, sem alterar o player e sem substituir funções existentes.

A primeira versão (v1) é um **registro de aplicativos XEX independentes**:
- procurar pastas locais configuradas;
- ler um manifesto limitado e verificável;
- mostrar programas habilitados em Aplicativos ou Emuladores;
- abrir a ficha existente e, após confirmação, passar o `default.xex`
  ao mecanismo de lançamento já usado pela dashboard.

**Não há carregamento de DLLs, hooks no kernel, execução automática,
injeção em memória, download remoto de binários nem instalação silenciosa.**
Plugins binários escritos para Aurora/Freestyle/DashLaunch não são
automaticamente compatíveis com este registro: precisam ser avaliados
individualmente e, se possível, adaptados a um executável de homebrew
autorizado ou a uma API futura da AuroraX.

## Onde instalar

Os diretórios consultados pela versão Xbox 360 são:

- `game:\Plugins\<Nome>\plugin.axplugin`
- `HDD1:\AuroraX\Plugins\<Nome>\plugin.axplugin`
- `USB0:\AuroraX\Plugins\<Nome>\plugin.axplugin`
- `USB1:\AuroraX\Plugins\<Nome>\plugin.axplugin`

No teste portátil para PC, usa-se `<pasta-da-dashboard>/Plugins`.
O plugin pode conter um `default.xex` ou outro arquivo `.xex` no
**mesmo diretório** do manifesto. Não são aceitos caminhos absolutos,
subdiretórios no campo `entry` ou URLs.

## Manifesto v1

Arquivo de texto UTF-8 ou ASCII, **`plugin.axplugin`**:

```ini
schema=1
id=aurorax.example.player
name=Meu Player
version=0.1.0
category=app
entry=default.xex
enabled=true
```

Regras:
- `schema` deve ser 1; `id` deve ser único.
- `category` aceita `app` ou `emulator`.
- `entry` aceita apenas um nome de arquivo simples terminado em `.xex`.
- `enabled=false` oculta o plugin sem apagar arquivos.
- Manifesto limitado a 4096 bytes e a 64 plugins por varredura.
- Plugins sem executável local ou manifestos inválidos são ignorados.
- Só instale executáveis de **origem conhecida e confiável**.
- A atualização da lista é feita pelo botão **Y (Atualizar)** na seção.

Exemplo de estrutura:

```text
HDD1:\AuroraX\Plugins\MeuPlayer\
  plugin.axplugin
  default.xex
  Assets\
```

## O que preservamos

| Funcionalidade | Situação verificada no código | Política |
| --- | --- | --- |
| Dashboard 720p, foco e oito atalhos | Implementado no renderer XDK; não homologado em hardware | Não modificar layout |
| Biblioteca `default.xex` e abertura de títulos | Código existente; validação hardware pendente | Preservar |
| Favoritos e Recentes | Persistência local no código nativo | Preservar |
| Aplicativos e Emuladores | Varredura nativa + novos manifests v1 | Expandir sem substituir |
| Gerenciador de arquivos | Navegação/listagem nativa | Completar ações, com confirmação |
| Cloud360 M3U, busca, categorias, XMLTV | Implementado no código nativo | Manter |
| WMV/MPEG-PS e relay de outros formatos | Código disponível, console não homologado | Prioridade de testes |
| Temperaturas/ventoinha, FTP/SMB e downloads | Aparecem em protótipos web ou roadmap; não comprovados no hardware | Criar providers |
| Temas/perfis/conquistas/DLC/TU/multidisco | Recursos parciais ou planejados | Implementar como módulos opcionais |

## Aba Homebrew (nova)

**Homebrew** é uma aba de navegação própria dentro de
**Aplicativos**, ao lado de **Todos**, **Instalados** e **Plugins**.
Nela, a dashboard procura recursivamente arquivos
`default.xex` em `Homebrew` junto à aplicação e nas raízes
`game:\Homebrew`, `HDD1:\Homebrew`, `USB0:\Homebrew` e
`USB1:\Homebrew`.

Essa aba serve para homebrews **sem manifesto de plugin**. Emuladores
continuam aparecendo também na seção Emuladores quando instalados
nas pastas configuradas para ela. Não é necessário converter o
aplicativo para `.xur` ou modificar o formato original do XEX.

- **Esquerda/direita** alterna abas.
- **Cima/baixo** seleciona o programa.
- **A** abre a ficha; **A novamente** solicita execução.
- **Y** revarre a pasta; **B** volta.

A aba **Plugins** exibe somente extensões com manifesto compatível.
`Todos` mostra a união das categorias. A aparência da home não muda.

## Backlog de extensões (não implementadas ainda)

1. **Provider de metadados e capas:** cache, fontes selecionáveis e modo offline.
2. **Emuladores e homebrew:** detecção por manifestos, configurações por sistema.
3. **Gerenciador de jogos:** multidisco, Title Updates e DLC, respeitando
   licenças e sem distribuir arquivos protegidos.
4. **Rede:** FTP e SMB com credenciais protegidas, timeouts e limites de acesso.
5. **Mídia:** legendas, retomada, buffering, player e EPG.
6. **Monitoramento:** temperaturas, ventoinha, armazenamento e rede por APIs
   documentadas; modificar ventoinha só após testes adequados.
7. **Loja de extensões:** futuramente apenas manifestos e arquivos de
   código aberto auditados e autorizados, confirmação manual de instalação.

**SDK de plugins nativos:** só será definido quando o XDK e a ABI de
módulos compartilhados forem verificados. A ABI v1 atual não existe;
a primeira integração é somente catálogo + lançador local.

## Testes

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Testes: `Tests/PluginRegistryTests.cpp` e os cenários de integração em
`Tests/ConsoleHomeTests.cpp`. Um teste portátil bem-sucedido não garante
execução no Xbox físico. **Não substituir a dashboard atual por uma Alpha
sem validar o retorno seguro da GPU.**


## INFINITY Plugins — módulos residentes

Sistema > Plugins exibe os cinco slots `[Plugins]` do primeiro `launch.ini` legível (pasta local, HDD1, USB0, USB1). Reconhece a referência pelo nome `LiveEmulation.xex` e sinaliza slots duplicados. Y atualiza; B retorna. É inventário de configuração, não prova de módulo carregado, conexão, licença ou autenticidade. Não carrega/descarrega módulos nem altera o INI, o boot ou configurações de rede. Compatibilidade de coexistência e ABI ainda exige XDK e teste em console. Referência: https://liveemulation.com/wiki . Os plugins locais continuam com manifesto anterior e novas pastas Infinity360 mais pastas legadas AuroraX.

## Layout unificado (V0.2.82)

Plugins independentes podem ser instalados em `Plugins` ao lado de `default.xex` ou em `MenuPreview/Plugins`. Cada extensão exige manifesto próprio e XEX2 local. A validação do cabeçalho não confirma ABI, assinatura ou compatibilidade. Não mova NexusPlugin ou HidDriver para o registro de aplicativos: são módulos de outra arquitetura.
