#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "raylib.h"
#include "raymath.h"

typedef struct string_t
{
    char *str;
    int length;
} string_t;

typedef struct window_t
{
    int height;
    int width;
    string_t title;
} window_t;

window_t g_main_window = {
    .width = 1920,
    .height = 1080,
    .title = {"My Window", 8}};

// level
typedef enum
{
    GameState_NULL,
    GameState_Start,
    GameState_Level_1,
    GameState_Game_Over,
    GameState_Size
} GameState;

// player
typedef struct
{
    Vector2 directionVector;
    Vector2 velocity;
    Vector2 playerPos;
    float speed;
    float maxSpeed;
    float acceleration;
    float stopSpeed;
    int hp;

} Player;

// bullet
//------------------------------------------------------------------------------

typedef enum
{
    BULLET_TYPE_PLAYER_STANDARD,
    BULLET_TYPE_PLAYER_EXTRA_POWER,
    BULLET_TYPE_ENEMY,
    BULLET_TYPE_ENEMY_MISSALES,
    BULLET_TYPE_NUM_BULLET_TYPES
} bullet_type_e;

typedef struct
{
    float speed;
    float damage;
} BulletTypeData;

BulletTypeData bulletTypeData[] =
    {
        {
            .speed = 1,
            .damage = 1,
        },
        {
            .speed = 2,
            .damage = 2,
        },
        {
            .speed = 1.5,
            .damage = 1,
        },
        {
            .speed = 800,
            .damage = 1,
        },
};

typedef struct
{
    Vector2 pos;
    bullet_type_e type;
} Bullet;

// Enemy
//------------------------------------------------------------------------------

typedef enum
{
    Enemy_Type_Zombie,
    Enemy_Type_Zombie_Gunner,
    Enemy_Type_Barricade,
    Enemy_Type_Witch,
    Enemy_Type_Types,
} EnemyType;

typedef struct
{
    Vector2 targetPoint;
    Vector2 pos;
    int currentHp;
    float fireRateTimer;
    EnemyType enemyType;
    int step;
} Enemy;

typedef struct
{
    int hp;
    float speed;
    float fireRate;
    int baseWaveSize;
    int waveScaling;
    float size;
} EnemyTypeData;

EnemyTypeData enemyTypeData[] =
    {
        {
            // zombie
            .hp = 1,
            .speed = 360,
            .fireRate = 0,
            .baseWaveSize = 4,
            .waveScaling = 4,
            .size = 25,
        },
        {
            // zombie gunner
            .hp = 1,
            .speed = 400,
            .fireRate = 1,
            .baseWaveSize = 2,
            .waveScaling = 2,
            .size = 20,
        },
        {
            // baricade
            .hp = 5,
            .speed = 800,
            .fireRate = 0,
            .baseWaveSize = 1,
            .waveScaling = 2,
            .size = 45,
        },
        {
            // witch
            .hp = 3,
            .speed = 400,
            .fireRate = 1,
            .baseWaveSize = 1,
            .waveScaling = 0,
            .size = 20,
        }};

EnemyType enemyTypeLevel1[] = {
    Enemy_Type_Zombie,
    Enemy_Type_Zombie_Gunner,
    Enemy_Type_Zombie,
    Enemy_Type_Zombie,
    Enemy_Type_Barricade,
};

EnemyType enemyTypeLevel2[] = {
    Enemy_Type_Witch,
    Enemy_Type_Barricade,
    Enemy_Type_Barricade,
    Enemy_Type_Zombie_Gunner,
    Enemy_Type_Zombie_Gunner,
    Enemy_Type_Zombie,
    Enemy_Type_Zombie,
};

// Pickupp
//------------------------------------------------------------------------------
typedef enum
{
    PICKUPP_HP,
    PICKUPP_UPPGRADE_1,
    PICKUPP_UPPGRADE_2,
    PICKUPP_UPPGRADE_3,
    PICKUPP_UPPGRADE_4,
    PICKUPP_SIZE,
} PickuppTyps;

typedef struct
{
    Vector2 pos;
    PickuppTyps typ;
} Pickupp;

// data varibals
//------------------------------------------------------------------------------

float backgrundspeed = 90;
int pickuppNum = 0;

int waveCount = 0;
int waveHoldeValue = 0;

int score = 0;

// funktioner
//------------------------------------------------------------------------------

void DebugMouse()
{
    //  Mouse pos draw
    Vector2 mousePosition;
    mousePosition = GetMousePosition();
    DrawText(TextFormat("x: %2f\ty: %2f", mousePosition.x, mousePosition.y), 8, 8, 21, GREEN);
}

float lerp(float a, float b, float t)
{
    return (a * (1.0 - t)) + (b * t);
}

int RandomIntRange(int lower, int upper)
{
    int num = (rand() % (upper - lower + 1)) + lower;
    return num;
}

// Player
//------------------------------------------------------------------------------

