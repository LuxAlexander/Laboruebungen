#ifndef SERIAL_MAP_VISUAL_H
#define SERIAL_MAP_VISUAL_H

#include "../avrhal/usart/usart.h"
#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "player.h"
#include <avr/interrupt.h>
#include <util/atomic.h>

#define STAT_BAR_WIDTH 10
#define MAX_STAT_VAL 100
#define SWORD 1
#define ARMOR 2
#define GOLD_COST 10
#define FLAT_DAMAGE 10
#define FLAT_HEAL 20
#define ENEMY_ATK 5
#define ENEMY_HP 25

// health, max_health; kills; money; dmg, def
static PlayerState enemy = {25, 25, 0, 5, 5, 1};

typedef enum {
    STATE_OVERVIEW,
    STATE_DETAIL,
    STATE_CAMPFIRE,
    STATE_SHOP,
    STATE_ENEMY,
    STATE_GAMEOVER
} MapState;

typedef enum {
    ROOM_MONSTER = 0,
    ROOM_HEALTH  = 1,
    ROOM_SHOP = 2
} RoomId;

typedef enum {
    POS_LEFT  = 0,
    POS_TOP   = 1,
    POS_RIGHT = 2,
    POS_DOWN  = 3
} RoomPos;

static MapState current_state = STATE_OVERVIEW;
static RoomId dungeonMap[3];
static RoomPos selectedPos = POS_TOP;
static uint8_t needsRedraw = 1;
static uint8_t cat_notPet = 0;

static char mapGetRoomIcon(RoomId room)
{
    switch(room) {
        case ROOM_MONSTER:  return 'X';
        case ROOM_HEALTH:   return '+';
        case ROOM_SHOP:     return '$';
    }
    return '?';
}

/**
 * Startet ein Quick-Time-Event auf dem OLED Display.
 * Nutzt die echten ADC-Werte deines Joysticks zur Abfrage.
 * @return 1 bei Erfolg (Abgewehrt), 0 bei Fehlschlag (Treffer)
 */
static uint8_t mapTriggerOledQTE(uint16_t timeout_ms) {
    uint8_t eventTyp = rand() % 4; // 0 = Links, 1 = Rechts, 2 = Oben, 3 = Unten
    uint16_t verbleibendeZeit = timeout_ms;
    uint8_t erfolg = 0;
    const uint8_t tick_ms = 20;

    uint16_t adc_x = 512;
    uint16_t adc_y = 512;

    while (verbleibendeZeit > 0) {
        displayClearBuffer();

        // Titel und Aufforderung
        displayPrint(20, 0, "!!! QTE !!!");
        if (eventTyp == 0) {
            displayPrint(4, 16, "<--- LINKS!!");
        } else if (eventTyp == 1) {
            displayPrint(4, 16, "RECHTS! --->");
        } else if (eventTyp == 2) {
            displayPrint(4, 16, "OBEN! ^");
        } else {
            displayPrint(4, 16, "UNTEN! v");
        }

        // Balken berechnen (Breite max. 100 Pixel auf dem OLED)
        uint8_t balkenBreite = (uint8_t)((uint32_t)verbleibendeZeit * 100 / timeout_ms);
        displayDrawRectangle(14, 40, 100, 10);
        if (balkenBreite > 0) {
            displayDrawFilledRectangle(14, 40, balkenBreite, 10);
        }

        displayUpdate();

        // ADC Werte atomar auslesen
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            adc_x = adcLastRead(0);
        }

        // Joystick-Auswertung
        if (eventTyp == 0 && adc_x < 300) {
            erfolg = 1;
            break;
        } else if (eventTyp == 1 && adc_x > 700) {
            erfolg = 1;
            break;
        } else if (eventTyp == 2 && adc_y < 300) {
            erfolg = 1;
            break;
        } else if (eventTyp == 3 && adc_y > 700) {
            erfolg = 1;
            break;
        }

        // Falsche Richtung erwischt -> Sofort vorbei
        if (eventTyp == 0 && adc_x > 700) break;
        if (eventTyp == 1 && adc_x < 300) break;
        if (eventTyp == 2 && adc_y > 700) break;
        if (eventTyp == 3 && adc_y < 300) break;

        _delay_ms(tick_ms);
        if (verbleibendeZeit > tick_ms) {
            verbleibendeZeit -= tick_ms;
        } else {
            verbleibendeZeit = 0;
        }
    }

    // Feedback auf dem Display ausgeben
    displayClearBuffer();
    if (erfolg) {
        displayPrint(28, 20, "Gegner getroffen!");
        displayPrint(12, 36, "Katze verfehlt");
    } else {
        displayPrint(32, 20, "Gegner verfehlt!");
        displayPrint(16, 36, "Katze trifft dich!");
    }
    displayUpdate();
    _delay_ms(1500);

    return erfolg;
}

