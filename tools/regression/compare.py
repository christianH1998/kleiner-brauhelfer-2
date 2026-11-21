"""Vergleicht zwei kbh-dump-Ausgaben (vorher/nachher).

Aufruf:
    py -3.10 tools/regression/compare.py vorher.json nachher.json [--tol 1e-9] [--ignore Feld1,Feld2]

Exit-Code 0 = identisch (innerhalb Toleranz), 1 = Abweichungen.
Felder, die nur in "nachher" existieren (bewusst neue Felder), werden gemeldet,
zaehlen aber nicht als Abweichung.
"""
import argparse
import json
import sys


def as_float(v):
    try:
        return float(v)
    except (TypeError, ValueError):
        return None


def equal(a, b, tol):
    if a == b:
        return True
    fa, fb = as_float(a), as_float(b)
    if fa is None or fb is None:
        return False
    return abs(fa - fb) <= tol * max(1.0, abs(fa), abs(fb))


def main():
    p = argparse.ArgumentParser()
    p.add_argument("vorher")
    p.add_argument("nachher")
    p.add_argument("--tol", type=float, default=1e-9)
    p.add_argument("--ignore", default="")
    p.add_argument("--max", type=int, default=50, help="max. ausgegebene Abweichungen")
    args = p.parse_args()

    # Gespeichert: ModelSud::update() setzt es bei jedem Lauf auf "jetzt"
    ignore = {"Gespeichert"} | {f for f in args.ignore.split(",") if f}
    with open(args.vorher, encoding="utf-8") as f:
        a = json.load(f)
    with open(args.nachher, encoding="utf-8") as f:
        b = json.load(f)

    diffs, neu = [], set()
    for table in sorted(k for k in a if not k.startswith("_")):
        ra, rb = a.get(table, []), b.get(table, [])
        if len(ra) != len(rb):
            diffs.append(f"{table}: Zeilenzahl {len(ra)} -> {len(rb)}")
            continue
        for i, (xa, xb) in enumerate(zip(ra, rb)):
            rid = xa.get("ID", i)
            for field in xb.keys() - xa.keys():
                neu.add(f"{table}.{field}")
            for field, va in xa.items():
                if field in ignore:
                    continue
                if field not in xb:
                    diffs.append(f"{table}[ID {rid}].{field}: Feld fehlt nachher")
                elif not equal(va, xb[field], args.tol):
                    diffs.append(f"{table}[ID {rid}].{field}: {va!r} -> {xb[field]!r}")

    if neu:
        print("Neue Felder (nicht bewertet):", ", ".join(sorted(neu)))
    if diffs:
        print(f"{len(diffs)} Abweichung(en):")
        for d in diffs[: args.max]:
            print("  " + d)
        if len(diffs) > args.max:
            print(f"  ... {len(diffs) - args.max} weitere")
        return 1
    print("Keine Abweichungen.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