Vector2 GetDirctionInput(Vector2 directionVector)
{
    directionVector = Vector2Zero();

    if (IsKeyDown(KEY_W))
    {
        directionVector.y--;
    }
    if (IsKeyDown(KEY_S))
    {
        directionVector.y++;
    }

    if (IsKeyDown(KEY_D))
    {
        directionVector.x++;
    }
    if (IsKeyDown(KEY_A))
    {
        directionVector.x--;
    }
    return directionVector;
}

Player CalculatMovement(Player player)
{
    // movement
    //------------------------------------------------------------------------------
    player.directionVector = Vector2Normalize(player.directionVector);

    if (Vector2LengthSqr(player.directionVector) > 0.1)
    {
        player.speed = lerp(player.speed, player.maxSpeed, player.acceleration * GetFrameTime());
        player.velocity = Vector2Scale(player.directionVector, player.speed);
    }
    else
    {
        player.speed = lerp(player.speed, 0, player.stopSpeed * GetFrameTime());

        player.velocity = Vector2Lerp(player.velocity, Vector2Zero(), player.stopSpeed * GetFrameTime());
    }

    //  adding movement
    player.playerPos = Vector2Add(player.playerPos, player.velocity);
    return player;
}

void AnimatePlayer(Vector2 playerPos)
{
    DrawCircle(playerPos.x, playerPos.y, 40, GREEN);
}

Vector2 CheckOutOfBoundes(int screenWidth, int screenHeight, Vector2 playerPos, Vector2 velocity, int radius)
{
    bool redo = false;

    if (playerPos.x < 0 + radius)
    {
        playerPos.x -= velocity.x;
    }
    if (playerPos.x > screenWidth - radius)
    {
        playerPos.x -= velocity.x;
    }
    if (playerPos.y < (screenHeight / 5) + radius)
    {
        playerPos.y -= velocity.y;
    }
    if (playerPos.y > screenHeight - radius)
    {
        playerPos.y -= velocity.y;
    }

    return playerPos;
}

GameState isPlayerDead(int playerHp, GameState gamestate)
{

    if (playerHp < 1)
    {
        gamestate = GameState_Game_Over;
    }
    return gamestate;
}

void drawPlayerHp(int hp, int screenHeight)
{
    for (int i = 0; i < hp; i++)
    {
        DrawCircle(20 + i * 40, screenHeight / 20, 20, RED);
    }
}

// bullet
//------------------------------------------------------------------------------

// bullet start
int SetBulletOn(int *drawBulet, int bulletNum, Bullet *bullet, Vector2 pos, int maxBullets, bullet_type_e type)
{
    // selcet bullet
    switch (type)
    {
    case BULLET_TYPE_PLAYER_STANDARD:

        bullet[bulletNum].pos = pos;
        bullet[bulletNum].type = BULLET_TYPE_PLAYER_STANDARD;

        break;
    case BULLET_TYPE_ENEMY:

        bullet[bulletNum].pos = pos;
        bullet[bulletNum].type = BULLET_TYPE_ENEMY;

        break;
    case BULLET_TYPE_ENEMY_MISSALES:
        bullet[bulletNum].pos = pos;
        bullet[bulletNum].type = BULLET_TYPE_ENEMY_MISSALES;

        break;
    default:
        break;
    }
    // general
    drawBulet[bulletNum] = 1;

    if (bulletNum == maxBullets)
    {
        bulletNum = 0;
    }
    bulletNum++;

    return bulletNum;
}

// bullet uppdate
void UppdateBulet(int *drawBullet, Bullet *bullet, int maxBullets, int screenWidth, BulletTypeData *bulletTypeData, Vector2 playerPos)
{
    for (int i = 0; i < maxBullets; i++)
    {
        if (drawBullet[i] == 1)
        {
            switch (bullet[i].type)
            {
            case BULLET_TYPE_PLAYER_STANDARD:
            {
                bullet[i].pos.x += bulletTypeData[BULLET_TYPE_PLAYER_STANDARD].speed;

                // edge
                if (bullet[i].pos.x > screenWidth + 10)
                {
                    drawBullet[i] = 0;
                }
            }
            break;
            case BULLET_TYPE_ENEMY:
            {
                bullet[i].pos.x -= bulletTypeData[BULLET_TYPE_ENEMY].speed;

                // edge
                if (bullet[i].pos.x < -10)
                {
                    drawBullet[i] = 0;
                }
            }
            break;
            case BULLET_TYPE_ENEMY_MISSALES:
            {
                Vector2 velocity;
                velocity = Vector2Subtract(playerPos, bullet[i].pos);
                velocity = Vector2Normalize(velocity);

                velocity = Vector2Add(Vector2Scale(Vector2Scale(velocity, bulletTypeData[bullet[i].type].speed), GetFrameTime()), bullet[i].pos);

                // printf("x %f ,y %f Vel mis \n", velocity.x, velocity.y);

                bullet[i].pos = velocity;
            }
            break;

            default:
                printf("somting is wrong | bullet uppdate \n");
                break;
            }
        }
    }
}

