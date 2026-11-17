"""KOMPRUSSR KU-01 - mediciones automaticas del plugin (VST3).

Uso:  python tools/medir.py [ruta_al_plugin.vst3]
Si no das ruta, busca el .vst3 compilado dentro de build/.
Deja tablas (CSV), graficas (PNG) y resumen.txt en la carpeta resultados/.
Son modelos "inspirados en" valvulas: estas medidas describen el DSP, no una emulacion exacta.
"""
import sys, csv, pathlib
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from pedalboard import load_plugin

SR = 48000
TUBES = ["6386U", "6BA6", "6BC8", "6K7", "6SK7"]
ROOT = pathlib.Path(__file__).resolve().parent.parent
OUT = ROOT / "resultados"
OUT.mkdir(exist_ok=True)
LINES = []


def say(text=""):
    print(text)
    LINES.append(text)


def find_plugins():
    """Devuelve todos los .vst3 de build/ (paquete y archivo interior), el mas reciente primero."""
    if len(sys.argv) > 1:
        return [sys.argv[1]]
    found = list((ROOT / "build").rglob("*.vst3"))
    if not found:
        sys.exit("No encuentro ningun .vst3 dentro de build/. Compila primero.")
    def key(p):
        return ("Release" not in str(p), p.is_file(), -p.stat().st_mtime)  # Release, paquete antes que archivo, mas nuevo primero
    found.sort(key=key)
    return [str(p) for p in found]


def open_plugin():
    errors = []
    for path in find_plugins():
        say(f"Probando a cargar: {path}")
        try:
            pl = load_plugin(path)
            say("  -> cargado correctamente")
            return pl
        except Exception as e:  # noqa
            errors.append((path, str(e)))
            say(f"  -> fallo: {str(e)[:300]}")
    say("\nNo se pudo cargar ningun .vst3. Copia todo este texto y mandaselo a Claude.")
    sys.exit(1)


def sine(db_rms, secs, f=1000.0):
    t = np.arange(int(secs * SR)) / SR
    return (10 ** (db_rms / 20) * np.sqrt(2) * np.sin(2 * np.pi * f * t)).astype(np.float32)


P = {}  # nombres reales de los parametros tal como los ve pedalboard (p. ej. make_up_gain_db)


def setup(pl, tube, comp):
    setattr(pl, P["tube"], tube)
    setattr(pl, P["compression"], float(comp))
    setattr(pl, P["make_up_gain"], 0.0)
    setattr(pl, P["power"], True)


def run(pl, mono):
    lead = np.zeros(2 * SR, np.float32)  # 2 s de silencio para que el detector parta de cero
    x = np.concatenate([lead, mono])
    y = pl.process(np.stack([x, x]), SR, reset=True)
    return np.asarray(y[0][len(lead):], dtype=np.float64)


def rms_db(y, secs=1.0):
    seg = y[-int(secs * SR):]
    return 10 * np.log10(np.mean(seg ** 2) + 1e-20)


def level_out(pl, tube, comp, level):
    setup(pl, tube, comp)
    return rms_db(run(pl, sine(level, 2.5)))


def crossing(comps, gr, target):
    if gr[0] >= target:
        return comps[0]
    for i in range(1, len(comps)):
        if gr[i] >= target > gr[i - 1]:
            return comps[i - 1] + (target - gr[i - 1]) * (comps[i] - comps[i - 1]) / (gr[i] - gr[i - 1])
    return None


def harmonics(pl, tube, comp, level):
    setup(pl, tube, comp)
    y = run(pl, sine(level, 2.5))[-SR:]
    spec = np.abs(np.fft.rfft(y * np.hanning(len(y))))
    peak = lambda f: spec[int(f) - 2:int(f) + 3].max()
    f1 = peak(1000)
    h = [20 * np.log10(peak(1000 * k) / f1 + 1e-12) for k in range(2, 6)]
    thd = 100 * np.sqrt(sum(10 ** (x / 10) for x in h))
    return h, thd


