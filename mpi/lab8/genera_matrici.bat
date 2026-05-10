@echo off
setlocal enabledelayedexpansion

:: Imposta il percorso dell'eseguibile
set "GEN=.\generate_seed.exe"

echo --- Inizio Generazione Matrici ---

:: Dimensioni Standard (Quadrate)
:: Ciclo attraverso la lista definita
for %%s in (6 10 840 1680 3360 13440 20000) do (
    echo [INFO] Generazione: %%s x %%s
    "%GEN%" -M %%s -N %%s -P 0
)

:: Matrice Grande (50k)
echo [ATTENZIONE] Generazione Grande: 50000 x 50000
"%GEN%" -M 50000 -N 50000 -P 0

:: Matrici Estreme (100k)
echo [CRITICO] Generazione Estreme...
"%GEN%" -M 100000 -N 50000 -P 0
"%GEN%" -M 100000 -N 100000 -P 0

echo --- Generazione completata! ---
pause