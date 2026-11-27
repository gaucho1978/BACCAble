#!/bin/zsh
# usage: run.sh <path to Core>. Builds the host test from the real sources and runs it.
C=$1; D=${0:A:h}
{ awk '/^void floatToStr\(/{f=1} f{print} f&&/^}/{exit}' $C/Src/functions_Common.c
  awk '/^\tvoid buildLineWithFormat\(/{f=1} /^\tuint8_t removePatterns\(/{f=0} f{print}' $C/Src/functions_C1baccable.c; } > $D/extracted.inc
cc -fsyntax-only -Wno-everything -Werror=initializer-overrides -DRELEASE_FLAVOR=C1_FLAVOR -DC1_FLAVOR=1 -I$D/stub -I$C/Inc $C/Src/uds_parameters.c || exit 1 # giulia h3: two entries with the same [index]
cc -w -g -fsanitize=address,undefined -DRELEASE_FLAVOR=C1_FLAVOR -DC1_FLAVOR=1 -I$D/stub -I$D -I$C/Inc -o $D/test_screens $D/test_screens.c $C/Src/uds_parameters.c $C/Src/giulia_watch.c -lm && $D/test_screens
cc -Wall -g -fsanitize=address,undefined -I$D/stub -I$C/Inc -o $D/test_stage3 $D/test_stage3.c $C/Src/warnings.c && $D/test_stage3
