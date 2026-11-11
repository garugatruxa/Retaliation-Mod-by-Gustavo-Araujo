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
| Matar um NPC civil (civil, traficante etc.) | **+4** |
| Matar um policial / SWAT / FBI / exército | **+5** |
| Matar um membro de gangue (Ballas, Vagos...) | **+2** |
| Roubar um veículo | **+2** |
| Explodir um veículo | **+4** |
| Ficar 45 s sem ganhar pontos | **−5** (repete a cada 45 s) |
| Morrer ou ser preso | **zera** (perde todas as estrelas vermelhas, como as da polícia) |

O máximo é **100 pontos**. As 6 estrelas vermelhas (nível máximo) começam em 60; de 60 a 100 é uma folga para o CJ segurar o nível máximo. Ficar acima de 80 pontos ativa o contador do [calor máximo](#polícia--retaliação). NPCs de missão e os próprios caçadores não dão pontos, para a pressão não crescer sozinha.

### Níveis de pressão (a cada 10 pontos)

O nível aparece na tela como **estrelas vermelhas**, numa fileira logo abaixo das estrelas de procurado da polícia (1 estrela vermelha por nível). Elas piscam quando uma emboscada está chegando.

| Nível | Pontos | Quem vem atrás do CJ | Armas | Veículos |
|---|---|---|---|---|
| 0 | 0–9 | Ninguém | — | — |
| 1 | 10–19 | Cobradores do cassino (família Leone) | Pistolas | Sedans e clássicos/lowriders + moto |
| 2 | 20–29 | Matadores do cassino (família Leone) | Deagle, escopeta | Esportivos, SUVs, lowriders + motos |
| 3 | 30–39 | Caipiras | Escopetas, rifle | Caminhonetes, motos de trilha, quadriciclo |
| 4 | 40–49 | Máfia russa | AK-47, MP5, Uzi | SUVs, sedans + motos |
| 5 | 50–59 | Triads (terno preto, San Fierro) | M4, AK-47, Spas-12 | Tuners de San Fierro + motos esportivas |
| 6 | 60–100 | Guerra total: elite de todos os grupos | M4, AK-47, Spas-12 | SUVs, Sandking, tuners + **25% de chance de um tanque Rhino** que persegue e atira no CJ |

A cada nível aumentam a quantidade de inimigos, a mira, a vida, o colete e a frequência das emboscadas.

### Emboscadas

- Chegam **aos poucos e de longe**: primeiro grupos de 2 a pé (a 70–95 m), depois carros e motos ainda mais longe (210–290 m). Os grupos são espalhados ao longo do tempo até a próxima emboscada.
- Uma emboscada nova começa a cada **45 segundos**, e os inimigos dela vão chegando aos poucos durante esse tempo.
- Motos vêm com piloto e garupa.
- Os carros perseguem o CJ e, perto dele, os inimigos descem para atacar.
- No máximo **16 inimigos ao mesmo tempo**, contando o helicóptero.
- Inimigos que ficam muito longe ou que demoram demais para chegar até o CJ são removidos, para abrir espaço para os próximos.
- As emboscadas pausam durante missões da história e em interiores.

### Helicópteros

A cada emboscada existe uma chance de vir um helicóptero junto:

| Nível | Chance |
|---|---|
| 1 | 5% |
| 2 | 10% |
| 3 | 15% |
| 4 | 25% |
| 5 | 50% |
| 6 | 50% |
| 70+ pontos | +10% a cada 10 pontos (70 = +10%, 80 = +20%, 90 = +30%, 100 = +40%), também para o tanque |

Com **90 pontos ou mais**, quando o helicóptero vem, há **25% de chance de virem 2 ao mesmo tempo**. O tipo de cada um é sorteado separadamente: pode ser um Hunter e um de atiradores, um de rapel e um Hunter, ou até dois iguais.

Quando vem, o tipo de cada helicóptero é sorteado:

- **Hunter (10%)**: helicóptero de ataque que circula o CJ e dispara rajadas de metralhadora (com dispersão) e mísseis.
- **Maverick com atiradores**: 4 mercenários nos esquis, 2 de cada lado, atirando de MP5.
- **Maverick com rapel**: o helicóptero para perto do CJ e 4 mercenários descem de corda, com a mesma animação da SWAT.

Matar o piloto faz o corpo dele cair do helicóptero, que pega fogo, cai e explode.

### Tanque

No nível 6 ou maior, cada emboscada tem **25% de chance** (mais o bônus de 70+ pontos) de trazer um **tanque Rhino**. Ele persegue o CJ e dispara projéteis do canhão mirando nele, a cada 5 s mais ou menos e a até 110 m. Vem um tanque por vez.

### Desempenho

- No máximo **16 inimigos** ao mesmo tempo.
- Inimigos que não chegam até o CJ, ou que ficam longe demais, são removidos.
- Se um mod de armas soltar dezenas de carregadores de uma vez, o Retaliation apaga o excesso (seção `[Performance]` do `.ini`).

### Polícia × retaliação

- **Juntas:** polícia e retaliação funcionam ao mesmo tempo. O CJ pode ter até 6 estrelas amarelas e 6 vermelhas. *(O antigo limite de estrelas por nível ainda existe como opção: `LimitWantedLevel=1` no `.ini`.)*
- **Calor máximo:** se o CJ ficar **3 minutos com 80 pontos ou mais**, as estrelas piscam e viram amarelas: a retaliação zera, os mercenários param e o CJ fica com **6 estrelas da polícia**. Depois disso, a retaliação só volta a contar pontos após **1 minuto** de cooldown.

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
   - `retstar.txd`: a imagem da estrela vermelha;
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
| `[Performance]` | Proteção contra montes de carregadores e munição dos caçadores |
| `[Stars]` | Posição, tamanho e cor das estrelas vermelhas |
| `[MaxHeat]` | Tempo com 5 estrelas vermelhas e estrelas da polícia dadas no "calor máximo" |
| `[Tank]` | Chance, alcance e cadência do tanque do nível 6 |
| `[Helicopter]` | Chance por nível, tipos (Hunter / rapel / atiradores), armas e comportamento |
| `[Tier1]` … `[Tier6]` | Skins, armas, veículos, quantidade de inimigos, mira, vida, colete e distâncias de cada nível |

**Estrelas fora do lugar?** Os valores padrão foram calibrados em 1920×1080 com o HUD original. Se você usa outra resolução ou um mod de HUD, ajuste `X`, `Y`, `StarSpacing`, `ScaleX` e `ScaleY` na seção `[Stars]`.

**Diagnóstico:** o mod grava o arquivo `Retaliation.log` na própria pasta, com pontos, emboscadas e helicópteros. Se algo der errado, ele ajuda a descobrir o que aconteceu.

---

## Histórico de versões

Todas as versões estão na aba [Releases](../../releases). O código de cada uma está em [`source/history/`](source/history).

### v1.05 — atual

Comparado à v1.04:

**Pontos e níveis**
- **Novos pontos:** NPC civil +4, policial +5, membro de gangue +2 (roubar +2 e explodir +4 continuam).
- **Máximo de 100 pontos** e **novo nível 6** (60 pontos): elite de todos os grupos. De 60 a 100 é folga para segurar o nível máximo.
- Perda de pontos mais lenta: −5 a cada **45 s** sem crimes.
- **Morrer ou ser preso zera todas as estrelas vermelhas**, como acontece com as da polícia.
- Emboscadas a cada **45 s** em todos os níveis, com os inimigos chegando aos poucos.

**Estrelas vermelhas (substituem o HUD)**
- O painel de teste e o menu F8 saíram. O nível agora aparece como **estrelas vermelhas** numa fileira logo abaixo das estrelas da polícia.
- As estrelas são uma **imagem própria** (`retstar.txd`), do tamanho das estrelas da polícia, com contorno preto leve.
- Piscam quando uma emboscada está chegando.

**Polícia × retaliação**
- Polícia e retaliação agora funcionam **juntas**: até 6 estrelas amarelas e 6 vermelhas. O limite de estrelas por nível passou a ser opcional e vem desligado.
- **Calor máximo:** 3 minutos com **80 pontos ou mais** fazem as vermelhas piscarem e virarem amarelas. A retaliação zera, o CJ fica com 6 estrelas da polícia, e há **1 minuto de cooldown** até voltar a contar pontos.

**Helicópteros**
- **Novo: Hunter** (10% de chance), com rajadas de metralhadora espalhadas e mísseis.
- **2 helicópteros ao mesmo tempo:** com 90+ pontos, 25% de chance, cada um com o tipo sorteado (podem ser iguais).
- A partir de 70 pontos, helicóptero e tanque ganham **+10% de chance a cada 10 pontos**.
- **Piloto ejetado:** ao morrer, o corpo cai do helicóptero antes da explosão. O marcador some quando o helicóptero é derrubado.
- **Correção da explosão infinita** (conflito com o mod *Fire On Driver*): o piloto não pega fogo, o destroço fica à prova de dano e é removido logo depois de cair.

**Tanque (novo)**
- No nível 6 ou maior, **25% de chance de um tanque Rhino** a cada emboscada. Ele persegue o CJ e dispara projéteis do canhão mirando nele.

**Desempenho e estabilidade**
- **Proteção contra a "enxurrada" de carregadores:** quando um mod de armas solta dezenas de pentes de uma vez, o excesso é apagado na hora.
- Caçadores com munição realista (600) em vez de 9999.
- Caçadores muito longe, ou que não chegam até o CJ em 150 s, são removidos e liberam espaço.
- Veículos dos caçadores são apagados com segurança: quem estiver dentro sai antes. Isso corrige um crash depois que o destroço de helicóptero era removido.

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

- **Fire On Driver:** esse mod põe fogo no motorista de todo veículo que explode, e isso fazia o destroço do helicóptero explodir sem parar. A v1.05 contorna o problema: o piloto é ejetado antes da explosão, não pega fogo, e o destroço fica à prova de dano até ser removido.
- **Carregadores do SanVive Weapons System:** em alguns NPCs, esse mod solta dezenas de pentes de uma vez ao recarregar. O Retaliation apaga o excesso automaticamente, mas o problema em si é do outro mod.
- **Tanque:** o tiro sai na direção do CJ, mas a torre do Rhino não gira visualmente. O projétil é o míssil do jogo.
- **Rapel, mísseis e estrela em imagem** usam funções internas do jogo 1.0 US. Em outras versões do executável o mod não roda, de propósito.
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
| `make_txd.py` | Gera o `retstar.txd` (textura da estrela vermelha) |
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
