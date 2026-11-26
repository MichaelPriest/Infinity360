# Auditoria local — atualização Xbox 360 17559

Fonte: ZIP SystemUpdate_17559_USB(1).zip fornecido pelo mantenedor. SHA-256 38885a3ab6b9854fb4fa31d5ddf21a4fddfd4cfb25e219060cfb0c2fb31f208f. 115992188 bytes compactados; 159893693 descompactados. 152 entradas, CRC integral OK. CRC não autentica origem Microsoft; versão indicada pelo nome. Nenhum binário incorporado.

| Tipo | Quantidade | Magic verificado em todas as entradas |
| --- | ---: | --- |
| XZP | 115 | BDES |
| XEX | 26 | XEX2 |
| LEX | 4 | XEX2 |
| Sem extensão | 5 | PIRS |
| Manifest | 1 | XMNP |
| XTT | 1 | xttf |

## Matriz de aproveitamento

- Perfil e avatar: AvatarEditor.xex, L.GamerProfile.xex.gp.xzp, L.signin.xex.signin.xzp, L.dash.xex.gamercar.xzp; consultar XAM residente via SDK, não lançar módulos avulsos.
- Social: L.Guide.ChatAndMessenger.xex.media.xzp, L.Guide.Friends.xex.friends.xzp, L.Guide.Voicemail.xex.media.xzp, L.Dash.Social.lex.xzp.xzp; somente recursos BDES neste ZIP, não os executáveis correspondentes. Referência para fluxos sociais; não comprova comunicação ativa.
- IPTV: dash.ExtraAVCodecs.xex, dash.ExtraAVCodecs2.xex, L.dash.xex.iptv.xzp, Title.Zune.xex e L.MiniMediaPlayer.xex.media.xzp. Interfaces/formatos não comprovados; preservar FFmpeg.
- Rede e armazenamento: Dash.NetworkStorage.lex e L.Guide.NetworkStorage.xex.media.xzp; verificar integração nativa e permissões, não assumir SMB compatível.
- UI: L.hud.xex.hud.xzp, L.huduiskin.xex.xam.xzp, L.vk.xex.vk.xzp, L.dash.xex.thermal.xzp; criar arte e interface próprias, reutilizar chamadas suportadas.

## Referências de integração

XSDK fixado pelo workflow Alpha: https://github.com/xenosworks/xsdk/blob/74dfcca8d21c5fe83b59079ef0e032e62994c1a2/include/xsdk/xam.h e kernel.h. Declara XamUserGetSigninState, XamUserGetName, XamUserGetSigninInfo, XamShowSigninUI, xsdk_xam_keyboard, xsdk_xam_message_box e xsdk_system_ui_open. SocialProfileXsdk.h acrescenta adapter de consulta dos quatro slots, habilitado explicitamente por INFINITY_SOCIAL_XSDK. Sem inferir Gamerscore, avatar, guest ou disponibilidade do serviço de chat; esses dados ainda não são consultados. Integração com renderizador, loop, build PowerPC e hardware pendentes.

Tabela XAM do Xenia: https://github.com/xenia-project/xenia/blob/master/src/xenia/kernel/xam/xam_table.inc — nomes de interfaces de amigos, cartão e composição de mensagens são referências para investigação, não prova de disponibilidade no SDK fixado ou de funcionamento físico.

Não foi feita extração/decriptação de conteúdo BDES, autenticação criptográfica Microsoft nem instalação da atualização. Não alterar NAND, flash, sistema ou DRM. Reconciliar alterações locais do Work antes de integrar na main. Nenhuma build XEX disparada.