// creat uppgrade

void spawnUppgrade(Enemy *enemyArray, Pickupp *pickuppArray, int *pickuppAlive, int k)
{
    int chanceForUppgrade = RandomIntRange(0, 4);

    if (chanceForUppgrade == 0)
    {
        pickuppArray[pickuppNum].pos = enemyArray[k].pos;
        pickuppAlive[pickuppNum] = 1;
        pickuppNum++;
    }
    if (pickuppNum >= 5)
    {
        pickuppNum = 0;
    }
}

float PickUppPickupp(Pickupp *pickuppArray, Vector2 playerPos, int i, int *pickuppAlive, float bulletFireRate)
{
    if (pickuppAlive[i] == 1)
    {
        if (CheckCollisionCircles(pickuppArray[i].pos, 10, playerPos, 40))
        {
            pickuppAlive[i] = 0;
            score += 200;
            if (bulletFireRate > 0.2)
            {
                bulletFireRate -= 0.05;
            }
        }
    }
    return bulletFireRate;
}

float UppdatePickup(Pickupp *pickuppArray, int *pickuppAlive, Vector2 playerPos, float bulletFireRate)
{
    Vector2 zombieVelocity = {-1, 0};
    for (int i = 0; i < 5; i++)
    {
        if (pickuppAlive[i] == 1)
        {
            pickuppArray[i].pos = Vector2Add(Vector2Scale(Vector2Scale(zombieVelocity, backgrundspeed), GetFrameTime()), pickuppArray[i].pos);
            bulletFireRate = PickUppPickupp(pickuppArray, playerPos, i, pickuppAlive, bulletFireRate);
        }
        if (pickuppArray[i].pos.x < -20)
        {
            pickuppAlive[i] = 0;
        }
    }
    return bulletFireRate;
}

void DrawPickup(Pickupp *pickuppArray, int *pickuppAlive)
{
    for (int i = 0; i < 5; i++)
    {
        if (pickuppAlive[i] == 1)
        {
            DrawCircle(pickuppArray[i].pos.x, pickuppArray[i].pos.y, 10, YELLOW);
        }
    }
}

// draws bullet
void DrawBulet(int *drawBullet, Bullet *bullet, int maxBullets)
{
    for (int i = 0; i < maxBullets; i++)
    {
        if (drawBullet[i] == 1)
        {
            switch (bullet[i].type)
            {
            case BULLET_TYPE_ENEMY_MISSALES:
                DrawCircle(bullet[i].pos.x, bullet[i].pos.y, 8, DARKPURPLE);
                break;

            default:
                DrawCircle(bullet[i].pos.x, bullet[i].pos.y, 5, RED);
                break;
            }
        }
    }
}

int CheckBuletHit(int *drawBullet, Bullet *bullet, int maxBullets, Enemy *enemyArray, int *isEnemyAlive, BulletTypeData *bulletTypeData, EnemyTypeData *enemyTypeData, Vector2 playerPos, int playerHp, Pickupp *pickuppArray, int *pickuppAlive)
{
    for (int i = 0; i < maxBullets; i++)
    {
        if (drawBullet[i] == 1)
        {
            switch (bullet[i].type)
            {
            case BULLET_TYPE_PLAYER_STANDARD:

                for (int k = 0; k < 100; k++)
                {
                    if (isEnemyAlive[k] == 1)
                    {
                        if (CheckCollisionCircles(bullet[i].pos, 5, enemyArray[k].pos, enemyTypeData[enemyArray[k].enemyType].size))
                        {
                            drawBullet[i] = 0;
                            enemyArray[k].currentHp -= bulletTypeData[BULLET_TYPE_PLAYER_STANDARD].damage;

                            if (enemyArray[k].currentHp < 1)
                            {
                                isEnemyAlive[k] = 0;
                                spawnUppgrade(enemyArray, pickuppArray, pickuppAlive, k);
                                score += 100;
                            }
                        }
                    }
                }

                break;
            case BULLET_TYPE_ENEMY:

                if (CheckCollisionCircles(bullet[i].pos, 5, playerPos, 40))
                {
                    drawBullet[i] = 0;
                    playerHp -= bulletTypeData[BULLET_TYPE_ENEMY].damage;
                }

                break;
            case BULLET_TYPE_ENEMY_MISSALES:
                if (CheckCollisionCircles(bullet[i].pos, 8, playerPos, 40))
                {
                    drawBullet[i] = 0;
                    playerHp -= bulletTypeData[BULLET_TYPE_ENEMY].damage;
                }
                break;
            default:
                break;
            }
        }
    }
    return playerHp;
}