/**
 * Zeigt den Game-Over-Bildschirm auf dem OLED-Display an.
 */
static void mapDrawOledGameOver(void) {
    displayClearBuffer();
    
    // Grafischer Rahmen
    displayDrawRectangle(2, 2, 124, 60);
    
    // Text zentriert
    displayPrint(24, 12, "GAME OVER");
    displayPrint(16, 32, "Dein Abenteuer");
    displayPrint(28, 44, "endet hier!");
    
    displayUpdate();
}

static void mapInitRandomRooms(uint16_t seed)
{
    srand(seed);
    dungeonMap[POS_LEFT]  = (RoomId)(rand() % 3);
    dungeonMap[POS_TOP]   = (RoomId)(rand() % 3);
    dungeonMap[POS_RIGHT] = (RoomId)(rand() % 3);
    needsRedraw = 1;
}

static void mapClearTerminal(void)
{
    usartWriteString_P(PSTR("\033[2J\033[H"));
    for(uint8_t i = 0; i < 25; i++) {
        usartWriteString_P(PSTR("\r\n"));
    }
}

static void mapDrawOverview(void)
{
    if (selectedPos == POS_TOP) {
        usartWriteString_P(PSTR("           +=========+\r\n")); 
        usartPrint("           #    %c    #\r\n", mapGetRoomIcon(dungeonMap[POS_TOP]));
        usartWriteString_P(PSTR("           +=========+\r\n"));
    } else {
        usartWriteString_P(PSTR("           +---------+\r\n")); 
        usartPrint("           |    %c    |\r\n", mapGetRoomIcon(dungeonMap[POS_TOP]));
        usartWriteString_P(PSTR("           +---------+\r\n"));
    }

    usartWriteString_P(PSTR("                ^\r\n"));

    if (selectedPos == POS_LEFT) usartWriteString_P(PSTR("+=========+"));
    else                         usartWriteString_P(PSTR("+---------+"));

    usartWriteString_P(PSTR("           ")); 

    if (selectedPos == POS_RIGHT) usartWriteString_P(PSTR("+=========+\r\n"));
    else                          usartWriteString_P(PSTR("+---------+\r\n"));

    if (selectedPos == POS_LEFT)  usartPrint("#    %c    #", mapGetRoomIcon(dungeonMap[POS_LEFT]));
    else                          usartPrint("|    %c    |", mapGetRoomIcon(dungeonMap[POS_LEFT]));

    usartWriteString_P(PSTR("   <- ->   ")); 

    if (selectedPos == POS_RIGHT) usartPrint("#    %c    #\r\n", mapGetRoomIcon(dungeonMap[POS_RIGHT]));
    else                          usartPrint("|    %c    |\r\n", mapGetRoomIcon(dungeonMap[POS_RIGHT]));

    if (selectedPos == POS_LEFT) usartWriteString_P(PSTR("+=========+"));
    else                         usartWriteString_P(PSTR("+---------+"));

    usartWriteString_P(PSTR("           ")); 

    if (selectedPos == POS_RIGHT) usartWriteString_P(PSTR("+=========+\r\n"));
    else                          usartWriteString_P(PSTR("+---------+\r\n"));

    usartWriteString_P(PSTR("\r\nJoystick: LINKS/RECHTS/OBEN | UNTEN = Betreten\r\n"));
}

