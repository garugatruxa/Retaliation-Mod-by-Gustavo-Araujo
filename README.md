# Retaliation Mod — by Gustavo Araújo

Mod para **GTA San Andreas (PC, versão 1.0 US)** que faz o submundo de Los Santos, San Fierro e Las Venturas **reagir aos seus crimes**.

Cada NPC que o CJ mata, cada carro que ele rouba ou explode aumenta a **pressão de retaliação**. Quanto maior a pressão, mais pesados ficam os grupos armados que vêm caçar o CJ: primeiro os cobradores do cassino, depois caipiras, máfia russa, Triads e, no limite, helicópteros de mercenários. Os caçadores são grupos à parte, como os capangas que aparecem quando o CJ deve ao cassino, e não fazem parte do sistema de procurado da polícia.

> **Download:** baixe a versão mais recente na aba [**Releases**](../../releases). Cada versão tem um `.zip` com a pasta pronta para o Mod Loader.

---

## Índice

- [Como funciona](#como-funciona)
- [Requisitos](#requisitos)
- [Guia de instalação](#guia-de-instalação)
- [Configuração (`Retaliation.ini`)](#configuração-retaliationini)
- [Histórico de versões](#histórico-de-versões)
- [Problemas conhecidos](#problemas-conhecidos)
- [Compilando a partir do código-fonte](#compilando-a-partir-do-código-fonte)
- [Créditos](#créditos)
- [Licença](#licença)

---

## Como funciona

*(Descrição da versão mais recente, a 1.05.)*

### Pontos de retaliação

| Ação do CJ | Pontos |
|---|---|
| Matar um NPC comum (civil, traficante etc.) | **+2** |
| Matar um policial / SWAT / FBI / exército | **+1** |
| Matar um membro de gangue (Ballas, Vagos...) | **+1** |
| Roubar um veículo | **+2** |
| Explodir um veículo | **+4** |
| Ficar 30 s sem ganhar pontos | **−5** (repete a cada 30 s) |
| Morrer ou ser preso | **−10** |

O máximo é **50 pontos**. NPCs de missão e os próprios caçadores não dão pontos, para a pressão não crescer sozinha.

### Níveis de pressão (a cada 10 pontos)

O nível aparece na tela como **estrelas vermelhas**, ao lado das estrelas de procurado da polícia (1 estrela vermelha por nível). Elas piscam quando uma emboscada está chegando.

| Nível | Pontos | Quem vem atrás do CJ | Armas | Veículos |
|---|---|---|---|---|
| 0 | 0–9 | Ninguém | — | — |
| 1 | 10–19 | Cobradores do cassino (família Leone) | Pistolas | Sedans e clássicos/lowriders + moto |
| 2 | 20–29 | Matadores do cassino (família Leone) | Deagle, escopeta | Esportivos, SUVs, lowriders + motos |
| 3 | 30–39 | Caipiras | Escopetas, rifle | Caminhonetes, motos de trilha, quadriciclo |
| 4 | 40–49 | Máfia russa | AK-47, MP5, Uzi | SUVs, sedans + motos |
| 5 | 50 | Triads (terno preto, San Fierro) | M4, AK-47, Spas-12 | Tuners de San Fierro + motos esportivas |

A cada nível aumentam a quantidade de inimigos, a mira, a vida, o colete e a frequência das emboscadas.

### Emboscadas

- Chegam **aos poucos e de longe**: primeiro grupos de 2 a pé (a 70–95 m), depois carros e motos ainda mais longe (210–290 m). Os grupos são espalhados ao longo do tempo até a próxima emboscada.
- Motos vêm com piloto e garupa.
- Os carros perseguem o CJ e, perto dele, os inimigos descem para atacar.
- No máximo **16 inimigos ao mesmo tempo**, contando o helicóptero.
- Inimigos que ficam muito longe ou que demoram demais para chegar até o CJ são removidos, para abrir espaço para os próximos.
- As emboscadas pausam durante missões da história e em interiores.

### Helicópteros

A cada emboscada existe uma chance de vir um helicóptero junto, sempre um por vez:

| Nível | Chance |
|---|---|
| 1 | 5% |
| 2 | 10% |
| 3 | 15% |
| 4 | 25% |
| 5 | 50% |

Quando vem, o tipo é sorteado:

- **Hunter (10%)**: helicóptero de ataque que circula o CJ e dispara rajadas de metralhadora e mísseis.
- **Maverick com atiradores**: 4 mercenários nos esquis, 2 de cada lado, atirando de MP5.
- **Maverick com rapel**: o helicóptero para perto do CJ e 4 mercenários descem de corda, com a mesma animação da SWAT.

Matar o piloto faz o corpo dele cair do helicóptero, que pega fogo, cai e explode.

### Polícia × retaliação

- **Limite de estrelas:** cada nível de retaliação tira 1 estrela do máximo da polícia (nível 1 = no máximo 4 estrelas ... nível 5 = o CJ não pode ser procurado). Os dois sistemas não se acumulam e o jogo não fica sobrecarregado.
- **Calor máximo:** se o CJ ficar **3 minutos com as 5 estrelas vermelhas**, as estrelas piscam e viram amarelas. A retaliação zera e o CJ recebe **6 estrelas da polícia**. Ele só volta a ganhar retaliação depois de perder todas as estrelas de procurado, morrer ou ser preso.

---

## Requisitos

| Requisito | Observação |
|---|---|
| **GTA San Andreas 1.0 US** | O executável precisa ser o 1.0 US (`gta_sa.exe`/`gta-sa.exe` de 14.383.616 bytes). A versão Steam/Rockstar atual precisa ser **rebaixada (downgrade)** para a 1.0. |
| **CLEO 4.4** ou mais recente | O mod usa o CLEO para rodar a cada frame. |
| **Mod Loader** | Para instalar arrastando a pasta. |
| **ASI Loader** | Normalmente já vem com o CLEO / Mod Loader. |

O mod verifica a versão do jogo ao iniciar. Se não for a 1.0 US, ele se desativa sozinho e registra o motivo no log.

---

## Guia de instalação

1. **Prepare o jogo:** GTA SA na versão **1.0 US**, com **CLEO 4** e **Mod Loader** instalados. Se você já usa mods CLEO pelo Mod Loader, está tudo pronto.
2. **Baixe** o arquivo `Retaliation_v1.05.zip` na aba [Releases](../../releases) (ou outra versão, se preferir).
3. **Extraia** o `.zip`. Dentro dele há uma pasta `Retaliation` com:
   - `Retaliation.asi`: a lógica do mod;
   - `Retaliation.cs`: o script CLEO que chama o `.asi` a cada frame;
   - `Retaliation.ini`: as configurações;
   - `LEIAME.txt`: o resumo do mod.
4. **Copie a pasta `Retaliation` inteira** para dentro de:
   ```
   <pasta do GTA San Andreas>\modloader\
   ```
   O caminho final fica assim: `...\modloader\Retaliation\Retaliation.asi`.
5. **Abra o jogo** e carregue um save. Mate alguns NPCs: as estrelas vermelhas aparecem a partir de 10 pontos.

**Atualizando de uma versão anterior:** substitua a pasta inteira, **incluindo o `.ini`**. Um `.ini` antigo pode ter opções que mudaram de nome.

**Desinstalando:** apague a pasta `modloader\Retaliation`.

> **Dica:** não deixe duas versões do Retaliation no modloader ao mesmo tempo. Elas usam o mesmo opcode e rodariam em dobro.

---

## Configuração (`Retaliation.ini`)

Tudo pode ser ajustado no `Retaliation.ini`. As mudanças valem depois de reiniciar o jogo. Cada opção está comentada no próprio arquivo. As seções são:

| Seção | O que controla |
|---|---|
| `[Points]` | Pontos por ação, máximo, perda com o tempo e penalidade por morte |
| `[Wanted]` | Limite de estrelas da polícia por nível |
| `[General]` | Pausa em missão, blips no radar, distâncias, intervalo entre grupos, limite total de inimigos e tempo limite dos caçadores |
| `[Stars]` | Posição, tamanho e cor das estrelas vermelhas |
| `[MaxHeat]` | Tempo com 5 estrelas vermelhas e estrelas da polícia dadas no "calor máximo" |
| `[Helicopter]` | Chance por nível, tipos (Hunter / rapel / atiradores), armas e comportamento |
| `[Tier1]` … `[Tier5]` | Skins, armas, veículos, quantidade de inimigos, mira, vida, colete e distâncias de cada nível |

**Estrelas fora do lugar?** Os valores padrão foram calibrados em 1920×1080 com o HUD original. Se você usa outra resolução ou um mod de HUD, ajuste `X`, `Y`, `StarSpacing`, `ScaleX` e `ScaleY` na seção `[Stars]`.

**Diagnóstico:** o mod grava o arquivo `Retaliation.log` na própria pasta, com pontos, emboscadas e helicópteros. Se algo der errado, ele ajuda a descobrir o que aconteceu.

---

## Histórico de versões

Todas as versões estão na aba [Releases](../../releases). O código de cada uma está em [`source/history/`](source/history).

### v1.05 — atual
- **HUD substituído por estrelas vermelhas** ao lado das estrelas de procurado (1 por nível), com contorno e piscando quando vem emboscada.
- **Calor máximo:** 3 minutos com 5 estrelas vermelhas → animação (piscam e viram amarelas) → 6 estrelas da polícia e retaliação travada até o CJ despistar a polícia, morrer ou ser preso.
- **Novo helicóptero: Hunter** (10% de chance), com metralhadora e mísseis.
- **Piloto ejetado:** ao morrer, o corpo do piloto cai do helicóptero antes da explosão.
- **Destroço do helicóptero à prova de dano** e removido logo depois de cair (correção do helicóptero explodindo sem parar).
- **Policiais e membros de gangue** passam a dar 1 ponto.
- **Remoção de caçadores travados:** inimigos muito longe, ou que não chegam até o CJ em 150 s, são removidos e liberam espaço.
- O menu de teste (F8) foi removido.

### v1.04
- **Novos valores de pontos:** matar NPC +2, roubar veículo +2 e **explodir veículo +4** (novo).
- **O helicóptero vem junto com as emboscadas**, com chance por nível (5% / 10% / 15% / 25% / 50%), em vez de aparecer só no nível 5.
- **Carros spawnam bem mais longe** (210–290 m) e cada grupo tem **veículos do seu estilo**: sedans, clássicos/lowriders, esportivos, SUVs, caminhonetes e tuners, além de **motos com piloto e garupa**.
- **Limite de 16 inimigos ao mesmo tempo**, contando o helicóptero.
- **Emboscadas chegam aos poucos**, em grupos espalhados ao longo do intervalo.
- Primeiras correções contra a explosão infinita do helicóptero.

### v1.03
- **Helicóptero cai e explode** quando o piloto morre, em vez de ir embora.
- **Emboscadas em fila e de longe:** grupos a pé primeiro, carros depois e mais longe.
- **Segundo helicóptero:** Maverick com **mercenários descendo de rapel**, sorteado 50/50 com o de atiradores.

### v1.02
- **Limite de estrelas da polícia** conforme o nível de retaliação.
- **Um grupo de NPCs por nível:** cobradores Leone, matadores Leone, caipiras, máfia russa e Triads.
- **Helicóptero de mercenários** no nível 5: Maverick com 4 atiradores nos esquis.
- **Menu de teste** no F8, com ajuste de nível pelas setas, e HUD 30% menor.
- Perda de pontos mais rápida: −5 a cada 30 s sem ganhar pontos.
- Correções de crash no spawn do helicóptero e na primeira emboscada.

### v1.0 — primeira versão
- Sistema de **pontos**: matar NPC +5, roubar veículo +2, máximo 50.
- **5 níveis** de pressão (a cada 10 pontos), com emboscadas a pé e de carro.
- Níveis 1–3: cobradores do cassino. Níveis 4–5: caipiras e gangues.
- Perda de 10 pontos a cada 5 minutos sem crimes.
- **HUD de teste** com barra de pontos e teclas de teste no NumPad.

---

## Problemas conhecidos

- **Fire On Driver:** esse mod põe fogo no motorista de todo veículo que explode, e isso pode fazer o destroço do helicóptero explodir várias vezes. A v1.05 contorna o problema ejetando o piloto antes da explosão e deixando o destroço à prova de dano.
- **Rapel e mísseis do Hunter** usam funções internas do jogo 1.0 US. Em outras versões do executável o mod não roda, de propósito.
- **Estrelas vermelhas:** calibradas para 1920×1080. Em outras resoluções ou com mods de HUD, ajuste a seção `[Stars]`.
- **Pontos não são salvos no save:** ao carregar um jogo, a retaliação começa do zero.
- Se o jogo continuar aberto em segundo plano depois de fechado, finalize o `gta-sa.exe` no Gerenciador de Tarefas antes de atualizar o mod.

---

## Compilando a partir do código-fonte

O código está em [`source/`](source):

| Arquivo | Função |
|---|---|
| `Retaliation.cpp` | Toda a lógica, como plugin ASI (C++, 32 bits) |
| `make_cs.py` | Gera o `Retaliation.cs` (script CLEO montado byte a byte, sem Sanny Builder) |
| `Retaliation.ini` / `LEIAME.txt` | Configuração e leia-me distribuídos com o mod |
| `build.ps1` | Script de build |
| `history/<versão>/` | Código de cada versão publicada |

**Ferramentas:** [llvm-mingw](https://github.com/mstorsjo/llvm-mingw) (compilador `i686-w64-mingw32-g++`) e Python 3.

```powershell
powershell -ExecutionPolicy Bypass -File source\build.ps1 -Version 1.05
```

O resultado sai em `Builds\Retaliation 1.05\`.

**Como funciona por dentro:** o `.asi` registra um opcode CLEO próprio (`1FA5`), que o `.cs` chama a cada frame. Os comandos do jogo (criar NPCs, carros, blips, texto) são executados por um `CRunningScript` privado, a mesma técnica que o plugin-sdk usa. Abates e roubos são detectados lendo os pools de NPCs e veículos do jogo.

---

## Créditos

- **Ideia, design e testes:** Gustavo Araújo
- **Desenvolvimento:** feito com o auxílio do [Claude Code](https://claude.com/claude-code)
- Referências técnicas: [plugin-sdk](https://github.com/DK22Pac/plugin-sdk) e [gta-reversed](https://github.com/gta-reversed/gta-reversed) (endereços e estruturas do GTA SA 1.0 US)
- Requer [CLEO](https://cleo.li) e [Mod Loader](https://gtaforums.com/topic/669520-mod-loader/), de seus respectivos autores

## Licença

Distribuído sob a licença [MIT](LICENSE): qualquer pessoa pode usar, modificar e redistribuir o mod, desde que mantenha o aviso de copyright.

GTA San Andreas é uma marca da Rockstar Games. Este é um mod de fã, sem vínculo com a Rockstar.