void checkForMisal(int *drawBullet, Bullet *bullet, int maxBullets, int i)
{
    for (int k = 0; k < maxBullets; k++)
    {
        if (drawBullet[k] == 1)
        {
            if (i != k)
            {
                if (CheckCollisionCircles(bullet[i].pos, 5, bullet[k].pos, 8))
                {
                    if (bullet[k].type == BULLET_TYPE_ENEMY_MISSALES)
                    {
                        drawBullet[i] = 0;
                        drawBullet[k] = 0;
                        continue;
                    }
                }
            }
        }
    }
}

void CheckBulletToBulletHit(int *drawBullet, Bullet *bullet, int maxBullets)
{
    for (int i = 0; i < maxBullets; i++)
    {
        if (drawBullet[i] == 1)
        {
            switch (bullet[i].type)
            {
            case BULLET_TYPE_PLAYER_STANDARD:
                checkForMisal(drawBullet, bullet, maxBullets, i);
                break;

            default:
                break;
            }
        }
    }
}
// Enemy
//------------------------------------------------------------------------------

bool ControlNewEnemys(int *isEnemyAlive, int *difficultyScaleing, int maxEnemys)
{
    for (int i = 0; i < maxEnemys; i++)
    {
        if (isEnemyAlive[i] == 1)
        {
            return false;
        }
    }

    waveCount++;

    if (waveHoldeValue + 5 == waveCount)
    {
        *difficultyScaleing += 1;
        waveHoldeValue = waveCount;
    }

    return true;
}

int spawnEventEnemy(Enemy *enemyArray, EnemyTypeData *enemyTypeData, int enemyNum, int *isEnemyAlive, int difficultyScaleing, EnemyType *enemysOnLvl, int enemysOnlvlSize, int spawnSlot[], int screenWidth)
{

    int enemyNumLocal = enemyNum;
    int spawns = 1;
    int lockSpawns = 0;

    // skapar rätt mängde finder

    int enemySelected = enemysOnLvl[RandomIntRange(0, enemysOnlvlSize)];

    for (int i = 0; i < spawns; i++)
    {
        enemyArray[enemyNum + i].enemyType = enemySelected;
        isEnemyAlive[enemyNum + i] = 1;

        if (lockSpawns < 1)
        {
            int spawnNumWithScale = enemyTypeData[enemyArray[enemyNum + i].enemyType].baseWaveSize + enemyTypeData[enemyArray[enemyNum + i].enemyType].waveScaling * difficultyScaleing;

            if (spawnNumWithScale > 1)
            {
                spawns += spawnNumWithScale - 1;
                lockSpawns += spawnNumWithScale - 1;
            }
        }

        // reseta räknevärket
        enemyNumLocal++;
        if (enemyNumLocal == 100)
        {
            enemyNum = 0;
            enemyNumLocal = 0;
        }
    }

    for (int i = 0; i < spawns; i++)
    {
        // Populate data
        enemyArray[enemyNum + i].currentHp = enemyTypeData[enemyArray[i].enemyType].hp;
        enemyArray[enemyNum + i].fireRateTimer = enemyTypeData[enemyArray[i].enemyType].fireRate;

        // clean data
        enemyArray[enemyNum + i].targetPoint = Vector2Zero();
        enemyArray[enemyNum + i].step = 0;

        // Seter start regeion
        int randomSpawn = RandomIntRange(spawnSlot[0], spawnSlot[1]);

        enemyArray[enemyNum + i].pos.x = screenWidth + 50;
        enemyArray[enemyNum + i].pos.y = randomSpawn;
    }

    return enemyNumLocal;
}

Vector2 GoToPoint(Enemy *enemyArray, int *isEnemyAlive, int maxEnemys, int i, bool doMovementCloseEnemys, float avoidensModifire)
{
    Vector2 zombieVelocity;
    Vector2 enemysInRangeVelocity = {0, 0};
    bool enemyInRange = false;
    if (doMovementCloseEnemys == true)
    {
        for (int k = 0; k < maxEnemys; k++)
        {
            if (isEnemyAlive[k] == 1)
            {
                if (i != k)
                {
                    float distanceEnemy = Vector2Distance(enemyArray[i].pos, enemyArray[k].pos);
                    if (distanceEnemy < enemyTypeData[enemyArray[i].enemyType].size * avoidensModifire)
                    {
                        Vector2 enemyTempVelocity = Vector2Subtract(enemyArray[i].pos, enemyArray[k].pos);
                        enemyTempVelocity = Vector2Normalize(enemyTempVelocity);
                        enemysInRangeVelocity = Vector2Add(enemysInRangeVelocity, enemyTempVelocity);
                        enemyInRange = true;
                    }
                }
            }
        }
    }

    zombieVelocity = Vector2Subtract(enemyArray[i].targetPoint, enemyArray[i].pos);
    zombieVelocity = Vector2Normalize(zombieVelocity);

    if (enemyInRange == true)
    {
        zombieVelocity = Vector2Add(zombieVelocity, enemysInRangeVelocity);
        zombieVelocity = Vector2Normalize(zombieVelocity);
    }

    zombieVelocity = Vector2Add(Vector2Scale(Vector2Scale(zombieVelocity, enemyTypeData[enemyArray[i].enemyType].speed), GetFrameTime()), enemyArray[i].pos);
    return zombieVelocity;
}

