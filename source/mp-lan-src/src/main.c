#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER

#include <winsock2.h>
#include <ws2tcpip.h>

#undef DrawText

#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

#define NET_PORT 25565
#define PACKET_POS 1
#define PACKET_BLOCK_ADD 2
#define PACKET_BLOCK_REM 3
#define PACKET_ENV_UPDATE 4
#define PACKET_HELLO 5

#define MAX_BLOCKS 2000
#define MAX_PARTICLES 500
#define PLAYER_SPEED 300.0f
#define PLAYER_RUN_SPEED 500.0f
#define JUMP_FORCE 550.0f
#define GRAVITY 1000.0f
#define MAX_FALL_SPEED 800.0f
#define BLOCK_SIZE 40
#define MAX_PLAYERS 4

#define RL_STANDALONE

typedef struct {
    unsigned char type;
    int playerId;
    float x;
    float y;
    int data1;
    int data2;
} NetPacket;

SOCKET sock;
struct sockaddr_in serverAddr;
struct sockaddr_in clients[MAX_PLAYERS];
bool clientConnected[MAX_PLAYERS];
bool isServer = false;
bool isConnected = false;
char targetIP[32] = "127.0.0.1";

typedef enum { SHAPE_SQUARE, SHAPE_RECT, SHAPE_TRIANGLE, SHAPE_CIRCLE, SHAPE_RHOMBUS } BlockShape;
typedef enum { WEATHER_NONE, WEATHER_RAIN, WEATHER_SNOW } WeatherType;
typedef enum { STATE_MENU, STATE_GAME } GameState;

typedef struct {
    Rectangle rect;
    int active;
    Color color;
    BlockShape shape;
} Block;

typedef struct {
    Vector2 position;
    float speed;
    int active;
} Particle;

typedef struct {
    Vector2 position;
    Vector2 velocity;
    bool grounded;
    int colorIndex;
    bool active;
} Player;

Block blocks[MAX_BLOCKS];
Particle particles[MAX_PARTICLES];
Color blockColors[5]; 
Color playerColors[6]; 
Player players[MAX_PLAYERS]; 
int myId = 0;

const char* shapeNames[] = { "CUADRADO", "RECTANGULO", "TRIANGULO", "CIRCULO", "ROMBO" };

void InitNetwork(bool server, const char* ip) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    u_long mode = 1;
    ioctlsocket(sock, FIONBIO, &mode);
    for(int i=0; i<MAX_PLAYERS; i++) clientConnected[i] = false;
    if (server) {
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(NET_PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
        isServer = true;
        isConnected = true;
    } else {
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(NET_PORT);
        inet_pton(AF_INET, ip, &serverAddr.sin_addr);
        isServer = false;
        isConnected = true;
    }
}

void SendPacket(NetPacket p) {
    if (isServer) {
        for(int i=0; i<MAX_PLAYERS; i++) {
            if (clientConnected[i] && i != myId) {
                sendto(sock, (char*)&p, sizeof(p), 0, (struct sockaddr*)&clients[i], sizeof(clients[i]));
            }
        }
    } else {
        sendto(sock, (char*)&p, sizeof(p), 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    }
}

void SendPacketTo(NetPacket p, struct sockaddr_in target) {
    sendto(sock, (char*)&p, sizeof(p), 0, (struct sockaddr*)&target, sizeof(target));
}

void ResetPlayer(Player *p, Block startPlatform) {
    p->position = (Vector2){ startPlatform.rect.x + 50 + (myId * 45), startPlatform.rect.y - 100 };
    p->velocity = (Vector2){ 0, 0 };
    p->active = true;
}

void InitGame() {
    for (int i = 1; i < MAX_BLOCKS; i++) blocks[i].active = 0;
    blocks[0].active = 1;
    blocks[0].rect = (Rectangle){ -200, 300, 800, 40 };
    blocks[0].color = GRAY;
    blocks[0].shape = SHAPE_RECT;
    for(int i=0; i<MAX_PLAYERS; i++) {
        players[i].active = (i == myId);
        players[i].colorIndex = i;
        ResetPlayer(&players[i], blocks[0]);
    }
}

void InitParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = 0;
}

