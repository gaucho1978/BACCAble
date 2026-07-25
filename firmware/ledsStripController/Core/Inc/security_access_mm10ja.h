/*
 * security_access_mm10ja.h
 *
 * Algoritmo UDS Security Access (servizio 0x27) per ECM Magneti Marelli MM10JA
 * (Alfa Romeo Giulia, Stelvio e derivati FCA/Stellantis con ECU su CAN ID 0x18DA10F1).
 *
 * ---------------------------------------------------------------------------
 * COME USARE
 * ---------------------------------------------------------------------------
 *
 * Il protocollo UDS Security Access funziona in due passi:
 *
 *  1. RICHIESTA SEED  — invia il messaggio 0x27 0x01 alla ECU (livello 1).
 *     La ECU risponde con 0x67 0x01 seguito da 4 byte di seed casuali.
 *
 *  2. INVIO KEY       — chiama mm10ja_compute_key() per calcolare la key
 *     a partire dal seed, poi invia 0x27 0x02 + i 4 byte di key alla ECU.
 *     Se la key è corretta la ECU risponde con 0x67 0x02 (accesso concesso).
 *
 * ---------------------------------------------------------------------------
 * ESEMPIO
 * ---------------------------------------------------------------------------
 *
 *   // Seed ricevuto dalla ECU (risposta 0x67 0x01):
 *   uint8_t seed[4] = { 0x90, 0x98, 0x7E, 0x36 };
 *
 *   // Calcolo della key:
 *   uint8_t key[4];
 *   mm10ja_compute_key(seed, key);
 *   // key risultante: { 0x4C, 0xA4, 0x46, 0xE7 }  <- verificato su sniffata reale
 *
 *   // Costruisci il messaggio UDS 0x27 0x02 con i 4 byte di key e invialo alla ECU.
 *
 * ---------------------------------------------------------------------------
 * ALGORITMO
 * ---------------------------------------------------------------------------
 *
 * Algoritmo originale: S1_FGA_ORIGINAL_ECU_Sup0002, livello 1.
 * Estratto dal file EFD (JS) del software diagnostico EOBD.
 * Costante interna: 0x17591215 (split in high/low 16 bit per i due semiseed).
 *
 * Nessuna dipendenza esterna — solo <stdint.h>.
 * Testato su STM32F072 (Cortex-M0, no FPU, no divisore HW).
 *
 * ---------------------------------------------------------------------------
 */

#ifndef SECURITY_ACCESS_MM10JA_H
#define SECURITY_ACCESS_MM10JA_H

#include <stdint.h>

/**
 * Calcola la key UDS 0x27 per l'ECM Magneti Marelli MM10JA (ECU 0x10).
 *
 * @param seed  Puntatore ai 4 byte di seed ricevuti dalla ECU (risposta 0x67 0x01).
 * @param key   Puntatore al buffer da 4 byte dove verrà scritta la key da inviare (0x27 0x02).
 */
void mm10ja_compute_key(const uint8_t seed[4], uint8_t key[4]);

#endif /* SECURITY_ACCESS_MM10JA_H */