int UpdateEnemy(Enemy *enemyArray, int maxEnemys, int *isEnemyAlive, EnemyTypeData *enemyTypeData, int *drawBullet, Bullet *bullet, int maxBullets, int bulletNum, Vector2 playerPos)
{
    for (int i = 0; i < maxEnemys; i++)
    {
        if (isEnemyAlive[i] == 1)
        {
            bool doMovementCloseEnemys = true;

            switch (enemyArray[i].enemyType)
            {
            case Enemy_Type_Zombie:
            {
                int boxSize = 50;

                float playerDistance = Vector2Distance(playerPos, enemyArray[i].pos);
                if (playerDistance > 80)
                {
                    // Gets taget
                    float targetDistance = Vector2Distance(playerPos, enemyArray[i].targetPoint);
                    if (targetDistance > 80)
                    {
                        enemyArray[i].targetPoint.x = RandomIntRange((int)(playerPos.x - boxSize), (int)(playerPos.x + boxSize));
                        enemyArray[i].targetPoint.y = RandomIntRange((int)(playerPos.y - boxSize), (int)(playerPos.y + boxSize));
                    }
                }
                else
                {
                    enemyArray[i].targetPoint = playerPos;
                    doMovementCloseEnemys = false;
                }

                enemyArray[i].pos = GoToPoint(enemyArray, isEnemyAlive, maxEnemys, i, doMovementCloseEnemys, 4.5f);
            }
            break;
            case Enemy_Type_Zombie_Gunner:
            {
                // set target
                int targetoffset = 20;

                float distancePlayerEnemy = fabs(playerPos.y - enemyArray[i].targetPoint.y);
                if (distancePlayerEnemy > 50)
                {
                    enemyArray[i].targetPoint.y = playerPos.y + RandomIntRange(-targetoffset, targetoffset);
                    enemyArray[i].targetPoint.x = RandomIntRange((g_main_window.width / 15) * 12, (g_main_window.width / 15) * 14);
                }

                // if its behind
                if (enemyArray[i].targetPoint.x < playerPos.x)
                {
                    int boxSize = 50;

                    float playerDistance = Vector2Distance(playerPos, enemyArray[i].pos);
                    if (playerDistance > 80)
                    {
                        // Gets taget
                        float targetDistance = Vector2Distance(playerPos, enemyArray[i].targetPoint);
                        if (targetDistance > 80)
                        {
                            enemyArray[i].targetPoint.x = RandomIntRange((int)(playerPos.x - boxSize), (int)(playerPos.x + boxSize));
                            enemyArray[i].targetPoint.y = RandomIntRange((int)(playerPos.y - boxSize), (int)(playerPos.y + boxSize));
                        }
                    }
                    else
                    {
                        enemyArray[i].targetPoint = playerPos;
                        doMovementCloseEnemys = false;
                    }
                }

                if (enemyArray[i].pos.x > (g_main_window.width / 15) * 14)
                {
                    enemyArray[i].targetPoint = playerPos;
                    doMovementCloseEnemys = false;
                }

                // Seperate + movement
                enemyArray[i].pos = GoToPoint(enemyArray, isEnemyAlive, maxEnemys, i, doMovementCloseEnemys, 3);

                // gun
                enemyArray[i].fireRateTimer -= GetFrameTime();

                if (enemyArray[i].fireRateTimer < 0)
                {
                    enemyArray[i].fireRateTimer = enemyTypeData[enemyArray[i].enemyType].fireRate;

                    // sjut
                    bulletNum = SetBulletOn(drawBullet, bulletNum, bullet, enemyArray[i].pos, maxBullets, BULLET_TYPE_ENEMY);
                }

                break;
            }
            case Enemy_Type_Barricade:
            {
                if (enemyArray[i].step == 0)
                {
                    if (enemyArray[i].targetPoint.x == 0)
                    {
                        if (enemyArray[i].targetPoint.y == 0)
                        {
                            enemyArray[i].targetPoint.x = RandomIntRange(0 + enemyTypeData[Enemy_Type_Barricade].size, g_main_window.width - enemyTypeData[Enemy_Type_Barricade].size);
                            enemyArray[i].targetPoint.y = RandomIntRange((g_main_window.height / 5) * 4 + enemyTypeData[Enemy_Type_Barricade].size, g_main_window.height - enemyTypeData[Enemy_Type_Barricade].size);
                        }
                    }

                    float Distance = Vector2Distance(enemyArray[i].targetPoint, enemyArray[i].pos);

                    if (Distance > 30)
                    {
                        enemyArray[i].pos = GoToPoint(enemyArray, isEnemyAlive, maxEnemys, i, true, 1.2f);
                    }
                    else
                    {
                        enemyArray[i].step = 1;
                    }
                }

                if (enemyArray[i].step == 1)
                {
                    Vector2 zombieVelocity = {-1, 0};
                    enemyArray[i].pos = Vector2Add(Vector2Scale(Vector2Scale(zombieVelocity, backgrundspeed), GetFrameTime()), enemyArray[i].pos);
                }

                if (enemyArray[i].pos.x < -100)
                {
                    isEnemyAlive[i] = 0;
                }
            }
            break;
            case Enemy_Type_Witch:
            {
                float distancePlayerEnemy = fabs(playerPos.y - enemyArray[i].targetPoint.y);
                if (distancePlayerEnemy < 200 || enemyArray[i].targetPoint.y == 0)
                {
                    if (enemyArray[i].pos.y < (g_main_window.height / 5) * 4)
                    {
                        enemyArray[i].targetPoint.y = RandomIntRange((g_main_window.height / 5) * 4, g_main_window.height - enemyTypeData[Enemy_Type_Witch].size);
                    }
                    else
                    {
                        enemyArray[i].targetPoint.y = RandomIntRange((g_main_window.height / 5 + enemyTypeData[Enemy_Type_Witch].size), (g_main_window.height / 5) * 4);
                    }

                    enemyArray[i].targetPoint.x = enemyArray[i].pos.x;
                    enemyArray[i].targetPoint.x = RandomIntRange((g_main_window.width / 15) * 10, g_main_window.width - enemyTypeData[Enemy_Type_Witch].size);
                }

                float distanceEnemyTarget = Vector2Distance(enemyArray[i].pos, enemyArray[i].targetPoint);
                if (distanceEnemyTarget > 5)
                {
                    enemyArray[i].pos = GoToPoint(enemyArray, isEnemyAlive, maxEnemys, i, true, 3);
                }

                enemyArray[i].fireRateTimer -= GetFrameTime();

                if (enemyArray[i].fireRateTimer < 0)
                {
                    enemyArray[i].fireRateTimer = enemyTypeData[enemyArray[i].enemyType].fireRate;

                    // sjut
                    bulletNum = SetBulletOn(drawBullet, bulletNum, bullet, enemyArray[i].pos, maxBullets, BULLET_TYPE_ENEMY_MISSALES);
                }
            }
            break;
            default:
                break;
            }
        }
    }

    return bulletNum;
}

