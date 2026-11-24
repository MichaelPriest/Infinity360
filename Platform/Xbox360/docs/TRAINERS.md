# Trainers locais (experimental)

Para uso offline, apenas com utilitários de origem confiável que funcionam como XEX independentes. A AuroraX não fornece trainers nem injeta código no jogo.

## Instalação manual

```text
AuroraX/Trainers/4D5307E6/Example/trainer.axtrainer
AuroraX/Trainers/4D5307E6/Example/trainer.xex
```

Manifesto de exemplo (valores ilustrativos; conferir o cabeçalho real do jogo):

```ini
schema=1
mode=standalone
name=Example Trainer
title_id=4D5307E6
media_id=12345678
version=20010002
entry=trainer.xex
```

Abra Biblioteca > jogo > detalhes > RB > Trainers. Y atualiza. A mostra a confirmação e A novamente solicita XLaunchNewImage do utilitário; B cancela. Não há ativação automática, injeção, seleção de cheats, download, integração com TU ativo ou garantia de retorno. O cabeçalho XEX2 não autentica o binário. Não utilizar em multiplayer online. A compatibilidade é apenas de metadados declarados e do cabeçalho do jogo, não validação funcional. A execução física ainda depende de XDK e testes de hardware.