static void drawHealthRoom(void)
{
    usartWriteString_P(PSTR("\033[36m                      HEALTH ROOM\r\n\033[0m"));    
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
    usartWriteString_P(PSTR("|   |   |            _|_             _ _                     |\r\n"));
    usartWriteString_P(PSTR("|   |---|           |   |           |   |                    |\r\n"));
    usartWriteString_P(PSTR("|   |   |           |---|           |---|                    |\r\n"));
    usartWriteString_P(PSTR("|  /     \\         /     \\         /     \\     |\\_/|         |\r\n"));
    usartWriteString_P(PSTR("|  |     |         |  /\\ |         |     |     |o o|___      |\r\n"));
    usartWriteString_P(PSTR("|  |_____|         | /  \\|         |_____|     (   )   \\\\     |\r\n"));
    usartWriteString_P(PSTR("|  |     |         |/\\  /|         |     |      |_|_| |_|_|  |\r\n"));
    usartWriteString_P(PSTR("|  |     |         |  \\/ |         |     |                   |\r\n"));
    usartWriteString_P(PSTR("|=============[X]===============[X]==========================|\r\n"));
    usartWriteString_P(PSTR("|              HEALED BY THE COZY CAMPFIRE                   |\r\n"));
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
    usartWriteString_P(PSTR("\r\nJoystick: OBEN = Kater streicheln | UNTEN = Weiterreisen\r\n"));
}

static void drawGameOver(void)
{
    usartWriteString_P(PSTR("\033[31m                      GAME OVER\r\n\033[0m"));    
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
    usartWriteString_P(PSTR("|                                                            |\r\n"));
    usartWriteString_P(PSTR("|         Dein Abenteuer fand ein abruptes Ende...           |\r\n"));
    usartWriteString_P(PSTR("|                                                            |\r\n"));
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
}

static void drawShop(void)
{
    usartWriteString_P(PSTR("\033[33m                    DORFSCHMIEDE (SHOP)\r\n\033[0m"));    
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
    usartWriteString_P(PSTR("|      /| ________________                                   |\r\n"));
    usartWriteString_P(PSTR("|O|===|*|>________________>        /===============\\         |\r\n"));
    usartWriteString_P(PSTR("|      \\|                          | [ ]       [ ] |         |\r\n"));
    usartWriteString_P(PSTR("|                                  |   =========   |         |\r\n"));
    usartWriteString_P(PSTR("|     SCHWERT (Links)              |   \\       /   |         |\r\n"));
    usartWriteString_P(PSTR("|     Preis: 10 Gold               |    \\_____/    |         |\r\n"));
    usartWriteString_P(PSTR("|     (+5 DMG)                     |  RUESTUNG (Rechts)      |\r\n"));
    usartWriteString_P(PSTR("|                                  |  Preis: 10 Gold         |\r\n"));
    usartWriteString_P(PSTR("|                                  |  (+10 DEF)              |\r\n"));
    usartWriteString_P(PSTR("|============================================================|\r\n"));
    usartWriteString_P(PSTR("|              WILLKOMMEN! Kaufe Verstärkungen               |\r\n"));
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
    usartWriteString_P(PSTR("\r\nJoystick: LINKS = Schwert kaufen | RECHTS = Ruestung kaufen | UNTEN = Verlassen\r\n"));
}

static void drawEnemyRoom(void){
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
    usartWriteString_P(PSTR("|         /\\_/\\                                              |\r\n"));
    usartWriteString_P(PSTR("|        ( o.o )       [ GRRR! EIN EINDRINGLING! ]           |\r\n"));
    usartWriteString_P(PSTR("|         > ^ <                                              |\r\n"));
    usartWriteString_P(PSTR("|       --|   |--#       GEGNER: Katzen-Krieger              |\r\n"));
    usartWriteString_P(PSTR("|        /|_|_|\\         HP:     "));
    usartPrint("%2d", enemy.health);
    usartWriteString_P(PSTR(" / 25                     |\r\n"));
    usartWriteString_P(PSTR("|                        ATK:    50 DMG                      |\r\n"));
    usartWriteString_P(PSTR("|                                                            |\r\n"));
    usartWriteString_P(PSTR("|                      (Die Katzi fletscht die Zaehne...)     |\r\n"));
    usartWriteString_P(PSTR("|                                                            |\r\n"));
    usartWriteString_P(PSTR("|============================================================|\r\n"));
    usartWriteString_P(PSTR("|   Oben = Angreifen                  Unten = Fliehen        |\r\n"));
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
}  

