#include <stdio.h>
#include <stdlib.h>

#include "raylib.h"
#include "raymath.h"

#define TILE_SIZE (short)32
#define SCREEN_X_TILES (short)32
#define SCREEN_Y_TILES (short)24
#define TILE_TYPES (short)16

#define ROOM_COUNT (short)32
#define ROOM_BUFFER (short)5
#define CONNECTION_COUNT (short)4

#define MAP_WIDTH (short)150
#define MAP_HEIGHT (short)150

typedef struct Room Room;

struct Room
{
    short x;
    short y;
    short w;
    short h;
    short connections[CONNECTION_COUNT];
    bool alive;
};

typedef struct
{
    short index;
    float distance;
} IndexAndDist;

// Bubble sort från https://www.geeksforgeeks.org/bubble-sort/
void swap(IndexAndDist *xp, IndexAndDist *yp)
{
    IndexAndDist temp = *xp;
    *xp = *yp;
    *yp = temp;
}

// An optimized version of Bubble Sort
void BubbleSort(IndexAndDist arr[], int n)
{
    int i, j;
    bool swapped;
    for (i = 0; i < n - 1; i++)
    {
        swapped = false;
        for (j = 0; j < n - i - 1; j++)
        {
            if (arr[j].distance > arr[j + 1].distance)
            {
                swap(&arr[j], &arr[j + 1]);
                swapped = true;
            }
        }

        // If no two elements were swapped by inner loop,
        // then break
        if (swapped == false)
            break;
    }
} // End of copied code

void AddRoom(Room *arr, Room newRoom)
{
    for (short i = 0; i < ROOM_COUNT; i++)
    {
        if (arr[i].alive)
            continue;

        arr[i] = newRoom;
        break;
    }
}

Vector2 GetRoomCenter(Room room)
{
    return (Vector2){
        room.x + room.w / 2,
        room.y + room.h / 2};
}

short GetNthClosestRoomIndex(Room rooms[], short from, short n)
{
    // Make array of rooms
    IndexAndDist arr[ROOM_COUNT];

    short i = 0;
    while (i < ROOM_COUNT)
    {
        arr[i] = (IndexAndDist){i, Vector2DistanceSqr(GetRoomCenter(rooms[from]), GetRoomCenter(rooms[i]))};
        i++;
    }

    // Sort array
    BubbleSort(arr, ROOM_COUNT);

    return arr[n].index;
}

void AddRoomConnection(Room rooms[], short i1)
{
    short i2;
    short n = 1;
    short ai1 = -1;
    short ai2 = -1;
    bool created = false;

    while (!created)
    {
        created = true;
        // printf("%hi\n", n);
        i2 = GetNthClosestRoomIndex(rooms, i1, n);
        short takenSpots1 = 0;
        short takenSpots2 = 0;

        // Check if connection already exists
        for (short i = 0; i < CONNECTION_COUNT; i++)
        {
            if (rooms[i1].connections[i] == i2)
            {
                // puts("Connection already exists");
                created = false;
                n++;
                break;
            }
        }

        // Check if connection could be added
        ai1 = -1;
        for (short i = 0; i < CONNECTION_COUNT; i++)
        {
            if (rooms[i1].connections[i] != -1)
            {
                takenSpots1++;
                continue;
            }

            ai1 = i;
        }

        ai2 = -1;
        for (short i = 0; i < CONNECTION_COUNT; i++)
        {
            if (rooms[i2].connections[i] != -1)
            {
                takenSpots2++;
                continue;
            }

            ai2 = i;
        }

        if (ai1 == -1 || ai2 == -1)
        {
            // puts("Connection does not fit");
            created = false;
            n++;
        }

        if (takenSpots1 > GetRandomValue(0, CONNECTION_COUNT) || takenSpots2 > GetRandomValue(0, CONNECTION_COUNT))
        {
            created = false;
            break;
        }

        if (n >= ROOM_COUNT)
        {
            created = false;
            break;
        }
    }

    if (created)
    {
        // Add the connection
        rooms[i1].connections[ai1] = i2;
        rooms[i2].connections[ai2] = i1;
    }
}

