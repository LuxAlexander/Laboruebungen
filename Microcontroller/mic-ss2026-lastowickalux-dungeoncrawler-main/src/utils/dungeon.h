#ifndef SERIAL_MAP_VISUAL_H
#define SERIAL_MAP_VISUAL_H

#include "avrhal/usart.h"
#include <stdint.h>
#include <stdlib.h>
#include <avr/pgmspace.h> // Essential for Flash optimizations
#include <util/delay.h>
#include "player_bar.h"
#include <avr/interrupt.h>
#include <util/atomic.h>

#define UNIQUE_ROOMS 3
#define STAT_BAR_WIDTH 10
#define MAX_STAT_VAL 100

#define SWORD 1
#define ARMOR 2

#define GOLD_COST 10

#define FLAT_DAMAGE 10
#define FLAT_HEAL 20

#define ENEMY_ATK 5
#define ENEMY_HP 25
#define TEN_PERCENT 10
#define MAX_KILL_CAP 9

//SFX = Sound Effects
#define SFX_PET_CAT 1
#define SFX_VICTORY 2
#define SFX_GAMEOVER 3

#define PLAYER_BASE_ATK 5
#define BASE_DEF 1
#define BASE_GOLD 0

//FLAGS
#define REDRAW_FLAG 1
#define NO_REDRAW_FLAG 0
#define LOCK_FLAG 1
#define LOCK_RESET_FLAG 0

#define CAT_PETS 0
#define CAT_AGGRESSIVE 1

// QTE = Quick-Time-Event
#define QTE_DIRECTIONS 2
#define QTE_LEFT 0
#define QTE_RIGHT 1
#define QTE_TICKS_MS 20

#define QTE_BAR_X 14
#define QTE_BAR_Y 40
#define QTE_BAR_WIDTH 100
#define QTE_BAR_HEIGHT 10

#define QTE_TITLE_X 20
#define QTE_TITLE_Y 0

#define QTE_TEXT_X 4
#define QTE_TEXT_Y 16

#define QTE_RESULT_X 28
#define QTE_RESULT_Y 20

#define QTE_INFO_X 12
#define QTE_INFO_Y 36

//DISPLAY GameOver
#define GAMEOVER_BOX_X 2
#define GAMEOVER_BOX_Y 2
#define GAMEOVER_BOX_W 124
#define GAMEOVER_BOX_H 60

// ADC thresholds for joystick directions
#define ADC_CENTER_VALUE 512
#define ADC_CENTER_MIN 400
#define ADC_CENTER_MAX 600

#define ADC_MIN_CHECK 300
#define ADC_MAX_CHECK 700

#define ADC_X 0

#define SHORT_TIMEOUT_MS 1500
#define SCREEN_DELAY_MS 3000

//Terminal Clear
#define TERMINAL_CLEAR_LINES 25

// health, max_health; kills; money; dmg, def
static PlayerState enemy = {ENEMY_HP, ENEMY_HP, BASE_GOLD, ENEMY_ATK, ENEMY_ATK, BASE_DEF};

typedef enum
{
    STATE_OVERVIEW,
    STATE_CAMPFIRE,
    STATE_SHOP,
    STATE_ENEMY,
    STATE_GAMEOVER
} MapState;

typedef enum
{
    ROOM_MONSTER = 0,
    ROOM_HEALTH = 1,
    ROOM_SHOP = 2
} RoomId;

typedef enum
{
    POS_LEFT = 0,
    POS_TOP = 1,
    POS_RIGHT = 2
} RoomPos;

static MapState current_state = STATE_OVERVIEW;
static RoomId dungeonMap[UNIQUE_ROOMS];
static RoomPos selectedPos = POS_TOP;
static uint8_t needsRedraw = REDRAW_FLAG;
static uint8_t cat_notPet = CAT_PETS;

static char mapGetRoomIcon(RoomId room)
{
    switch (room)
    {
    case ROOM_MONSTER:
        return 'X';
    case ROOM_HEALTH:
        return '+';
    case ROOM_SHOP:
        return '$';
    }
    return '?';
}

/**
 * Startet ein Quick-Time-Event auf dem OLED Display.
 * Nutzt die echten ADC-Werte deines Joysticks zur Abfrage.
 * @return 1 bei Erfolg (Abgewehrt), 0 bei Fehlschlag (Treffer)
 */
