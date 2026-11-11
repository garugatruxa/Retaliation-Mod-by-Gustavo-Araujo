"""Generates the preset folders shipped next to the mod:

  <out>/Dificuldade - Facil/Dificuldade.ini      (and Medio, Dificil)
  <out>/Desempenho - PC Fraco/Desempenho.ini     (and Intermediario, Forte)

Copy ONE Dificuldade.ini and/or ONE Desempenho.ini into modloader\\Retaliation.
Values in a preset override Retaliation.ini. Dificil + PC Forte = the mod's
default (v1.05 tuning), so those two files just spell out the default values.
"""
import os
import sys

# per level: FootPeds, CarsPerAmbush, PedsPerCar, Accuracy, Health, Armour, IntervalSeconds
DIFF = {
    "Facil": dict(
        name="Facil", desc="poucos inimigos, mira fraca, pouca blindagem, helicopteros e tanque raros",
        tiers=[(1, 1, 1, 15, 100, 0, 70), (1, 1, 2, 25, 100, 0, 65), (2, 1, 2, 35, 100, 0, 65),
               (2, 1, 3, 45, 100, 0, 60), (3, 1, 3, 55, 110, 25, 60), (3, 1, 4, 60, 125, 50, 60)],
        heli=[0, 3, 5, 8, 15, 20], hunter=5, double=0, tank=8, bonus=5),
    "Medio": dict(
        name="Medio", desc="meio-termo entre o Facil e o Dificil",
        tiers=[(1, 1, 2, 22, 100, 0, 55), (2, 1, 2, 35, 100, 0, 55), (2, 1, 3, 45, 100, 0, 55),
               (3, 1, 3, 55, 110, 25, 50), (4, 1, 4, 65, 125, 50, 50), (4, 2, 3, 72, 150, 75, 50)],
        heli=[3, 6, 10, 15, 30, 35], hunter=8, double=12, tank=15, bonus=7),
    "Dificil": dict(
        name="Dificil", desc="padrao do mod (o mais dificil)",
        tiers=[(2, 1, 2, 30, 100, 0, 45), (2, 1, 3, 45, 100, 0, 45), (4, 1, 4, 55, 100, 25, 45),
               (4, 2, 3, 65, 120, 50, 45), (6, 2, 4, 80, 150, 100, 45), (6, 2, 4, 85, 175, 100, 45)],
        heli=[5, 10, 15, 25, 50, 50], hunter=10, double=25, tank=25, bonus=10),
}

# MaxEnemies, MaxActive per level, DespawnDistance, SpawnDelaySeconds, HunterTimeoutSeconds,
# car spawn distance (min,max), DoubleChance cap (None = keep difficulty), MaxSameObjectsInPile
PERF = {
    "PC Fraco": dict(
        name="PC Fraco", file="Fraco", desc="no maximo 10 inimigos, spawns mais perto e mais espacados",
        max=10, active=[3, 5, 7, 8, 10, 10], despawn=280, delay=7, timeout=120,
        car=(180, 230), double=0, pile=4),
    "Intermediario": dict(
        name="Intermediario", file="Intermediario", desc="no maximo 13 inimigos",
        max=13, active=[4, 6, 8, 10, 13, 13], despawn=330, delay=6, timeout=140,
        car=(200, 260), double=None, pile=5),
    "PC Forte": dict(
        name="PC Forte", file="Forte", desc="padrao do mod (o mais pesado): no maximo 16 inimigos",
        max=16, active=[4, 6, 9, 12, 16, 16], despawn=380, delay=5, timeout=150,
        car=None, double=None, pile=6),
}
DEFAULT_CAR = [(210, 270), (210, 270), (210, 270), (210, 280), (220, 290), (220, 290)]

HEADER = """; =============================================================
;  Retaliation v1.06 - preset de {kind}: {name}
;  {desc}
;
;  COMO USAR: copie este arquivo ({file}) para a pasta do mod:
;     <GTA SA>\\modloader\\Retaliation\\
;  (use so UM preset de dificuldade e UM de desempenho; para voltar ao
;   padrao, apague o arquivo da pasta do mod)
;  Os valores daqui substituem os do Retaliation.ini.
; =============================================================

[Preset]
Name={name}
"""


def write(path, text):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="\r\n") as f:
        f.write(text)


