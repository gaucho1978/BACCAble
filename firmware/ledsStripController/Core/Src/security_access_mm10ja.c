/*
 * security_access_mm10ja.c
 *
 * Algoritmo UDS Security Access (0x27) per ECM Magneti Marelli MM10JA.
 * Vedere security_access_mm10ja.h per istruzioni d'uso ed esempio.
 *
 * Algoritmo originale: S1_FGA_ORIGINAL_ECU_Sup0002, livello 1.
 * Estratto dal file EFD (JS) del software diagnostico EOBD.
 * Coppia verificata su sniffata CAN reale (Alfa Romeo Giulia):
 *   seed = { 0x90, 0x98, 0x7E, 0x36 }  ->  key = { 0x4C, 0xA4, 0x46, 0xE7 }
 */

#include "security_access_mm10ja.h"

#define INTERNAL_KEY  0x17591215UL

/*
 * WF1 — divisione con segno per 178 (0xB2), combinazione lineare dei termini.
 * Input: valore a 16 bit con segno (estratto dai bit [15:0] di 'input').
 * Se il risultato ha bit 31 settato (negativo), aggiunge 0x7673 per correggere.
 * Output garantito in [0, 0xA2D2] — sempre nei 16 bit bassi.
 */
static uint32_t wf1(uint32_t input)
{
    int32_t s   = (int32_t)(int16_t)(uint16_t)(input & 0xFFFFU);
    int32_t q   = s / 0xB2;
    int32_t r   = s - q * 0xB2;
    int32_t res = (-0x3F) * q + 0xAA * r;
    uint32_t v  = (uint32_t)res;
    if (v & 0x80000000UL)
        v += 0x7673UL;
    return v;
}

/*
 * WF2 — divisione con segno per 177 (0xB1), combinazione lineare dei termini.
 * Input: valore a 16 bit con segno (estratto dai bit [15:0] di 'input').
 * Se il risultato ha bit 31 settato (negativo), aggiunge 0x763D per correggere.
 * Output garantito in [0, 0x76C2] — sempre nei 16 bit bassi.
 */
static uint32_t wf2(uint32_t input)
{
    int32_t s   = (int32_t)(int16_t)(uint16_t)(input & 0xFFFFU);
    int32_t q   = s / 0xB1;
    int32_t r   = s - q * 0xB1;
    int32_t res = (-2) * q + 0xAB * r;
    uint32_t v  = (uint32_t)res;
    if (v & 0x80000000UL)
        v += 0x763DUL;
    return v;
}

/*
 * ecu_dbl_key — applica WF1 e WF2 in cascata su seed e chiave interna.
 * Formula: WF1(seed) + WF2( WF1(ikey) + WF2(seed) )
 */
static uint32_t ecu_dbl_key(uint32_t seed, uint32_t ikey)
{
    uint32_t inner = wf1(ikey) + wf2(seed);
    return wf1(seed) + wf2(inner);
}

void mm10ja_compute_key(const uint8_t seed[4], uint8_t key[4])
{
    /* Divide il seed in due parole da 16 bit (big-endian) */
    uint32_t hi_seed = ((uint32_t)seed[0] << 8) | seed[1];
    uint32_t lo_seed = ((uint32_t)seed[2] << 8) | seed[3];

    /* La costante interna si divide anch'essa in high/low 16 bit */
    uint32_t hi_key = ecu_dbl_key(hi_seed, INTERNAL_KEY >> 16)       & 0xFFFFUL;
    uint32_t lo_key = ecu_dbl_key(lo_seed, INTERNAL_KEY & 0xFFFFUL)  & 0xFFFFUL;

    uint32_t k = (hi_key << 16) | lo_key;

    key[0] = (uint8_t)(k >> 24);
    key[1] = (uint8_t)(k >> 16);
    key[2] = (uint8_t)(k >>  8);
    key[3] = (uint8_t)(k);
}