def envelope(y, ms=10):
    n = int(SR * ms / 1000)
    return np.sqrt(np.convolve(y ** 2, np.ones(n) / n, mode="same") + 1e-20)


def dynamics(pl, tube, comp=70):
    setup(pl, tube, comp)
    x = np.concatenate([sine(-30, 1.0), sine(-6, 1.0), sine(-30, 2.0)])
    y = run(pl, x)[:len(x)]
    gain = 20 * np.log10(envelope(y) / envelope(x))
    base = gain[int(0.5 * SR):int(0.95 * SR)].mean()
    gr = base - gain
    t = np.arange(len(x)) / SR
    steady = gr[int(1.7 * SR):int(1.95 * SR)].mean()
    end = gr[int(1.9 * SR):int(2.0 * SR)].mean()
    attack = release = None
    if steady > 0.3:
        i = np.argmax(gr[int(1.0 * SR):] >= 0.63 * steady)
        attack = 1000 * i / SR
        j = np.argmax(gr[int(2.0 * SR):] <= 0.37 * end)
        release = 1000 * j / SR
    return t, gr, steady, attack, release


def main():
    pl = open_plugin()
    names = list(pl.parameters.keys())
    say(f"Parametros que ve el programa: {names}")
    for n in ("compression", "make_up_gain", "tube", "power"):
        real = next((x for x in names if x == n or x.startswith(n + "_")), None)
        if real is None:
            sys.exit(f"No encuentro el parametro '{n}'. Copia la linea de 'Parametros' y pasasela a Claude.")
        P[n] = real

    comps = list(range(0, 101, 5))
    levels = [-18, -14, -10]

    # 1) Cuanta reduccion hay segun el knob (relativa a Compression 0 %)
    say("\n=== 1. A que % de Compression empieza a notarse ===")
    say("Reduccion neta de nivel respecto a Compression 0 %, con un seno de 1 kHz (nivel RMS de entrada).")
    res, rows = {}, []
    for tube in TUBES:
        for lv in levels:
            out = [level_out(pl, tube, c, lv) for c in comps]
            res[(tube, lv)] = np.array(out)
            gr = out[0] - np.array(out)
            cr = [crossing(comps, gr, g) for g in (0.5, 1.0, 2.5)]
            rows.append([tube, lv] + [("" if c is None else round(c, 1)) for c in cr] + [round(float(gr[-1]), 2)])
        print(f"  {tube} medida")
    with open(OUT / "umbral_audible.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["valvula", "nivel_entrada_dBFS_RMS", "pct_para_0.5dB", "pct_para_1dB", "pct_para_2.5dB", "reduccion_a_100pct_dB"])
        w.writerows(rows)
    say(f"{'valvula':8}{'entrada':>10}{'0,5 dB':>12}{'1 dB':>12}{'2,5 dB':>12}{'a 100 %':>12}")
    for r in rows:
        fmt = lambda v: "no llega" if v == "" else f"{v} %"
        say(f"{r[0]:8}{str(r[1]) + ' dB':>10}{fmt(r[2]):>12}{fmt(r[3]):>12}{fmt(r[4]):>12}{str(r[5]) + ' dB':>12}")

    fig, axs = plt.subplots(1, 3, figsize=(15, 4.5), sharey=True)
    for ax, lv in zip(axs, levels):
        for tube in TUBES:
            ax.plot(comps, res[(tube, lv)][0] - res[(tube, lv)], label=tube)
        ax.set_title(f"Entrada {lv} dB RMS"); ax.set_xlabel("Compression (%)"); ax.grid(alpha=.3)
    axs[0].set_ylabel("Reduccion neta (dB)"); axs[0].legend()
    fig.tight_layout(); fig.savefig(OUT / "reduccion_vs_compression.png", dpi=110); plt.close(fig)

    # 2) Curva entrada/salida del 6386U
    cl = list(range(-40, -3, 4))
    fig, ax = plt.subplots(figsize=(6.5, 6))
    ax.plot(cl, cl, "k--", lw=.8, label="sin cambio")
    for c in (0, 25, 50, 75, 100):
        ax.plot(cl, [level_out(pl, "6386U", c, lv) for lv in cl], label=f"{c} %")
    ax.set_xlabel("Entrada (dB RMS)"); ax.set_ylabel("Salida (dB RMS)"); ax.set_title("6386U: entrada contra salida")
    ax.grid(alpha=.3); ax.legend(title="Compression"); fig.tight_layout()
    fig.savefig(OUT / "curva_entrada_salida_6386U.png", dpi=110); plt.close(fig)

    # 3) Armonicos
    say("\n=== 2. Armonicos (seno de 1 kHz, Compression 70 %) ===")
    hrows = []
    fig, axs = plt.subplots(1, 2, figsize=(12, 4.5), sharey=True)
    for ax, lv in zip(axs, (-18, -6)):
        for i, tube in enumerate(TUBES):
            h, thd = harmonics(pl, tube, 70, lv)
            hrows.append([tube, lv] + [round(v, 1) for v in h] + [round(thd, 3)])
            ax.bar(np.arange(4) + i * 0.16, h, 0.16, label=tube)
            say(f"  {tube:6} {lv:>4} dB:  2.o {h[0]:6.1f}  3.o {h[1]:6.1f}  4.o {h[2]:6.1f}  5.o {h[3]:6.1f} dB   THD {thd:.3f} %")
        ax.set_xticks(np.arange(4) + .32); ax.set_xticklabels(["2.o", "3.o", "4.o", "5.o"])
        ax.set_title(f"Entrada {lv} dB RMS"); ax.grid(alpha=.3, axis="y")
    axs[0].set_ylabel("Nivel relativo a la fundamental (dB)"); axs[0].legend()
    fig.tight_layout(); fig.savefig(OUT / "armonicos.png", dpi=110); plt.close(fig)
    with open(OUT / "armonicos.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["valvula", "nivel_dB_RMS", "H2_dB", "H3_dB", "H4_dB", "H5_dB", "THD_pct"])
        w.writerows(hrows)

    # 4) Ataque y recuperacion
    say("\n=== 3. Ataque y recuperacion (Compression 70 %, rafaga de -30 a -6 dB RMS) ===")
    drows = []
    fig, ax = plt.subplots(figsize=(9, 4.5))
    for tube in TUBES:
        t, gr, steady, a, r = dynamics(pl, tube)
        ax.plot(t, gr, label=tube)
        drows.append([tube, round(float(steady), 2), "" if a is None else round(a), "" if r is None else round(r)])
        say(f"  {tube:6} reduccion {steady:5.2f} dB   ataque {'n/a' if a is None else f'{a:.0f} ms'}   recuperacion {'n/a' if r is None else f'{r:.0f} ms'}")
    ax.axvspan(1.0, 2.0, color="grey", alpha=.12)
    ax.set_xlabel("Tiempo (s)"); ax.set_ylabel("Reduccion (dB)"); ax.grid(alpha=.3); ax.legend()
    ax.set_title("Reduccion en el tiempo (zona gris: rafaga fuerte)")
    fig.tight_layout(); fig.savefig(OUT / "ataque_recuperacion.png", dpi=110); plt.close(fig)
    with open(OUT / "dinamica.csv", "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["valvula", "reduccion_dB", "ataque_ms_63pct", "recuperacion_ms_63pct"])
        w.writerows(drows)

    # 5) Saltos de volumen entre valvulas
    say("\n=== 4. Nivel de salida respecto al 6386U (entrada -18 dB RMS) ===")
    ref0 = level_out(pl, "6386U", 0, -18)
    ref70 = level_out(pl, "6386U", 70, -18)
    for tube in TUBES:
        d0 = level_out(pl, tube, 0, -18) - ref0
        d70 = level_out(pl, tube, 70, -18) - ref70
        say(f"  {tube:6} Compression 0 %: {d0:+5.2f} dB    Compression 70 %: {d70:+5.2f} dB")

    (OUT / "resumen.txt").write_text("\n".join(LINES), encoding="utf-8")
    say(f"\nListo. Resultados en: {OUT}")


if __name__ == "__main__":
    main()