def main(out):
    for key, d in DIFF.items():
        t = HEADER.format(kind="dificuldade", name=d["name"], desc=d["desc"], file="Dificuldade.ini")
        t += "\n[Helicopter]\n"
        for i, c in enumerate(d["heli"], 1):
            t += f"ChanceTier{i}={c}\n"
        t += f"HunterChance={d['hunter']}\nDoubleChance={d['double']}\n"
        t += f"\n[Tank]\nChance={d['tank']}\n"
        t += f"\n[General]\nExtraChancePer10Points={d['bonus']}\n"
        for i, (foot, cars, per, acc, hp, arm, iv) in enumerate(d["tiers"], 1):
            t += (f"\n[Tier{i}]\nFootPeds={foot}\nCarsPerAmbush={cars}\nPedsPerCar={per}\n"
                  f"Accuracy={acc}\nHealth={hp}\nArmour={arm}\nIntervalSeconds={iv}\n")
        write(os.path.join(out, f"Dificuldade - {key}", "Dificuldade.ini"), t)

    for key, p in PERF.items():
        t = HEADER.format(kind="desempenho", name=p["name"], desc=p["desc"], file="Desempenho.ini")
        t += (f"\n[General]\nMaxEnemies={p['max']}\nDespawnDistance={p['despawn']:.1f}\n"
              f"SpawnDelaySeconds={p['delay']}\nHunterTimeoutSeconds={p['timeout']}\n")
        if p["double"] is not None:
            t += f"\n[Helicopter]\nDoubleChance={p['double']}      ; 2 helicopteros juntos pesam muito\n"
        t += f"\n[Performance]\nMaxSameObjectsInPile={p['pile']}\n"
        for i, a in enumerate(p["active"], 1):
            cmin, cmax = p["car"] if p["car"] else DEFAULT_CAR[i - 1]
            t += f"\n[Tier{i}]\nMaxActive={a}\nCarMinDist={cmin}\nCarMaxDist={cmax}\n"
        write(os.path.join(out, f"Desempenho - {key}", "Desempenho.ini"), t)

    t = """; =============================================================
;  Retaliation v1.06 - EXTRA: ATIVO EM MISSOES   ( USE POR SUA CONTA E RISCO )
; =============================================================
;
;  !!! ATENCAO !!!
;  Com este arquivo, as emboscadas, os helicopteros e o tanque continuam
;  vindo DURANTE AS MISSOES DA HISTORIA. Isso NAO E SUPORTADO:
;    - missoes podem falhar ou travar (cacadores matando NPCs ou veiculos
;      da missao, explosoes em cutscenes, alvos destruidos antes da hora...)
;    - NAO HA GARANTIA de que seja possivel terminar (zerar) a historia
;      com esta opcao ativa.
;  Recomendado so para quem ja zerou o jogo ou quer o caos total.
;  SALVE O JOGO ANTES de usar.
;
;  COMO USAR: copie este arquivo (Missoes.ini) para:
;     <GTA SA>\\modloader\\Retaliation\\
;  Para desligar, apague Missoes.ini da pasta do mod.
;  Pode ser usado junto com qualquer preset de dificuldade/desempenho.
; =============================================================

[Preset]
Name=Ativo em Missoes

[General]
PauseSpawnsOnMission=0      ; 0 = o mod continua funcionando durante as missoes
"""
    write(os.path.join(out, "Extra - Ativo em Missoes (RISCO)", "Missoes.ini"), t)

    readme = """PRESETS DO RETALIATION v1.06
=============================

Escolha no maximo UM preset de dificuldade e UM de desempenho e copie o
arquivo .ini de dentro da pasta para a pasta do mod:

    <GTA SA>\\modloader\\Retaliation\\

  Dificuldade - Facil / Medio / Dificil   ->  Dificuldade.ini
      quantidade de inimigos por emboscada, mira, vida, colete,
      intervalo entre emboscadas e chances de helicoptero e tanque.

  Desempenho - PC Fraco / Intermediario / PC Forte   ->  Desempenho.ini
      limite total de inimigos (10 / 13 / 16), limite por nivel,
      distancias de spawn e despawn e limpeza de carregadores.

  Extra - Ativo em Missoes (RISCO)   ->  Missoes.ini
      faz o mod funcionar TAMBEM durante as missoes da historia.
      ATENCAO: pode quebrar missoes e NAO ha garantia de conseguir
      terminar a historia com esta opcao. Salve antes de usar.

Sem nenhum preset copiado, o mod roda como Dificil + PC Forte (o padrao)
e fica pausado durante as missoes.
Para voltar ao padrao, apague Dificuldade.ini / Desempenho.ini da pasta
do mod. O Retaliation.log mostra qual preset foi carregado.
"""
    write(os.path.join(out, "LEIA-ME - Presets.txt"), readme)
    print("presets written to", out)


if __name__ == "__main__":
    main(sys.argv[1])