void UpdateWeather(WeatherType weather, Camera2D cam, int screenW, int screenH) {
    if (weather == WEATHER_NONE) return;
    float dt = GetFrameTime();
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i].position.x = (float)GetRandomValue((int)(cam.target.x - screenW), (int)(cam.target.x + screenW));
            particles[i].position.y = cam.target.y - screenH/2.0f - GetRandomValue(0, 200);
            particles[i].speed = (float)((weather == WEATHER_RAIN) ? GetRandomValue(400, 800) : GetRandomValue(50, 150));
            particles[i].active = 1;
        }
        particles[i].position.y += particles[i].speed * dt;
        if (weather == WEATHER_SNOW) particles[i].position.x += GetRandomValue(-1, 1); 
        if (particles[i].position.y > cam.target.y + screenH) particles[i].active = 0;
    }
}

void DrawWeather(WeatherType weather) {
    if (weather == WEATHER_NONE) return;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            if (weather == WEATHER_RAIN) DrawLineV(particles[i].position, (Vector2){particles[i].position.x, particles[i].position.y + 10}, Fade(BLUE, 0.7f));
            else DrawCircleV(particles[i].position, 2, Fade(WHITE, 0.8f));
        }
    }
}

void DrawBlockShape(Block b) {
    switch (b.shape) {
        case SHAPE_SQUARE: case SHAPE_RECT: DrawRectangleRec(b.rect, b.color); break;
        case SHAPE_TRIANGLE: 
            DrawTriangle((Vector2){b.rect.x + b.rect.width/2, b.rect.y}, (Vector2){b.rect.x, b.rect.y + b.rect.height}, (Vector2){b.rect.x + b.rect.width, b.rect.y + b.rect.height}, b.color);
            break;
        case SHAPE_CIRCLE: DrawCircle((int)(b.rect.x + b.rect.width/2), (int)(b.rect.y + b.rect.height/2), (float)b.rect.width/2, b.color); break;
        case SHAPE_RHOMBUS:
             DrawTriangle((Vector2){b.rect.x + b.rect.width/2, b.rect.y}, (Vector2){b.rect.x, b.rect.y + b.rect.height/2}, (Vector2){b.rect.x + b.rect.width, b.rect.y + b.rect.height/2}, b.color);
             DrawTriangle((Vector2){b.rect.x, b.rect.y + b.rect.height/2}, (Vector2){b.rect.x + b.rect.width/2, b.rect.y + b.rect.height}, (Vector2){b.rect.x + b.rect.width, b.rect.y + b.rect.height/2}, b.color);
             break;
    }
}

void DrawPlayerRender(Player p, int id, Color color) {
    if (!p.active) return;
    DrawRectangleV(p.position, (Vector2){40, 40}, color);
    DrawRectangle((int)p.position.x + 10, (int)p.position.y + 12, 6, 6, BLACK);  
    DrawRectangle((int)p.position.x + 24, (int)p.position.y + 12, 6, 6, BLACK); 
    DrawRectangle((int)p.position.x + 10, (int)p.position.y + 28, 20, 4, BLACK); 
    const char* label = TextFormat("Player [ %d ]", id);
    int textW = MeasureText(label, 10);
    DrawText(label, (int)(p.position.x + 20 - textW/2), (int)(p.position.y - 15), 10, WHITE);
}

void AddBlock(float x, float y, int w, int h, int colorIdx, int shapeIdx) {
    Rectangle target = {x, y, 1, 1};
    for(int i=1; i<MAX_BLOCKS; i++) {
        if (blocks[i].active && CheckCollisionRecs(target, blocks[i].rect)) return;
    }
    for (int i = 1; i < MAX_BLOCKS; i++) {
        if (!blocks[i].active) {
            blocks[i].active = 1;
            blocks[i].rect = (Rectangle){ x, y, (float)w, (float)h };
            blocks[i].color = blockColors[colorIdx];
            blocks[i].shape = (BlockShape)shapeIdx;
            break;
        }
    }
}