static uint8_t mapTriggerOledQTE(uint16_t timeout_ms)
{
    uint8_t eventTyp = rand() % QTE_DIRECTIONS; // 0 = Links, 1 = Rechts

    uint16_t kills = getKills();
    if (kills > MAX_KILL_CAP)
        kills = MAX_KILL_CAP; // Begrenzt die Kills auf maximal 10

    uint16_t verbleibendeZeit = (timeout_ms * (TEN_PERCENT - kills)) / TEN_PERCENT;
    uint8_t erfolg = 0;
    const uint8_t tick_ms = QTE_TICKS_MS;

    uint16_t adc_x = ADC_CENTER_VALUE;

    while (verbleibendeZeit > 0)
    {
        displayClearBuffer();

        // Titel und Aufforderung
        displayPrint(QTE_TITLE_X, QTE_TITLE_Y, "!!! QTE !!!");
        if (eventTyp == QTE_LEFT)
        {
            displayPrint(QTE_TEXT_X, QTE_TEXT_Y, "<--- LINKS!");
        }
        else
        {
            displayPrint(QTE_TEXT_X, QTE_TEXT_Y, "RECHTS! --->");
        }

        // Balken berechnen (Breite max. 100 Pixel auf dem OLED)
        uint8_t balkenBreite = (uint8_t)((uint32_t)verbleibendeZeit * QTE_BAR_WIDTH/ timeout_ms);
        // Balken zeichnen, der sich von voll (100 Pixel) auf leer (0 Pixel) reduziert
        displayDrawRectangle(QTE_BAR_X, QTE_BAR_Y, QTE_BAR_WIDTH, QTE_BAR_HEIGHT);
        if (balkenBreite > 0)
        {
            displayDrawFilledRectangle(QTE_BAR_X, QTE_BAR_Y, balkenBreite, QTE_BAR_HEIGHT);
        }

        displayUpdate();

        // ADC Werte atomar auslesen
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
        {
            adc_x = adcLastRead(ADC_X);
        }

        // Joystick-Auswertung
        if (eventTyp == QTE_LEFT && adc_x < ADC_MIN_CHECK)
        {
            erfolg = 1;
            break;
        }
        else if (eventTyp == QTE_RIGHT && adc_x > ADC_MAX_CHECK)
        {
            erfolg = 1;
            break;
        }

        // Falsche Richtung erwischt -> Sofort vorbei
        if (eventTyp == QTE_LEFT && adc_x > ADC_MAX_CHECK)
            break;
        if (eventTyp == QTE_RIGHT && adc_x < ADC_MIN_CHECK)
            break;

        _delay_ms(tick_ms);
        if (verbleibendeZeit > tick_ms)
        {
            verbleibendeZeit -= tick_ms;
        }
        else
        {
            verbleibendeZeit = 0;
        }
    }

    // Feedback auf dem Display ausgeben
    displayClearBuffer();
    if (erfolg)
    {
        displayPrint(QTE_RESULT_X, QTE_RESULT_Y, "GEBLOCKT!");
        displayPrint(QTE_INFO_X, QTE_INFO_Y, "Katze verfehlt");
    }
    else
    {
        displayPrint(QTE_RESULT_X, QTE_RESULT_Y, "TREFFER!");
        displayPrint(QTE_INFO_X, QTE_INFO_Y, "Du verlierst HP");
    }
    displayUpdate();
    _delay_ms(SHORT_TIMEOUT_MS);

    return erfolg;
}

/**
 * Zeigt den Game-Over-Bildschirm auf dem OLED-Display an.
 */
static void mapDrawOledGameOver(void)
{
    displayClearBuffer();

    // Grafischer Rahmen
    displayDrawRectangle(GAMEOVER_BOX_X, GAMEOVER_BOX_Y, GAMEOVER_BOX_W, GAMEOVER_BOX_H);

    // Text zentriert
    displayPrint(24, 12, "GAME OVER");
    displayPrint(16, 32, "Dein Abenteuer");
    displayPrint(28, 44, "endet hier!");

    displayUpdate();
}

static void mapInitRandomRooms(uint16_t seed)
{
    srand(seed);
    dungeonMap[POS_LEFT] = (RoomId)(rand() % UNIQUE_ROOMS);
    dungeonMap[POS_TOP] = (RoomId)(rand() % UNIQUE_ROOMS);
    dungeonMap[POS_RIGHT] = (RoomId)(rand() % UNIQUE_ROOMS);
    needsRedraw = REDRAW_FLAG;
}