bool RoomCollision(Room r1, Room r2)
{
    if (
        ((r1.x + ROOM_BUFFER > r2.x && r1.x < r2.x + r2.w + ROOM_BUFFER) ||
         (r1.x < r2.x + ROOM_BUFFER && r1.x + r1.w + ROOM_BUFFER > r2.x)) &&
        ((r1.y + ROOM_BUFFER > r2.y && r1.y < r2.y + r2.h + ROOM_BUFFER) ||
         (r1.y < r2.y + ROOM_BUFFER && r1.y + r1.h + ROOM_BUFFER > r2.y)))
        return true;

    if (r1.x < ROOM_BUFFER || r1.y < ROOM_BUFFER || r1.x + r1.w > MAP_WIDTH - ROOM_BUFFER || r1.y + r1.h > MAP_HEIGHT - ROOM_BUFFER)
        return true;

    return false;
}

void Search(Room rooms[], bool visited[], short from)
{
    // printf("%hi\n", from);
    visited[from] = true;

    for (short i = 0; i < CONNECTION_COUNT; i++)
    {
        short test = rooms[from].connections[i];

        if (test != -1)
        {
            if (!visited[test])
            {
                // puts("^searched that one^");
                Search(rooms, visited, test);
            }

            // puts("^Denied because visited^");
        }
    }
}

bool IsGraphConnected(Room rooms[])
{
    bool visited[ROOM_COUNT];
    for (short i = 0; i < ROOM_COUNT; i++)
        visited[i] = false;

    Search(rooms, visited, 1);

    for (short i = 0; i < ROOM_COUNT; i++)
    {
        if (!visited[i])
            return false;
    }

    return true;
}

void DrawRoomToMap(short map[], Room room)
{
    for (short y = room.y; y < room.y + room.h; y++)
    {
        for (short x = room.x; x < room.x + room.w; x++)
        {
            map[x + y * MAP_WIDTH] = TILE_TYPES;
        }
    }
}

short Sign(short n)
{
    if (n > 0)
        return 1;
    if (n < 0)
        return -1;

    return 0;
}

void DrawCorridorToMap(short map[], Room r1, Room r2)
{
    Vector2 p1 = GetRoomCenter(r1);
    Vector2 p2 = GetRoomCenter(r2);

    short px = p1.x;
    short py = p1.y;

    while (!CheckCollisionPointRec((Vector2){px, py}, (Rectangle){r2.x, r2.y, r2.w, r2.h}))
    {
        short hDiff = px - p2.x;
        short vDiff = py - p2.y;

        if (abs(hDiff) > abs(vDiff))
        {
            for (short n = 0; (float)n < abs(hDiff) / 2; n++)
            {
                px -= Sign(hDiff);
                map[px + py * MAP_WIDTH] = TILE_TYPES;
            }
            for (short n = 0; n < abs(vDiff); n++)
            {
                py -= Sign(vDiff);
                map[px + py * MAP_WIDTH] = TILE_TYPES;
            }
            for (short n = 0; (float)n < abs(hDiff) / 2; n++)
            {
                px -= Sign(hDiff);
                map[px + py * MAP_WIDTH] = TILE_TYPES;
            }
        }
        else
        {
            for (short n = 0; (float)n < abs(vDiff) / 2; n++)
            {
                py -= Sign(vDiff);
                map[px + py * MAP_WIDTH] = TILE_TYPES;
            }
            for (short n = 0; n < abs(hDiff); n++)
            {
                px -= Sign(hDiff);
                map[px + py * MAP_WIDTH] = TILE_TYPES;
            }
            for (short n = 0; (float)n < abs(vDiff) / 2; n++)
            {
                py -= Sign(vDiff);
                map[px + py * MAP_WIDTH] = TILE_TYPES;
            }
        }
    }
}

void GridCollisionH(Vector2 *playerPos, short map[MAP_WIDTH * MAP_HEIGHT], short px, short py)
{
    if (map[px + py * MAP_WIDTH] >= TILE_TYPES)
        return;

    if (
        px * TILE_SIZE + TILE_SIZE > playerPos->x - 12 &&
        px * TILE_SIZE + TILE_SIZE < playerPos->x - 12 + TILE_SIZE / 2 &&
        playerPos->y + 12 > py * TILE_SIZE &&
        playerPos->y - 12 < py * TILE_SIZE + TILE_SIZE)
        playerPos->x = px * TILE_SIZE + TILE_SIZE + 12;

    if (
        px * TILE_SIZE < playerPos->x + 12 &&
        px * TILE_SIZE > playerPos->x + 12 - TILE_SIZE / 2 &&
        playerPos->y + 12 > py * TILE_SIZE &&
        playerPos->y - 12 < py * TILE_SIZE + TILE_SIZE)
        playerPos->x = px * TILE_SIZE - 12;
}