void RemoveBlock(float x, float y) {
    Rectangle target = {x, y, 1, 1};
    for (int i = 1; i < MAX_BLOCKS; i++) { 
        if (blocks[i].active && CheckCollisionRecs(target, blocks[i].rect)) {
            blocks[i].active = 0;
        }
    }
}

int main(void) {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Platform LAN 4 Players");
    SetTargetFPS(60); 
    blockColors[0] = BLUE; blockColors[1] = RED; blockColors[2] = GREEN;
    blockColors[3] = YELLOW; blockColors[4] = PINK;
    playerColors[0] = GRAY; playerColors[1] = ORANGE; playerColors[2] = VIOLET;
    playerColors[3] = GOLD; playerColors[4] = LIME; playerColors[5] = BLUE;    
    InitGame();
    InitParticles();
    GameState gameState = STATE_MENU;
    Camera2D camera = { 0 };
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.zoom = 1.0f;
    int selectedColorIndex = 0;
    int selectedShapeIndex = 0;
    float previewTimer = 0.0f;
    bool isNight = false;
    WeatherType currentWeather = WEATHER_NONE;
    int ipLetterCount = 0;

    while (!WindowShouldClose()) {
        if (gameState == STATE_GAME) {
            NetPacket packet;
            struct sockaddr_in sender;
            int senderLen = sizeof(sender);
            while (recvfrom(sock, (char*)&packet, sizeof(packet), 0, (struct sockaddr*)&sender, &senderLen) > 0) {
                if (packet.type == PACKET_HELLO && isServer) {
                    clients[packet.playerId] = sender;
                    clientConnected[packet.playerId] = true;
                    NetPacket pEnv = {PACKET_ENV_UPDATE, myId, 0, 0, (int)currentWeather, (int)isNight};
                    SendPacketTo(pEnv, sender);
                    for(int i=1; i<MAX_BLOCKS; i++) {
                        if (blocks[i].active) {
                            int cIdx = 0;
                            for(int c=0; c<5; c++) if(ColorToInt(blocks[i].color) == ColorToInt(blockColors[c])) cIdx = c;
                            NetPacket pSync = {PACKET_BLOCK_ADD, myId, blocks[i].rect.x, blocks[i].rect.y, cIdx, blocks[i].shape};
                            SendPacketTo(pSync, sender);
                        }
                    }
                } else if (isServer && packet.type != PACKET_HELLO) {
                    clients[packet.playerId] = sender;
                    clientConnected[packet.playerId] = true;
                    SendPacket(packet);
                }
                if (packet.playerId >= 0 && packet.playerId < MAX_PLAYERS && packet.playerId != myId) {
                    if (packet.type == PACKET_POS) {
                        players[packet.playerId].position = (Vector2){packet.x, packet.y};
                        players[packet.playerId].colorIndex = packet.data1;
                        players[packet.playerId].active = true;
                    } else if (packet.type == PACKET_BLOCK_ADD) {
                        AddBlock(packet.x, packet.y, (packet.data2 == SHAPE_RECT) ? BLOCK_SIZE * 2 : BLOCK_SIZE, BLOCK_SIZE, packet.data1, packet.data2);
                    } else if (packet.type == PACKET_BLOCK_REM) {
                        RemoveBlock(packet.x, packet.y);
                    } else if (packet.type == PACKET_ENV_UPDATE) {
                        currentWeather = (WeatherType)packet.data1;
                        isNight = (bool)packet.data2;
                    }
                }
            }
        }

        if (gameState == STATE_MENU) {
            if (IsKeyPressed(KEY_H)) {
                myId = 0; InitNetwork(true, ""); InitGame(); gameState = STATE_GAME;
            }
            for(int i=1; i<MAX_PLAYERS; i++) {
                if (IsKeyPressed(KEY_ONE + i - 1)) {
                    myId = i; InitNetwork(false, targetIP); InitGame(); gameState = STATE_GAME;
                    NetPacket pHello = {PACKET_HELLO, myId};
                    for(int j=0; j<3; j++) SendPacket(pHello);
                }
            }
            int key = GetCharPressed();
            while (key > 0) {
                if ((key >= 32) && (key <= 125) && (ipLetterCount < 30)) {
                    targetIP[ipLetterCount] = (char)key;
                    targetIP[ipLetterCount+1] = '\0';
                    ipLetterCount++;
                }
                key = GetCharPressed();
            }
            if (IsKeyPressed(KEY_BACKSPACE) && ipLetterCount > 0) targetIP[--ipLetterCount] = '\0';

            BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("MULTIPLAYER 4P LAN", 100, 80, 40, DARKGRAY);
            DrawText("[H] HOST (P0)", 100, 160, 20, BLACK);
            DrawText("[1] JOIN AS P1 | [2] JOIN AS P2 | [3] JOIN AS P3", 100, 195, 20, DARKBLUE);
            DrawText("IP SERVER:", 100, 300, 20, GRAY);
            DrawRectangle(100, 330, 300, 40, LIGHTGRAY);
            DrawText(targetIP, 110, 340, 20, BLACK);
            EndDrawing();
        } else {
            float dt = fminf(GetFrameTime(), 0.034f);
            if (IsKeyPressed(KEY_X)) InitGame();
            if (IsKeyPressed(KEY_F3)) players[myId].colorIndex = (players[myId].colorIndex + 1) % 6;
            if (isServer) {
                bool envChanged = false;
                if (IsKeyPressed(KEY_F4)) { isNight = !isNight; envChanged = true; }
                if (IsKeyPressed(KEY_F5)) { currentWeather = (currentWeather + 1) % 3; InitParticles(); envChanged = true; }
                if (envChanged) {
                    NetPacket pEnv = {PACKET_ENV_UPDATE, myId, 0, 0, (int)currentWeather, (int)isNight};
                    SendPacket(pEnv);
                }
            }
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                selectedShapeIndex = (selectedShapeIndex + (int)wheel + 5) % 5;
                previewTimer = 3.0f;
            }
            if (previewTimer > 0) previewTimer -= dt;
            if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
                selectedColorIndex = (selectedColorIndex + 1) % 5;
                previewTimer = 3.0f;
            }
            Player *me = &players[myId];
            float currentSpeed = IsKeyDown(KEY_DOWN) ? PLAYER_RUN_SPEED : PLAYER_SPEED;
            if (IsKeyDown(KEY_RIGHT)) me->velocity.x = currentSpeed;
            else if (IsKeyDown(KEY_LEFT)) me->velocity.x = -currentSpeed;
            else me->velocity.x = 0;
            if (IsKeyPressed(KEY_UP) && me->grounded) { me->velocity.y = -JUMP_FORCE; me->grounded = false; }
            me->velocity.y += GRAVITY * dt;
            if (me->velocity.y > MAX_FALL_SPEED) me->velocity.y = MAX_FALL_SPEED;
            me->position.x += me->velocity.x * dt;
            Rectangle pRect = { me->position.x, me->position.y, 40, 40 }; 
            for (int i = 0; i < MAX_BLOCKS; i++) {
                if (blocks[i].active && CheckCollisionRecs(pRect, blocks[i].rect)) {
                    if (me->velocity.x > 0) me->position.x = blocks[i].rect.x - 40;
                    else if (me->velocity.x < 0) me->position.x = blocks[i].rect.x + blocks[i].rect.width;
                }
            }
            me->position.y += me->velocity.y * dt;
            me->grounded = false;
            pRect = (Rectangle){me->position.x, me->position.y, 40, 40};
            for (int i = 0; i < MAX_BLOCKS; i++) {
                if (blocks[i].active && CheckCollisionRecs(pRect, blocks[i].rect)) {
                    if (me->velocity.y > 0) { me->position.y = blocks[i].rect.y - 40; me->velocity.y = 0; me->grounded = true; }
                    else if (me->velocity.y < 0) { me->position.y = blocks[i].rect.y + blocks[i].rect.height; me->velocity.y = 0; }
                }
            }
            if (me->position.y > 2000) ResetPlayer(me, blocks[0]);
            NetPacket posP = {PACKET_POS, myId, me->position.x, me->position.y, me->colorIndex};
            SendPacket(posP);
            camera.target.x += (me->position.x - camera.target.x) * 5.0f * dt;
            camera.target.y += (me->position.y - camera.target.y) * 5.0f * dt;
            Vector2 mWorld = GetScreenToWorld2D(GetMousePosition(), camera);
            int gx = (int)floor(mWorld.x / BLOCK_SIZE) * BLOCK_SIZE;
            int gy = (int)floor(mWorld.y / BLOCK_SIZE) * BLOCK_SIZE;
            int bw = (selectedShapeIndex == SHAPE_RECT) ? BLOCK_SIZE * 2 : BLOCK_SIZE;
            Rectangle potB = { (float)gx, (float)gy, (float)bw, (float)BLOCK_SIZE };
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                bool ok = true;
                for(int i=0; i<MAX_PLAYERS; i++) if(players[i].active && CheckCollisionRecs(potB, (Rectangle){players[i].position.x, players[i].position.y, 40, 40})) ok = false;
                for(int i=0; i<MAX_BLOCKS; i++) if(blocks[i].active && CheckCollisionRecs(potB, blocks[i].rect)) ok = false;
                if (ok) {
                    AddBlock(potB.x, potB.y, bw, BLOCK_SIZE, selectedColorIndex, selectedShapeIndex);
                    NetPacket bP = {PACKET_BLOCK_ADD, myId, potB.x, potB.y, selectedColorIndex, selectedShapeIndex};
                    SendPacket(bP); SendPacket(bP);
                }
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                for (int i = 1; i < MAX_BLOCKS; i++) { 
                    if (blocks[i].active && CheckCollisionPointRec(mWorld, blocks[i].rect)) {
                        blocks[i].active = 0;
                        NetPacket rP = {PACKET_BLOCK_REM, myId, blocks[i].rect.x, blocks[i].rect.y};
                        SendPacket(rP); SendPacket(rP); break;
                    }
                }
            }
            UpdateWeather(currentWeather, camera, screenWidth, screenHeight);
            BeginDrawing();
                ClearBackground(isNight ? (Color){ 10, 10, 30, 255 } : SKYBLUE);
                BeginMode2D(camera);
                    for (int i = 0; i < MAX_BLOCKS; i++) if (blocks[i].active) DrawBlockShape(blocks[i]);
                    for (int i = 0; i < MAX_PLAYERS; i++) if (players[i].active) DrawPlayerRender(players[i], i, playerColors[players[i].colorIndex]);
                    DrawWeather(currentWeather);
                    DrawRectangleLinesEx(potB, 2, WHITE);
                EndMode2D();
                if (previewTimer > 0) {
                    DrawRectangle(screenWidth - 150, screenHeight - 150, 140, 140, Fade(BLACK, 0.5f));
                    Block pb = { {screenWidth - 110, screenHeight - 100, (selectedShapeIndex == SHAPE_RECT) ? 80 : 40, 40}, 1, blockColors[selectedColorIndex], selectedShapeIndex };
                    DrawBlockShape(pb);
                    DrawText(shapeNames[selectedShapeIndex], screenWidth - 140, screenHeight - 30, 10, WHITE);
                }
            EndDrawing();
        }
    }
    closesocket(sock); WSACleanup(); CloseWindow(); return 0;
}