static void mapClearTerminal(void)
{
    // this works on Standard Linux/Unix consoles, macOS Terminal, and modern Linux terminal emulators
    // Modern Windows
    // clear the screen and reset the cursor position
    usartWriteString_P(PSTR("\033[2J\033[H"));
    for (uint8_t i = 0; i < TERMINAL_CLEAR_LINES; i++)
    {
        usartWriteString_P(PSTR("\r\n")); // 25 Zeilen, weiter Rücken, fallback
    }
}

static void mapDrawOverview(void)
{
    if (selectedPos == POS_TOP)
    {
        usartWriteString_P(PSTR("           +=========+\r\n"));
        usartPrint("           #    %c    #\r\n", mapGetRoomIcon(dungeonMap[POS_TOP]));
        usartWriteString_P(PSTR("           +=========+\r\n"));
    }
    else
    {
        usartWriteString_P(PSTR("           +---------+\r\n"));
        usartPrint("           |    %c    |\r\n", mapGetRoomIcon(dungeonMap[POS_TOP]));
        usartWriteString_P(PSTR("           +---------+\r\n"));
    } // might look the same but it'S if a room is selected = or not -

    usartWriteString_P(PSTR("                ^\r\n"));

    if (selectedPos == POS_LEFT)
        usartWriteString_P(PSTR("+=========+"));
    else
        usartWriteString_P(PSTR("+---------+"));

    usartWriteString_P(PSTR("           "));

    if (selectedPos == POS_RIGHT)
        usartWriteString_P(PSTR("+=========+\r\n"));
    else
        usartWriteString_P(PSTR("+---------+\r\n"));

    if (selectedPos == POS_LEFT)
        usartPrint("#    %c    #", mapGetRoomIcon(dungeonMap[POS_LEFT]));
    else
        usartPrint("|    %c    |", mapGetRoomIcon(dungeonMap[POS_LEFT]));

    usartWriteString_P(PSTR("   <- ->   "));

    if (selectedPos == POS_RIGHT)
        usartPrint("#    %c    #\r\n", mapGetRoomIcon(dungeonMap[POS_RIGHT]));
    else
        usartPrint("|    %c    |\r\n", mapGetRoomIcon(dungeonMap[POS_RIGHT]));

    if (selectedPos == POS_LEFT)
        usartWriteString_P(PSTR("+=========+"));
    else
        usartWriteString_P(PSTR("+---------+"));

    usartWriteString_P(PSTR("           "));

    if (selectedPos == POS_RIGHT)
        usartWriteString_P(PSTR("+=========+\r\n"));
    else
        usartWriteString_P(PSTR("+---------+\r\n"));

    usartWriteString_P(PSTR("\r\nJoystick: LINKS/RECHTS/OBEN | UNTEN = Betreten\r\n"));
}

static void drawHealthRoom(void)
{
    //\033[36m = Cyan, \033[0m = Reset
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
    //\033[31m = Rot, \033[0m = Reset
    usartWriteString_P(PSTR("\033[31m                      GAME OVER\r\n\033[0m"));
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
    usartWriteString_P(PSTR("|                                                            |\r\n"));
    usartWriteString_P(PSTR("|         Dein Abenteuer fand ein abruptes Ende...           |\r\n"));
    usartWriteString_P(PSTR("|                                                            |\r\n"));
    usartWriteString_P(PSTR("+------------------------------------------------------------+\r\n"));
}