void debug(Enemy *enemyArray, int maxEnemys, int *isEnemyAlive, Vector2 playerPos, EnemyTypeData *enemyTypeData)
{
    for (int i = 0; i < maxEnemys; i++)
    {
        if (isEnemyAlive[i] == 1)
        {
            switch (enemyArray[i].enemyType)
            {
            case Enemy_Type_Zombie:

                DrawLine(enemyArray[i].pos.x, enemyArray[i].pos.y, enemyArray[i].targetPoint.x, enemyArray[i].targetPoint.y, GRAY);
                DrawCircleLines(enemyArray[i].pos.x, enemyArray[i].pos.y, enemyTypeData[Enemy_Type_Zombie].size * 2.25, GRAY);
                break;
            case Enemy_Type_Zombie_Gunner:

                DrawCircleLines(enemyArray[i].pos.x, enemyArray[i].pos.y, enemyTypeData[Enemy_Type_Zombie_Gunner].size * 1.5, GRAY);
                DrawLine(enemyArray[i].pos.x, enemyArray[i].pos.y, enemyArray[i].targetPoint.x, enemyArray[i].targetPoint.y, GRAY);
                break;
            default:

                DrawLine(enemyArray[i].pos.x, enemyArray[i].pos.y, enemyArray[i].targetPoint.x, enemyArray[i].targetPoint.y, GRAY);
                break;
            }
        }
    }
}

void DrawEnemy(Enemy *enemyArray, int maxEnemys, int *isEnemyAlive, EnemyTypeData *enemyTypeData)
{
    for (int i = 0; i < maxEnemys; i++)
    {
        if (isEnemyAlive[i] == 1)
        {
            switch (enemyArray[i].enemyType)
            {
            case Enemy_Type_Zombie:
                DrawCircle(enemyArray[i].pos.x, enemyArray[i].pos.y, enemyTypeData[enemyArray[i].enemyType].size, BLUE);
                break;
            case Enemy_Type_Zombie_Gunner:
                DrawCircle(enemyArray[i].pos.x, enemyArray[i].pos.y, enemyTypeData[enemyArray[i].enemyType].size, YELLOW);
                break;
            case Enemy_Type_Barricade:
                DrawCircle(enemyArray[i].pos.x, enemyArray[i].pos.y, enemyTypeData[enemyArray[i].enemyType].size, BROWN);
                break;
            case Enemy_Type_Witch:
                DrawCircle(enemyArray[i].pos.x, enemyArray[i].pos.y, enemyTypeData[enemyArray[i].enemyType].size, PURPLE);
                break;
            default:
                break;
            }
        }
    }
}