static void mapDrawSelectedRoom(void)
{
    RoomId currentRoom = dungeonMap[selectedPos];

    switch(currentRoom)
    {
        case ROOM_MONSTER:
            playerStatsShop();
            drawEnemyRoom();
            break;

        case ROOM_HEALTH:
            drawHealthRoom();
            break;

        case ROOM_SHOP:
            playerStatsShop();
            drawShop();
            break;
        default:
            break;
    }
}

static void mapHandleInput(uint16_t adc_x, uint16_t adc_y, uint16_t dynamic_seed)
{
    static uint8_t lock = 0;

    if (adc_x > 400 && adc_x < 600 && adc_y > 400 && adc_y < 600) {
        lock = 0;
        return;
    }

    if (lock) return;

    if (current_state == STATE_OVERVIEW) {
        if (adc_x < 300) {
            if (selectedPos != POS_LEFT) { selectedPos = POS_LEFT; needsRedraw = 1; }
            lock = 1; return;
        }
        if (adc_x > 700) {
            if (selectedPos != POS_RIGHT) { selectedPos = POS_RIGHT; needsRedraw = 1; }
            lock = 1; return;
        }
        if (adc_y > 700) {
            if (selectedPos != POS_TOP) { selectedPos = POS_TOP; needsRedraw = 1; }
            lock = 1; return;
        }
        
        // UNTEN = Enter Room
        if (adc_y < 300) {
            if (dungeonMap[selectedPos] == ROOM_HEALTH) {
                playerHeal(FLAT_HEAL);
                current_state = STATE_CAMPFIRE; 
            }
            else if (dungeonMap[selectedPos] == ROOM_MONSTER) { 
                current_state = STATE_ENEMY;
            }
            else if (dungeonMap[selectedPos] == ROOM_SHOP) {
                current_state = STATE_SHOP;
            }
            needsRedraw = 1;
            lock = 1;
            return;
        }
    } 
    else if (current_state == STATE_DETAIL) {
        // OBEN = Exit Standard Room
        if (adc_y > 700) {
            current_state = STATE_OVERVIEW;
            mapInitRandomRooms(dynamic_seed);
            selectedPos = POS_TOP; 
            needsRedraw = 1;
            lock = 1;
            return;
        }
    }
    else if (current_state == STATE_CAMPFIRE) {
        // OBEN = Pet the Cat
        if (adc_y > 700) {
            mapClearTerminal();
            usartWriteString_P(PSTR("   |\\_/|   \r\n"));
            usartWriteString_P(PSTR("   ( =^.^= )  *purr*\r\n"));
            usartWriteString_P(PSTR("   (\")_(\") \r\n"));
            usartWriteString_P(PSTR("\r\nDer Kater schnurrt gluecklich!\r\n"));
            
            audio_trigger_sfx(1);
            _delay_ms(3000);

            current_state = STATE_OVERVIEW;
            mapInitRandomRooms(dynamic_seed);
            selectedPos = POS_TOP;
            needsRedraw = 1;
            lock = 1;
            return;
        }
        // UNTEN = Leave Campfire
        if (adc_y < 300) {
            mapClearTerminal();
            usartWriteString_P(PSTR("   |\\_/|   \r\n"));
            usartWriteString_P(PSTR("   ( T_T )  *miau...*\r\n"));
            usartWriteString_P(PSTR("   (\")_(\") \r\n"));
            usartWriteString_P(PSTR("\r\nDer Kater ist traurig ueber dein Aufbrechen.\r\n"));
            cat_notPet += 1;
            _delay_ms(3000);
            if(cat_notPet >= 1) {
                mapClearTerminal();
                usartWriteString_P(PSTR("   |\\_/|   \r\n"));
                usartWriteString_P(PSTR("   ( >.< )  *hisss!*\r\n"));
                usartWriteString_P(PSTR("   (\")_(\") \r\n"));
                usartWriteString_P(PSTR("\r\nDer Kater ist sauer, dass du ihn nicht gestreichelt hast. Er kratzt dich und du verlierst 10 HP!\r\n"));
                playerTakeDamage(FLAT_DAMAGE);
                _delay_ms(3000);
                if (playerGetCurrentHp() <= 0) {
                    current_state = STATE_GAMEOVER;
                    mapDrawOledGameOver();
                    needsRedraw = 1;
                    lock = 1;
                    return;
                } 
            }
            current_state = STATE_OVERVIEW;
            mapInitRandomRooms(dynamic_seed);
            selectedPos = POS_TOP;
            needsRedraw = 1;
            lock = 1;
            return;
        }
    } 
    else if (current_state == STATE_ENEMY) {
        // Down = Fliehen
        if (adc_y < 300) {
            playerTakeDamage(FLAT_DAMAGE);

            if (playerGetCurrentHp() <= 0) {
                current_state = STATE_GAMEOVER;
                mapDrawOledGameOver();
            } else {
                usartWriteString_P(PSTR("You escaped with only a scratch!\r\n"));
                _delay_ms(3000);
                current_state = STATE_OVERVIEW;
                mapInitRandomRooms(dynamic_seed);
            }
            needsRedraw = 1;
            lock = 1;
            return;
        }
        
        // Top = Angreifen (Startet das QTE!)
        if (adc_y > 700) {
            usartWriteString_P(PSTR("Kampf beginnt! Reagiere auf dem Display!\r\n"));
            _delay_ms(1500);

            while (enemy.health > 0) {
                
                uint8_t geblockt = mapTriggerOledQTE(1500); 
                
                if (!geblockt) {
                    playerTakeDamage(FLAT_DAMAGE * ENEMY_ATK / playerDef());
                } else {
                    enemy.health -= playerAtk();
                    if(enemy.health < 0){
                        enemy.health=0;
                    }
                }
                mapDrawSelectedRoom();
                if (playerGetCurrentHp() <= 0) {
                    current_state = STATE_GAMEOVER;
                    mapDrawOledGameOver();
                    break; 
                }
            }

            if (current_state == STATE_GAMEOVER) {
                needsRedraw = 1;
                lock = 1;
                return;
            } else {
                usartWriteString_P(PSTR("Kampf gewonnen!\r\n"));
                audio_trigger_sfx(2);
                _delay_ms(3000);
                current_state = STATE_OVERVIEW;
                mapInitRandomRooms(dynamic_seed);
                enemy.health = enemy.max_health;
                playerAddKill();
                playerAddMoney(enemy.money);
                needsRedraw = 1;
                lock = 1;
                return;
            }
        }
    }
    else if (current_state == STATE_GAMEOVER) {
        drawGameOver();
        // Wenn man im Game-Over-Screen den Joystick stark bewegt, startet das Spiel neu
        if (adc_x < 300 || adc_x > 700 || adc_y < 300 || adc_y > 700) {
            playerReset(); 
            current_state = STATE_OVERVIEW;
            mapInitRandomRooms(dynamic_seed);
            needsRedraw = 1;
            lock = 1;
            return;
        }
    } 
    else if (current_state == STATE_SHOP) {
        // Left = Schwert kaufen
        if (adc_x < 300) {
            playerBuy(GOLD_COST, SWORD);
            lock = 1;
            needsRedraw = 1;
            return;
        }
        // Right = Ruestung kaufen
        if (adc_x > 700) {
            playerBuy(GOLD_COST, ARMOR);
            lock = 1;
            needsRedraw = 1;
            return;
        }
        // Top = Easteregg / Info
        if (adc_y > 700) {
            usartWriteString_P(PSTR("What are you looking at?\r\n"));
            _delay_ms(1500);
            needsRedraw = 1;
            lock = 1;
            return;
        }
        // Unten = Verlassen
        if (adc_y < 300) {
            current_state = STATE_OVERVIEW;
            mapInitRandomRooms(dynamic_seed);
            needsRedraw = 1;
            lock = 1;
            return;
        }
    }
}

static void mapUpdateDisplay(void)
{
    if (needsRedraw) {
        mapClearTerminal(); 
        
        if (current_state == STATE_OVERVIEW) {
            mapDrawOverview();
        } else if (current_state != STATE_GAMEOVER) {
            mapDrawSelectedRoom();
            playerDrawHUD();
        } else {
            audio_trigger_sfx(3);
            drawGameOver();
        }
        needsRedraw = 0;
    }
}

#endif