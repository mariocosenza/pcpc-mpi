@echo off
setlocal enabledelayedexpansion

:: --- Configurazione ---
set "EXE=.\game_of_life.exe"
set "LOG=scaling_results_windows.txt"
set "PROCS=1 2 4 6 8 10 12"
set "STD_SIZES=6 10 840 1680 3360 13440"
set "PINNING=-genv I_MPI_PIN_DOMAIN=core -genv I_MPI_PIN_ORDER=compact"

echo --- Test Scalabilita Start: %date% %time% --- > "%LOG%"

:: 1. TEST MATRICI STANDARD
for %%s in (%STD_SIZES%) do (
    echo.
    echo ----------------------------------------------------
    echo DIMENSIONE MATRICE: %%s x %%s
    echo ----------------------------------------------------
    
    for %%p in (%PROCS%) do (
        set "fn=matrix_%%sx%%s_seed1234_pattern0.bin"
      
        echo [RUN] Size: %%s x %%s ^| Processi: %%p
        
        echo ^>^>^> Test: %%s x %%s con %%p processi ^<^<^< >> "%LOG%"
        mpiexec -n %%p %PINNING% "%EXE%" -M %%s -N %%s -G 100 -FN !fn! >> "%LOG%" 2>&1
    )
)

:: MATRICI GRANDI (20k a 100 gen, 50k a 20 gen) ---
 for %%p in (%PROCS%) do (
     echo [BIG] Size: 20000x20000 ^| Processi: %%p
     mpiexec -n %%p %PINNING% "%EXE%" -M 20000 -N 20000 -G 100 -FN matrix_20000x20000_seed1234.bin >> "%LOG%" 2>&1

     echo [BIG] Size: 50000x50000 ^| Processi: %%p
     mpiexec -n %%p %PINNING% "%EXE%" -M 50000 -N 50000 -G 20 -FN matrix_50000x50000_seed1234.bin >> "%LOG%" 2>&1
 )

:: MATRICI ESTREME (100k) ---
 set "EXT_PROCS=4 10"
 for %%p in (%EXT_PROCS%) do (
     echo [EXTREME] 100kx50k ^| Processi: %%p
     mpiexec -n %%p %PINNING% "%EXE%" -M 100000 -N 50000 -G 5 -FN matrix_100000x50000_seed1234.bin >> "%LOG%" 2>&1
     echo [EXTREME] 100kx100k ^| Processi: %%p
     mpiexec -n %%p %PINNING% "%EXE%" -M 100000 -N 100000 -G 5 -FN matrix_100000x100000_seed1234.bin >> "%LOG%" 2>&1
 )

echo.
echo --- Test Completati! Risultati in %LOG% ---
pause