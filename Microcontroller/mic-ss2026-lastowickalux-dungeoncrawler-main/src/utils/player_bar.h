#ifndef PLAYER_H
#define PLAYER_H

#include "avrhal/usart.h"
#include <stdint.h>

#define HEALTH_BAR_WIDTH 10
#define DAMAGE_INCREASE 5
#define DEFENSE_INCREASE 10

typedef struct {
    int16_t health;
    int16_t max_health;
    uint16_t kills;
    uint16_t money;
    uint16_t dmg; // Attack-Damage
    uint16_t def; // Defence
} PlayerState;

// Globaler Spieler-Status (Statisch initialisiert für das einfache Einbinden)
static PlayerState player = {100, 100, 0, 0, 10, 1};

/**
 * Setzt den Spieler auf die Startwerte zurück
 */
static void playerReset(void)
{
    player.health = 100;
    player.max_health = 100;
    player.kills = 0;
    player.money = 0;
    player.dmg = 10;
    player.def = 1;
}

/**
 * Zeichnet das komplette HUD (HP-Bar, Kills, Gold)
 * Jedes Segment wird einzeln gesendet, um den USART-Puffer zu schonen!
 */
static void playerDrawHUD(void)
{
    // 1. Berechnung wie viele '#' gezeichnet werden müssen (0 bis HEALTH_BAR_WIDTH)
    int16_t segments = 0;
    if (player.health > 0) {
        segments = (player.health * HEALTH_BAR_WIDTH) / player.max_health;
    }
    if (segments > HEALTH_BAR_WIDTH) segments = HEALTH_BAR_WIDTH;

    // 2. HP-Bar zeichnen
    usartPrint("HP: [");
    for (uint8_t i = 0; i < HEALTH_BAR_WIDTH; i++) {
        if (i < segments) {
            usartPrint("#");
        } else {
            usartPrint(".");
        }
    }
    usartPrint("] ");

    // 3. Zahlenwerte ausgeben
    usartPrint("%d/%d ", player.health, player.max_health);
    usartPrint("| Kills: %u ", player.kills);
    usartPrint("| Gold: %u$\r\n", player.money);
    usartPrint("+------------------------------------------------------------+\r\n");
}

// Print Shop Player Stats
static void playerStatsShop()
{
    usartPrint("%d/%d ", player.health, player.max_health);
    usartPrint("| Damage: %u ", player.dmg);
    usartPrint("| Defence: %u ", player.def);
    usartPrint("| Gold: %u$\r\n", player.money);
}

/**
 * Fügt dem Spieler Schaden zu
 */
static void playerTakeDamage(int16_t damage)
{
    player.health -= damage;
    if (player.health < 0) {
        player.health = 0;
    }
}

/**
 * Heilt den Spieler
 */
static void playerHeal(int16_t amount)
{
    player.health += amount;
    if (player.health > player.max_health) {
        player.health = player.max_health;
    }
}

/**
 * Erhöht den Kill-Counter
 */
static void playerAddKill(void)
{
    player.kills++;
}

/**
 * Erhöht das Vermögen
 */
static void playerAddMoney(uint16_t amount)
{
    player.money += amount;
}

static void playerAddDmg(uint16_t amount) 
{
    player.dmg += amount;
}

int16_t playerGetCurrentHp() 
{
    return player.health;
}

static void playerAddDef(uint16_t amount) 
{
    player.def += amount;
}

uint16_t playerDef() 
{
    return player.def;
}

uint16_t playerAtk() 
{
    return player.dmg;
}

static void playerBuy(uint16_t amount, uint16_t option)
{
    if (amount <= player.money) {
        player.money -= amount;
        usartWriteString_P(PSTR("Transaktion erfolgreich!\r\n"));
        if (option == 1) {
            playerAddDmg(DAMAGE_INCREASE);
        } else if (option == 2) {
            playerAddDef(DEFENSE_INCREASE);
        }
    }
    else {
        usartWriteString_P(PSTR("Nicht ausreichend Gold!\r\n"));
    }
}

uint16_t getKills() {
    return player.kills;
} 


#endif