static void drawShop(void)
{
    //\033[33m = Gelb, \033[0m = Reset
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

static void drawEnemyRoom(void)
{
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

    switch (currentRoom)
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
    static uint8_t lock = LOCK_RESET_FLAG;

    if (adc_x > ADC_CENTER_MIN && adc_x < ADC_CENTER_MAX && adc_y > ADC_CENTER_MIN && adc_y < ADC_CENTER_MAX)
    {
        lock = LOCK_RESET_FLAG;
        return;
    }

    if (lock)
        return;

    if (current_state == STATE_OVERVIEW)
    {
        if (adc_x < ADC_MIN_CHECK)
        {
            if (selectedPos != POS_LEFT)
            {
                selectedPos = POS_LEFT;
                needsRedraw = REDRAW_FLAG;
            }
            lock = LOCK_FLAG;
            return;
        }
        if (adc_x > ADC_MAX_CHECK)
        {
            if (selectedPos != POS_RIGHT)
            {
                selectedPos = POS_RIGHT;
                needsRedraw = REDRAW_FLAG;
            }
            lock = LOCK_FLAG;
            return;
        }
        if (adc_y > ADC_MAX_CHECK)
        {
            if (selectedPos != POS_TOP)
            {
                selectedPos = POS_TOP;
                needsRedraw = REDRAW_FLAG;
            }
            lock = LOCK_FLAG;
            return;
        }

        // UNTEN = Enter Room
        if (adc_y < ADC_MIN_CHECK)
        {
            if (dungeonMap[selectedPos] == ROOM_HEALTH)
            {
                playerHeal(FLAT_HEAL);
                current_state = STATE_CAMPFIRE;
            }
            else if (dungeonMap[selectedPos] == ROOM_MONSTER)
            {
                current_state = STATE_ENEMY;
            }
            else if (dungeonMap[selectedPos] == ROOM_SHOP)
            {
                current_state = STATE_SHOP;
            }
            needsRedraw = REDRAW_FLAG;
            lock = LOCK_FLAG;
            return;
        }
    }
    else if (current_state == STATE_CAMPFIRE)
    {
        // OBEN = Pet the Cat
        if (adc_y > ADC_MAX_CHECK)
        {
            mapClearTerminal();
            usartWriteString_P(PSTR("   |\\_/|   \r\n"));
            usartWriteString_P(PSTR("   ( =^.^= )  *purr*\r\n"));
            usartWriteString_P(PSTR("   (\")_(\") \r\n"));
            usartWriteString_P(PSTR("\r\nDer Kater schnurrt gluecklich!\r\n"));
            audio_trigger_sfx(SFX_PET_CAT);
            _delay_ms(SCREEN_DELAY_MS);

            current_state = STATE_OVERVIEW;
            mapInitRandomRooms(dynamic_seed);
            selectedPos = POS_TOP;
            needsRedraw = REDRAW_FLAG;
            lock = LOCK_FLAG;
            return;
        }
        // UNTEN = Leave Campfire
        if (adc_y < ADC_MIN_CHECK)
        {
            mapClearTerminal();
            usartWriteString_P(PSTR("   |\\_/|   \r\n"));
            usartWriteString_P(PSTR("   ( T_T )  *miau...*\r\n"));
            usartWriteString_P(PSTR("   (\")_(\") \r\n"));
            usartWriteString_P(PSTR("\r\nDer Kater ist traurig ueber dein Aufbrechen.\r\n"));
            cat_notPet++;
            _delay_ms(SCREEN_DELAY_MS);
            if (cat_notPet >= CAT_AGGRESSIVE)
            {
                mapClearTerminal();
                usartWriteString_P(PSTR("   |\\_/|   \r\n"));
                usartWriteString_P(PSTR("   ( >.< )  *hisss!*\r\n"));
                usartWriteString_P(PSTR("   (\")_(\") \r\n"));
                usartWriteString_P(PSTR("\r\nDer Kater ist sauer, dass du ihn nicht gestreichelt hast. Er kratzt dich und du verlierst 10 HP!\r\n"));
                playerTakeDamage(FLAT_DAMAGE);
                _delay_ms(SCREEN_DELAY_MS);
                if (playerGetCurrentHp() <= 0)
                {
                    current_state = STATE_GAMEOVER;
                    mapDrawOledGameOver();
                    needsRedraw = REDRAW_FLAG;
                    lock = LOCK_FLAG;
                    return;
                }
            }
            current_state = STATE_OVERVIEW;
            mapInitRandomRooms(dynamic_seed);
            selectedPos = POS_TOP;
            needsRedraw = REDRAW_FLAG;
            lock = LOCK_FLAG;
            return;
        }
    }
    else if (current_state == STATE_ENEMY)
    {
        // Down = Fliehen
        if (adc_y < ADC_MIN_CHECK)
        {
            playerTakeDamage(FLAT_DAMAGE);

            if (playerGetCurrentHp() <= 0)
            {
                current_state = STATE_GAMEOVER;
                mapDrawOledGameOver();
            }
            else
            {
                usartWriteString_P(PSTR("You escaped with only a scratch!\r\n"));
                _delay_ms(SCREEN_DELAY_MS);
                current_state = STATE_OVERVIEW;
                mapInitRandomRooms(dynamic_seed);
            }
            needsRedraw = REDRAW_FLAG;
            lock = LOCK_FLAG;
            return;
        }

        // Top = Angreifen (Startet das QTE!)
        if (adc_y > ADC_MAX_CHECK)
        {
            usartWriteString_P(PSTR("Kampf beginnt! Reagiere auf dem Display!\r\n"));
            _delay_ms(SHORT_TIMEOUT_MS);

            while (enemy.health > 0)
            {

                uint8_t geblockt = mapTriggerOledQTE(SHORT_TIMEOUT_MS);

                if (!geblockt)
                {
                    uint32_t base_damage = (uint32_t)FLAT_DAMAGE * ENEMY_ATK;

                    uint32_t calculated_damage = (base_damage * TEN_PERCENT) / playerDef();

                    uint32_t min_damage = base_damage / TEN_PERCENT;

                    if (calculated_damage < min_damage)
                    {
                        calculated_damage = min_damage;
                    }

                    if (calculated_damage > base_damage)
                    {
                        calculated_damage = base_damage;
                    }

                    playerTakeDamage((uint16_t)calculated_damage);
                }
                else
                {
                    enemy.health -= playerAtk();
                    if (enemy.health < 0)
                    {
                        enemy.health = 0;
                    }
                }
                mapDrawSelectedRoom();
                if (playerGetCurrentHp() <= 0)
                {
                    current_state = STATE_GAMEOVER;
                    mapDrawOledGameOver();
                    break;
                }
            }

            if (current_state == STATE_GAMEOVER)
            {
                needsRedraw = REDRAW_FLAG;
                lock = LOCK_FLAG;
                return;
            }
            else
            {
                usartWriteString_P(PSTR("Kampf gewonnen!\r\n"));
                audio_trigger_sfx(SFX_VICTORY);
                _delay_ms(SCREEN_DELAY_MS);
                current_state = STATE_OVERVIEW;
                mapInitRandomRooms(dynamic_seed);
                enemy.health = enemy.max_health;
                playerAddMoney(enemy.money);
                playerAddKill();
                needsRedraw = REDRAW_FLAG;
                lock = LOCK_FLAG;
                return;
            }
        }
    }
    else if (current_state == STATE_GAMEOVER)
    {
        drawGameOver();
        // Wenn man im Game-Over-Screen den Joystick stark bewegt, startet das Spiel neu
        if (adc_x < ADC_MIN_CHECK || adc_x > ADC_MAX_CHECK || adc_y < ADC_MIN_CHECK || adc_y > ADC_MAX_CHECK)
        {
            playerReset();
            current_state = STATE_OVERVIEW;
            mapInitRandomRooms(dynamic_seed);
            needsRedraw = REDRAW_FLAG;
            lock = LOCK_FLAG;
            return;
        }
    }
    else if (current_state == STATE_SHOP)
    {
        // Left = Schwert kaufen
        if (adc_x < ADC_MIN_CHECK)
        {
            playerBuy(GOLD_COST, SWORD);
            lock = LOCK_FLAG;
            needsRedraw = REDRAW_FLAG;
            return;
        }
        // Right = Ruestung kaufen
        if (adc_x > ADC_MAX_CHECK)
        {
            playerBuy(GOLD_COST, ARMOR);
            lock = LOCK_FLAG;
            needsRedraw = REDRAW_FLAG;
            return;
        }
        // Top = Easteregg / Info
        if (adc_y > ADC_MAX_CHECK)
        {
            usartWriteString_P(PSTR("What are you looking at?\r\n"));
            _delay_ms(SHORT_TIMEOUT_MS);
            needsRedraw = REDRAW_FLAG;
            lock = LOCK_FLAG;
            return;
        }
        // Unten = Verlassen
        if (adc_y < ADC_MIN_CHECK)
        {
            current_state = STATE_OVERVIEW;
            mapInitRandomRooms(dynamic_seed);
            needsRedraw = REDRAW_FLAG;
            lock = LOCK_FLAG;
            return;
        }
    }
}

static void mapUpdateDisplay(void)
{
    if (needsRedraw)
    {
        mapClearTerminal();

        if (current_state == STATE_OVERVIEW)
        {
            mapDrawOverview();
        }
        else if (current_state != STATE_GAMEOVER)
        {
            mapDrawSelectedRoom();
            playerDrawHUD();
        }
        else
        {
            audio_trigger_sfx(SFX_GAMEOVER);
            drawGameOver();
        }
        needsRedraw = NO_REDRAW_FLAG;
    }
}

#endif