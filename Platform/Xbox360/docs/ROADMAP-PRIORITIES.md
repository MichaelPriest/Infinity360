# Prioridades INFINITY 360

| Prioridade | Escopo |
| --- | --- |
| P0 | Sessão, lançamento de títulos XEX e GOD/XBLA, instalação de DVD, multidisco, armazenamento e recuperação da dashboard |
| P1 | TU/DLC, XboxUnity, Store, Cloud360 IPTV e integração com plugins |
| P2 | SMC, energia, System Link e saves |
| P3 | Temas, idiomas, guia e conquistas |

## Instalação e lançamento

A versão 0.2.8 introduziu o provider experimental de montagem GOD/XBLA pelo XAM. A 0.2.9 acrescentou importação de pacotes existentes. A 0.2.10 converte o DVD montado em um novo pacote GOD no HDD ou USB selecionado, relê os fragmentos e o indexa na biblioteca. O seletor multidisco reconhece cópias GOD e XEX.

Resta homologar leitura óptica, montagem e lançamento pelo XAM, seleção de disco e retorno à dashboard em hardware. O checklist físico e as versões Windows ficam para o fim.

## Revisão P1 — compatibilidade de Title Updates

A instalação local, o recibo de staging e o download XboxUnity compartilham agora a validação de Title ID, Media ID, tipo, formato e Base Version quando ambos os valores são conhecidos. Metadados ausentes em pacotes legados permanecem inconclusivos; o código não confirma ativação nem licença. A revalidação antes da publicação também compara os demais campos de identidade e tamanho declarado. Testes de regressão em ContentInstallTests; CI e hardware devem confirmar o resultado.

## Pendências por prioridade

- P0: validação de boot/GPU, leitura DVD, lançamento XEX/GOD/XBLA, multidisco, montagem dos volumes e retorno no console real.
- P1: cache TU legado, seleção/ativação efetiva de TU, DLC e licenças legítimas em hardware, metadados/capas XboxUnity, fila de downloads persistente, compatibilidade de plugins, IPTV nativo HLS/MP4 e validação Store.
- P2: sensores SMC e energia, System Link autorizado, integração de perfil ativo e saves, SMB, captura de tela, rede e resolução de vídeo.
- P3: temas e fontes nativos, localização, Guide, conquistas, estatísticas e personalização de blocos.
- Windows e checklist físico: após desenvolvimento das funções de console.

### P1 — Recuperação de TU arquivada

A aba TU Arquivadas lista os pacotes pessoais e permite recuperá-los para TitleUpdates com dupla confirmação, preservação do recibo e bloqueio de sobrescrita. O inventário informa Media ID e Base Version incompatíveis, metadados inconclusivos e identidade compatível; isso não comprova ativação. Testes de colisão, recuperação e recibo acrescentados. CI e hardware pendentes nesta revisão.
