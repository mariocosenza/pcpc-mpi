#!/bin/bash

GEN="./generate_seed"

standard_sizes=(6 10 840 1680 3360 13440 20000)

echo "--- Inizio Generazione Matrici (Linux) ---"

for s in "${standard_sizes[@]}"; do
    echo "Generazione: $s x $s"
    $GEN -M $s -N $s -P 0
done

# Matrice Grande (50.000 x 50.000)
echo "Generazione Grande: 50000 x 50000"
$GEN -M 50000 -N 50000 -P 0

# Matrici Estreme (100.000 x 50.000 e 100.000 x 100.000)
echo "Generazione Estreme..."
$GEN -M 100000 -N 50000 -P 0
$GEN -M 100000 -N 100000 -P 0

echo "--- Generazione completata! ---"
ls -lh *.bin