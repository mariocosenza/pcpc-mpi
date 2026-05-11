#!/bin/bash

EXE="./game_of_life.exe"
LOG="scaling_results_linux.txt"

PROCS=(1 2 4 6 8 10 12 16 20 24 28 32)
VTUNE_PROCS=(4 8 12 16 20 24 28 32)
EXT_PROCS=(12 24 32)
VTUNE_TYPES=(hotspots memory-access threading hpc-performance)

STD_SIZES=(6 10 840 1680 3360 13440)
PINNING="-genv I_MPI_PIN_DOMAIN=core -genv I_MPI_PIN_ORDER=compact"

echo "--- Test Scalabilita Start: $(date) ---" | tee $LOG

for s in "${STD_SIZES[@]}"; do
    echo -e "\n----------------------------------------------------"
    echo "DIMENSIONE MATRICE: ${s}x${s}"
    echo "----------------------------------------------------"
    for p in "${PROCS[@]}"; do
        fn="matrix_${s}x${s}_seed1234_pattern0.bin"
        echo "[RUN] Size: ${s}x${s} | Processi: $p"
        echo -e "\n>>> Test: ${s}x${s} con $p processi <<<" >> $LOG
        mpiexec -n $p $PINNING $EXE -M $s -N $s -G 100 -FN $fn 2>&1 | tee -a $LOG
    done
done

echo -e "\n>>> AVVIO TEST MATRICI GRANDI (NO VTUNE) <<<"
for p in "${PROCS[@]}"; do
    echo "[BIG] Size: 20000x20000 | Processi: $p"
    echo -e "\n>>> Test NORMALE: 20000x20000 con $p processi <<<" >> $LOG
    mpiexec -n $p $PINNING $EXE -M 20000 -N 20000 -G 100 -FN "matrix_20000x20000_seed1234.bin" 2>&1 | tee -a $LOG
    
    echo "[BIG] Size: 50000x50000 | Processi: $p"
    echo -e "\n>>> Test NORMALE: 50000x50000 con $p processi <<<" >> $LOG
    mpiexec -n $p $PINNING $EXE -M 50000 -N 50000 -G 20 -FN "matrix_50000x50000_seed1234.bin" 2>&1 | tee -a $LOG
done

echo -e "\n>>> AVVIO TEST PROFILAZIONE VTUNE SULLA 20k <<<"
for p in "${VTUNE_PROCS[@]}"; do
    for vtype in "${VTUNE_TYPES[@]}"; do
        echo "[VTUNE: $vtype] Size: 20000x20000 | Processi: $p"
        VTUNE_DIR="./vtune_ris_${vtype}_20k_p${p}"
        echo -e "\n>>> Test VTune ($vtype): 20000x20000 con $p processi <<<" >> $LOG
        mpiexec -n $p $PINNING vtune -collect $vtype -data-limit=2000 -result-dir $VTUNE_DIR -- $EXE -M 20000 -N 20000 -G 100 -FN "matrix_20000x20000_seed1234.bin" 2>&1 | tee -a $LOG
    done
done

echo -e "\n>>> AVVIO TEST MATRICI ESTREME (100k) <<<"
for p in "${EXT_PROCS[@]}"; do
    echo "[EXTREME] 100kx50k | Processi: $p"
    echo -e "\n>>> Test EXTREME: 100000x50000 con $p processi <<<" >> $LOG
    mpiexec -n $p $PINNING $EXE -M 100000 -N 50000 -G 5 -FN "matrix_100000x50000_seed1234.bin" 2>&1 | tee -a $LOG
    
    echo "[EXTREME] 100kx100k | Processi: $p"
    echo -e "\n>>> Test EXTREME: 100000x100000 con $p processi <<<" >> $LOG
    mpiexec -n $p $PINNING $EXE -M 100000 -N 100000 -G 5 -FN "matrix_100000x100000_seed1234.bin" 2>&1 | tee -a $LOG
done

echo -e "\n--- Test Completati! ---"