int playerCollideEnemy(Enemy *enemyArray, int *isEnemyAlive, EnemyTypeData *enemyTypeData, Vector2 playerPos, int playerHp)
{
    float size = 0;

    for (int i = 0; i < 100; i++)
    {
        if (isEnemyAlive[i] == 1)
        {
            float distence = Vector2Distance(playerPos, enemyArray[i].pos);
            if (distence < 40 + enemyTypeData[i].size)
            {
                isEnemyAlive[i] = 0;
                playerHp--;
            }
        }
    }
    return playerHp;
}

int main()
{
    // setup
    //------------------------------------------------------------------------------
#pragma region setup
    // SetTargetFPS(120);

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    InitWindow(screenWidth, screenHeight, "Hello World");
    ToggleFullscreen();

    g_main_window.width = GetRenderWidth();
    g_main_window.height = GetRenderHeight();

    screenWidth = GetRenderWidth();
    screenHeight = GetRenderHeight();
    printf("width: %d\theight: %d\n", screenWidth, screenHeight);
#pragma endregion

    // variables
    //------------------------------------------------------------------------------
#pragma region Varibales
    // GameState
    GameState gameState;
    gameState = GameState_Start;

    // start
    bool angleOffsetDir = false;
    double circleOffset = 0;
    double circleOffsetBase = 30;
    double spinOffset = 0;

    float spinSpeed = 0.08;

    float lerpTime = 0;
    bool lerpTimeBool = true;

    bool startAnimasionTimer = false;
    float startTimer = 0;

    // Player
    Player player;

    player.playerPos.x = 800;
    player.playerPos.y = 800;

    player.velocity.x = 0;
    player.velocity.y = 0;

    player.speed = 0;
    player.acceleration = 6;
    player.maxSpeed = 0.8;
    player.stopSpeed = 7;
    player.hp = 5;

    // Pickupp
    Pickupp pickuppArray[5];
    int pickuppAlive[5];

    for (char i = 0; i < 5; i++)
    {
        pickuppAlive[i] = 0;
    }

    // random
    srand(time(NULL)); // Initialization, should only be called once.

    // enemy
    bool testSpawnEnemy = 0;
    Enemy enemyArray[100];
    int isEnemyAlive[100];

    for (int i = 0; i < 100; i++)
    {
        isEnemyAlive[i] = 0;
    }

    int enemyNum = 0;
    int difficultyScaleing = 0;

    EnemyType allEnemysIndex;

    // bullets
    int maxBullets = 500;

    Bullet bullet[maxBullets];
    int drawBullet[maxBullets];
    int bulletNum = 0;

    float bulletFireRate = 0.4f;
    float bulletTimerSet = bulletFireRate;

    for (int i = 0; i < maxBullets; i++)
    {
        drawBullet[i] = 0;
    }

#pragma endregion Varibales

    // make perling noise
    //------------------------------------------------------------------------------
#pragma region Noise
    int countRandomValue = 121;
    float randomValue1_0[countRandomValue];

    for (int i = 0; i < countRandomValue; i++)
    {
        double num = (double)rand() / (double)RAND_MAX;
        randomValue1_0[i] = num;
    }
#pragma endregion Noise
    // Game loop
    //------------------------------------------------------------------------------

    while (!WindowShouldClose())
    {
        switch (gameState)
        {
        case GameState_NULL:
        {
            printf("%s \n", "Someting went wrong!!");
            return -1;
        }
        break;

        case GameState_Start:
        {
            // uppdate
            //------------------------------------------------------------------------------

            // ut in med lijerna
            if (lerpTime > 1)
            {
                lerpTimeBool = false;
            }
            if (lerpTime < 0)
            {
                lerpTimeBool = true;
            }

            if (lerpTimeBool == true)
            {
                lerpTime += 1 * GetFrameTime();
            }
            else
            {
                lerpTime -= 1 * GetFrameTime();
            }

            // spin speed
            spinOffset += spinSpeed * GetFrameTime();

            // starta spelet logik

            if (IsKeyPressed(KEY_SPACE))
            {
                startAnimasionTimer = true;
            }

            if (startAnimasionTimer == true)
            {
                spinSpeed += pow(spinSpeed, 0.03) * GetFrameTime();
                circleOffsetBase += circleOffsetBase * 0.0005;
                startTimer += 0.2 * GetFrameTime();
            }

            // draw
            //------------------------------------------------------------------------------
            BeginDrawing();

            if (startTimer > 1)
            {
                gameState = GameState_Level_1;
            }
            else
            {
                Color grayscale = ColorAlpha(WHITE, startTimer);
                DrawRectangle(0, 0, screenWidth, screenHeight, grayscale);
            }

            // draw
            ClearBackground((Color){0, 0, 0, 255});
            DrawCircleLines(screenWidth / 2, screenHeight / 2, 165, WHITE);
            DrawCircleLines(screenWidth / 2, screenHeight / 2, 160, WHITE);

            for (int i = 0; i < 120; i++)
            {
                double angle2 = ((i * 3) * (M_PI / 180.0) + spinOffset);

                circleOffset = lerp(randomValue1_0[i], randomValue1_0[i + 1], lerpTime);
                circleOffset *= circleOffsetBase;

                float thicknes = 5;

                Vector2 startPos;
                startPos.x = (screenWidth / 2) + (cos(angle2) * 180);
                startPos.y = (screenHeight / 2) - (sin(angle2) * 180);

                Vector2 endPos;
                endPos.x = (screenWidth / 2) + (cos(angle2) * (220 + circleOffset));
                endPos.y = (screenHeight / 2) - (sin(angle2) * (220 + circleOffset));

                DrawLineBezier(startPos, endPos, thicknes, WHITE);
            }
            EndDrawing();
        }
        break;

        case GameState_Level_1:
        {
            // movement
            //------------------------------------------------------------------------------
            player.directionVector = GetDirctionInput(player.directionVector);
            player = CalculatMovement(player);
            player.playerPos = CheckOutOfBoundes(screenWidth, screenHeight, player.playerPos, player.velocity, 40);

            // update
            //------------------------------------------------------------------------------
            // enemy
            testSpawnEnemy = ControlNewEnemys(isEnemyAlive, &difficultyScaleing, 100);

            if (testSpawnEnemy == true)
            {
                int spawnSlot[2] = {screenHeight / 5, screenHeight};

                if (difficultyScaleing < 1)
                {
                    enemyNum = spawnEventEnemy(enemyArray, enemyTypeData, enemyNum, isEnemyAlive, difficultyScaleing, enemyTypeLevel1, 5, spawnSlot, screenWidth);
                }
                else
                {
                    enemyNum = spawnEventEnemy(enemyArray, enemyTypeData, enemyNum, isEnemyAlive, difficultyScaleing, enemyTypeLevel2, 7, spawnSlot, screenWidth);
                }
                testSpawnEnemy = false;
            }
            bulletNum = UpdateEnemy(enemyArray, 100, isEnemyAlive, enemyTypeData, drawBullet, bullet, maxBullets, bulletNum, player.playerPos);
            player.hp = playerCollideEnemy(enemyArray, isEnemyAlive, enemyTypeData, player.playerPos, player.hp);

            // bullet

            if (IsKeyDown(KEY_RIGHT))
            {
                if (bulletTimerSet < 0)
                {
                    bulletTimerSet = bulletFireRate;
                    bulletNum = SetBulletOn(drawBullet, bulletNum, bullet, player.playerPos, maxBullets, BULLET_TYPE_PLAYER_STANDARD);
                }
            }
            bulletTimerSet -= GetFrameTime();

            UppdateBulet(drawBullet, bullet, maxBullets, screenWidth, bulletTypeData, player.playerPos);
            player.hp = CheckBuletHit(drawBullet, bullet, maxBullets, enemyArray, isEnemyAlive, bulletTypeData, enemyTypeData, player.playerPos, player.hp, pickuppArray, pickuppAlive);
            // check if player is dead
            gameState = isPlayerDead(player.hp, gameState);

            // Pickupp
            bulletFireRate = UppdatePickup(pickuppArray, pickuppAlive, player.playerPos, bulletFireRate);

            // draw
            //------------------------------------------------------------------------------
            BeginDrawing();
            ClearBackground((Color){0, 0, 0, 255});

            // draw backgrund
            DrawRectangle(0, 0, screenWidth, screenHeight / 5, WHITE);

            // player
            AnimatePlayer(player.playerPos);
            drawPlayerHp(player.hp, screenHeight);

            // bullet
            DrawBulet(drawBullet, bullet, maxBullets);
            CheckBulletToBulletHit(drawBullet, bullet, maxBullets);

            // enemy
            DrawEnemy(enemyArray, 100, isEnemyAlive, enemyTypeData);

            // Pickupp
            DrawPickup(pickuppArray, pickuppAlive);

            // debug
            // DrawFPS(25, 25);
            // DebugMouse();
            // debug(enemyArray, 100, isEnemyAlive, player.playerPos, enemyTypeData);

            // score
            DrawText(TextFormat("Score %d", score), screenWidth / 2, screenHeight / 20, 30, RED);

            EndDrawing();
        }
        break;
        case GameState_Game_Over:
        {
            BeginDrawing();
            ClearBackground(BLACK);

            DrawText("Game over", screenWidth / 2, screenHeight / 2, 100, RED);
            DrawText(TextFormat("Score %d", score), screenWidth / 2, screenHeight / 2 - 200, 100, RED);

            EndDrawing();
        }
        break;
        default:
            printf("något gick fel");
            break;
        }
    }
    return 0;
}