void GridCollisionV(Vector2 *playerPos, short map[MAP_WIDTH * MAP_HEIGHT], short px, short py)
{
    if (map[px + py * MAP_WIDTH] >= TILE_TYPES)
        return;
    if (
        py * TILE_SIZE + TILE_SIZE > playerPos->y - 12 &&
        py * TILE_SIZE + TILE_SIZE < playerPos->y - 12 + TILE_SIZE / 2 &&
        playerPos->x + 12 > px * TILE_SIZE &&
        playerPos->x - 12 < px * TILE_SIZE + TILE_SIZE)
        playerPos->y = py * TILE_SIZE + TILE_SIZE + 12;

    if (
        py * TILE_SIZE < playerPos->y + 12 &&
        py * TILE_SIZE > playerPos->y + 12 - TILE_SIZE / 2 &&
        playerPos->x + 12 > px * TILE_SIZE &&
        playerPos->x - 12 < px * TILE_SIZE + TILE_SIZE)
        playerPos->y = py * TILE_SIZE - 12;
}

int main()
{
    const short screenWidth = SCREEN_X_TILES * TILE_SIZE;
    const short screenHeight = SCREEN_Y_TILES * TILE_SIZE;

    InitWindow(screenWidth, screenHeight, "TerrainGen");

    Vector2 playerPos = (Vector2){10 * TILE_SIZE, 10 * TILE_SIZE};
    float playerSpeed = 150;
    float playerRadius = 12;

    Camera2D camera;
    camera.offset = Vector2Zero();
    camera.target = Vector2Zero();
    camera.rotation = 0;
    camera.zoom = 1;

    short map[MAP_WIDTH * MAP_WIDTH];
    for (short y = 0; y < MAP_WIDTH; y++)
    {
        for (short x = 0; x < MAP_WIDTH; x++)
        {
            map[x + y * MAP_WIDTH] = 0;
        }
    }

    Room rooms[ROOM_COUNT];
    for (short i = 0; i < ROOM_COUNT; i++)
        rooms[i].alive = false;

    SetRandomSeed(177013);

    for (short a = 0; a < ROOM_COUNT; a++)
    {
        for (short n = 1; n <= 200; n++)
        {
            Room newRoom = (Room){
                GetRandomValue(0, MAP_WIDTH - 16),
                GetRandomValue(0, MAP_WIDTH - 16),
                GetRandomValue(6, 16),
                GetRandomValue(6, 16),
                {-1,
                 -1,
                 -1,
                 -1},
                true};

            bool roomFits = true;

            for (short i = 0; i < ROOM_COUNT; i++)
            {
                if (!rooms[i].alive)
                    continue;

                if (RoomCollision(newRoom, rooms[i]))
                {
                    roomFits = false;
                    break;
                }
            }

            if (roomFits)
            {
                AddRoom(rooms, newRoom);
                // printf("Added room after %hi tries\n", n);
                break;
            }

            if (n == (short)100)
            {
                puts("epic fail");
            }
        }
    }

    while (!IsGraphConnected(rooms))
    {
        short randomIndex = GetRandomValue(0, ROOM_COUNT);
        AddRoomConnection(rooms, randomIndex);
    }

    for (short i = 0; i < ROOM_COUNT; i++)
    {
        DrawRoomToMap(map, rooms[i]);
    }

    bool connected[ROOM_COUNT * ROOM_COUNT];
    for (short y = 0; y < ROOM_COUNT; y++)
    {
        for (short x = 0; x < ROOM_COUNT; x++)
        {
            connected[x + y * ROOM_COUNT] = false;
        }
    }

    for (short i = 0; i < ROOM_COUNT; i++)
    {
        for (short n = 0; n < CONNECTION_COUNT; n++)
        {
            if (rooms[i].connections[n] != -1)
            {
                short i1 = i;
                short i2 = rooms[i].connections[n];

                if (connected[i1 + i2 * ROOM_COUNT] || connected[i2 + i1 * ROOM_COUNT])
                    continue;

                DrawCorridorToMap(map, rooms[i1], rooms[i2]);
                connected[i1 + i2 * ROOM_COUNT] = true;
            }
        }
    }

    for (short y = 0; y < MAP_WIDTH; y++)
    {
        for (short x = 0; x < MAP_WIDTH; x++)
        {
            short *sp = &(map[x + y * MAP_WIDTH]);

            short n = 0;
            if (map[(x) + (y - 1) * MAP_WIDTH] < TILE_TYPES)
                n += 1;
            if (map[(x - 1) + (y)*MAP_WIDTH] < TILE_TYPES)
                n += 2;
            if (map[(x + 1) + (y)*MAP_WIDTH] < TILE_TYPES)
                n += 4;
            if (map[(x) + (y + 1) * MAP_WIDTH] < TILE_TYPES)
                n += 8;

            if (*sp < TILE_TYPES)
            {
                *sp -= n;
                *sp += TILE_TYPES - 1;
            }
            else
            {
                *sp += n;
            }
        }
    }

    Texture2D lightTexture = LoadTexture("light.png");
    Texture2D darkTexture = LoadTexture("dark.png");

    while (!WindowShouldClose())
    {
        Vector2 playerMovement = Vector2Zero();
        
        if (IsKeyDown(KEY_RIGHT))
            playerMovement.x ++;
        if (IsKeyDown(KEY_LEFT))
            playerMovement.x --;
        if (IsKeyDown(KEY_DOWN))
            playerMovement.y ++;
        if (IsKeyDown(KEY_UP))
            playerMovement.y --;

        playerMovement = Vector2Normalize(playerMovement);
        playerMovement = Vector2Scale(playerMovement, playerSpeed * GetFrameTime());

        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode2D(camera);

        for (short y = 0; y < MAP_WIDTH; y++)
        {
            for (short x = 0; x < MAP_WIDTH; x++)
            {
                short n = map[x + y * MAP_WIDTH];

                Texture2D *texturePtr = &darkTexture;

                if (n >= TILE_TYPES)
                    texturePtr = &lightTexture;

                DrawTexturePro(
                    *texturePtr,
                    (Rectangle){
                        16 * (n % TILE_TYPES),
                        0,
                        16,
                        16},
                    (Rectangle){
                        x * TILE_SIZE,
                        y * TILE_SIZE,
                        TILE_SIZE,
                        TILE_SIZE},
                    Vector2Zero(),
                    0,
                    WHITE);
            }
        }

        short px = (playerPos.x - TILE_SIZE / 2) / TILE_SIZE;
        short py = (playerPos.y - TILE_SIZE / 2) / TILE_SIZE;

        playerPos.x += playerMovement.x;
        if (!IsKeyDown(KEY_SPACE))
        {
            GridCollisionH(&playerPos, map, px, py);
            GridCollisionH(&playerPos, map, px + 1, py);
            GridCollisionH(&playerPos, map, px, py + 1);
            GridCollisionH(&playerPos, map, px + 1, py + 1);
        }

        playerPos.y += playerMovement.y;
        if (!IsKeyDown(KEY_SPACE))
        {
            GridCollisionV(&playerPos, map, px, py);
            GridCollisionV(&playerPos, map, px + 1, py);
            GridCollisionV(&playerPos, map, px, py + 1);
            GridCollisionV(&playerPos, map, px + 1, py + 1);
        }

        camera.target.x = playerPos.x - screenWidth / 2;
        camera.target.y = playerPos.y - screenHeight / 2;

        //DrawRectangleLines(px * TILE_SIZE, py * TILE_SIZE, 2 * TILE_SIZE, 2 * TILE_SIZE, RED);

        DrawCircleV(playerPos, playerRadius, RED);

        EndMode2D();

        for (short i = 0; i < ROOM_COUNT; i++)
        {
            if (!rooms[i].alive)
                continue;

            DrawRectangle(
                rooms[i].x,
                rooms[i].y,
                rooms[i].w,
                rooms[i].h,
                WHITE);

            for (short n = 0; n < CONNECTION_COUNT; n++)
            {
                if (rooms[i].connections[n] == -1)
                    continue;

                DrawLineV(GetRoomCenter(rooms[i]), GetRoomCenter(rooms[rooms[i].connections[n]]), RED);
            }
        }

        DrawRectangleLines(
            camera.target.x / TILE_SIZE,
            camera.target.y / TILE_SIZE,
            SCREEN_X_TILES,
            SCREEN_Y_TILES,
            RED);

        DrawText(TextFormat("FPS: %i", GetFPS()), 5, 5, 10, WHITE);
        EndDrawing();
    }

    CloseWindow();

    return 0;
}