#include "raylib.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "rlgl.h"

#define MAX_BLOCKS 2000
#define MAX_PARTICLES 500
#define PLAYER_SPEED 300.0f
#define PLAYER_RUN_SPEED 500.0f
#define JUMP_FORCE 550.0f
#define GRAVITY 1000.0f
#define MAX_FALL_SPEED 800.0f
#define BLOCK_SIZE 40
#define FREE_CAM_SPEED 600.0f
#define SONG_COUNT 6	

#define CONSOLE_HISTORY 100
#define CONSOLE_VISIBLE 15
#define NUM_GEARS 7
#define GEAR_OFFSET_X 45.0f

#define NUM_CUSTOM_BLOCKS 12

typedef enum {
	SHAPE_SQUARE, SHAPE_RECT, SHAPE_TRIANGLE, SHAPE_CIRCLE, SHAPE_RHOMBUS,
	SHAPE_CUST1, SHAPE_CUST2, SHAPE_CUST3, SHAPE_CUST4, SHAPE_CUST5, SHAPE_CUST6,
	SHAPE_CUST7, SHAPE_CUST8, SHAPE_CUST9, SHAPE_CUST10, SHAPE_CUST11, SHAPE_CUST12
} BlockShape;

typedef enum { WEATHER_NONE, WEATHER_RAIN, WEATHER_SNOW } WeatherType;
typedef enum { CAM_FIXED, CAM_SMOOTH, CAM_FREE } GameCameraMode;

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
	bool facingRight;
} Player;

typedef struct {
	Vector2 playerPos;
	bool isNightState;
	WeatherType weatherType;
	int playerColor;
	int blockColor;
	int blockShape;
	int activeBlocksCount;
	Block blocksToSave[MAX_BLOCKS];
} GameData;

Block blocks[MAX_BLOCKS];
Particle particles[MAX_PARTICLES];
Color blockColors[5];
Color playerColors[6];

Texture2D playerTexture;
bool hasPlayerTexture = false;

Texture2D cursorTexture;
bool hasCursorTexture = false;

Texture2D gearTextures[NUM_GEARS];
int currentGearIndex = 0;

Texture2D customBlockTextures[NUM_CUSTOM_BLOCKS];

Sound fxDeath;
bool hasDeathSound = false;

Music songs[SONG_COUNT] = { 0 };
int currentSongIndex = 0;
const char* songFiles[] = {
	"sounds/song1.mp3",
	"sounds/song2.mp3",
	"sounds/song3.mp3",
	"sounds/song4.mp3",
	"sounds/song5.mp3",
	"sounds/song6.mp3"
};

const char* shapeNames[] = {
	"CUADRADO", "RECTANGULO", "TRIANGULO", "CIRCULO", "ROMBO",
	"CUSTOM 1", "CUSTOM 2", "CUSTOM 3", "CUSTOM 4", "CUSTOM 5", "CUSTOM 6",
	"CUSTOM 7", "CUSTOM 8", "CUSTOM 9", "CUSTOM 10", "CUSTOM 11", "CUSTOM 12"
};

const char* playerColorNames[] = { "GRIS", "NARANJA", "VIOLETA", "ORO", "LIMA", "AZUL" };
const char* blockColorNames[] = { "AZUL", "ROJO", "VERDE", "AMARILLO", "ROSA" };
const char* cameraModeNames[] = { "FIJA", "SUAVE", "LIBRE" };
const char* weatherNames[] = { "DESPEJADO", "LLUVIA", "NIEVE" };
const char* dayNightNames[] = { "DIA", "NOCHE" };

char consoleLog[CONSOLE_HISTORY][128];
int consoleScroll = 0;
bool showConsole = false;

bool showCheatUI = false;
char cheatBuffer[7] = { 0 };
bool cheatFly = false;
bool cheatInfJump = false;
bool cheatNoClip = false;

static void AddConsoleLog(const char* text) {
	for (int i = 0; i < CONSOLE_HISTORY - 1; i++) {
		strcpy(consoleLog[i], consoleLog[i + 1]);
	}
	strncpy(consoleLog[CONSOLE_HISTORY - 1], text, 127);
	consoleScroll = 0;
}

static void UpdateCheatState() {
	cheatFly = false;
	cheatInfJump = false;
	cheatNoClip = false;

	if (strcmp(cheatBuffer, "29103") == 0) cheatFly = true;
	if (strcmp(cheatBuffer, "84721") == 0) cheatInfJump = true;
	if (strcmp(cheatBuffer, "112233") == 0) cheatNoClip = true;
}

static void ResetPlayer(Player* player, Block startPlatform) {
	player->position = (Vector2){ startPlatform.rect.x + 50, startPlatform.rect.y - 100 };
	player->velocity = (Vector2){ 0, 0 };
	player->facingRight = true;
	AddConsoleLog("Player position reset");
}

static void ResetGame(Player* player, Block* baseBlocks) {
	for (int i = 1; i < MAX_BLOCKS; i++) {
		blocks[i].active = 0;
	}
	ResetPlayer(player, baseBlocks[0]);
	AddConsoleLog("Game map reset");
}

static void InitParticles() {
	for (int i = 0; i < MAX_PARTICLES; i++) {
		particles[i].active = 0;
	}
}

static void UpdateWeather(WeatherType weather, Camera2D cam, int screenW, int screenH) {
	if (weather == WEATHER_NONE) return;

	float dt = GetFrameTime();

	for (int i = 0; i < MAX_PARTICLES; i++) {
		if (!particles[i].active) {
			particles[i].position.x = (float)GetRandomValue((int)(cam.target.x - screenW), (int)(cam.target.x + screenW));
			particles[i].position.y = cam.target.y - screenH / 2.0f - GetRandomValue(0, 200);
			particles[i].speed = (float)((weather == WEATHER_RAIN) ? GetRandomValue(400, 800) : GetRandomValue(50, 150));
			particles[i].active = 1;
		}

		particles[i].position.y += particles[i].speed * dt;
		if (weather == WEATHER_SNOW) particles[i].position.x += (float)GetRandomValue(-50, 50) * dt;

		if (particles[i].position.y > cam.target.y + screenH) {
			particles[i].active = 0;
		}
	}
}

static void DrawWeather(WeatherType weather) {
	if (weather == WEATHER_NONE) return;
	for (int i = 0; i < MAX_PARTICLES; i++) {
		if (particles[i].active) {
			if (weather == WEATHER_RAIN) {
				DrawLineV(particles[i].position, (Vector2) { particles[i].position.x, particles[i].position.y + 10 }, Fade(BLUE, 0.7f));
			}
			else {
				DrawCircleV(particles[i].position, 2, Fade(WHITE, 0.8f));
			}
		}
	}
}

static void DrawBlockShape(Block b) {
	if (b.shape >= SHAPE_CUST1 && b.shape <= SHAPE_CUST12) {
		int texIndex = b.shape - SHAPE_CUST1;
		if (customBlockTextures[texIndex].id != 0) {
			Rectangle source = { 0.0f, 0.0f, (float)customBlockTextures[texIndex].width, (float)customBlockTextures[texIndex].height };
			Vector2 origin = { 0.0f, 0.0f };
			DrawTexturePro(customBlockTextures[texIndex], source, b.rect, origin, 0.0f, WHITE);
		}
		else {
			DrawRectangleRec(b.rect, MAGENTA);
		}
		return;
	}

	switch (b.shape) {
	case SHAPE_SQUARE: DrawRectangleRec(b.rect, b.color); break;
	case SHAPE_RECT: DrawRectangleRec(b.rect, b.color); break;
	case SHAPE_TRIANGLE:
		DrawTriangle(
			(Vector2) {
			b.rect.x + b.rect.width / 2, b.rect.y
		},
			(Vector2) {
			b.rect.x, b.rect.y + b.rect.height
		},
			(Vector2) {
			b.rect.x + b.rect.width, b.rect.y + b.rect.height
		},
			b.color
		);
		break;
	case SHAPE_CIRCLE:
		DrawCircle((int)(b.rect.x + b.rect.width / 2), (int)(b.rect.y + b.rect.height / 2), (float)b.rect.width / 2, b.color);
		break;
	case SHAPE_RHOMBUS:
		DrawTriangle(
			(Vector2) {
			b.rect.x + b.rect.width / 2, b.rect.y
		},
			(Vector2) {
			b.rect.x, b.rect.y + b.rect.height / 2
		},
			(Vector2) {
			b.rect.x + b.rect.width, b.rect.y + b.rect.height / 2
		},
			b.color
		);
		DrawTriangle(
			(Vector2) {
			b.rect.x, b.rect.y + b.rect.height / 2
		},
			(Vector2) {
			b.rect.x + b.rect.width / 2, b.rect.y + b.rect.height
		},
			(Vector2) {
			b.rect.x + b.rect.width, b.rect.y + b.rect.height / 2
		},
			b.color
		);
		break;
	}
}

static void DrawPlayer(Player p, Color color) {
	if (hasPlayerTexture) {
		Rectangle sourceRec = { 0.0f, 0.0f, (float)playerTexture.width, (float)playerTexture.height };
		if (!p.facingRight) sourceRec.width = -sourceRec.width;
		Rectangle destRec = { p.position.x, p.position.y, 40.0f, 40.0f };
		Vector2 origin = { 0.0f, 0.0f };
		DrawTexturePro(playerTexture, sourceRec, destRec, origin, 0.0f, WHITE);
	}
	else {
		DrawRectangleV(p.position, (Vector2) { 40, 40 }, color);
		if (p.facingRight) {
			DrawRectangleV((Vector2) { p.position.x + 24, p.position.y + 12 }, (Vector2) { 6, 6 }, BLACK);
			DrawRectangleV((Vector2) { p.position.x + 30, p.position.y + 12 }, (Vector2) { 6, 6 }, BLACK);
		}
		else {
			DrawRectangleV((Vector2) { p.position.x + 4, p.position.y + 12 }, (Vector2) { 6, 6 }, BLACK);
			DrawRectangleV((Vector2) { p.position.x + 10, p.position.y + 12 }, (Vector2) { 6, 6 }, BLACK);
		}
	}
}

static void DrawTextRight(const char* text, int y, int fontSize, Color color, int screenWidth) {
	int textWidth = MeasureText(text, fontSize);
	DrawText(text, screenWidth - textWidth - 10, y, fontSize, color);
}

static void SaveGame(const Player* player, const Block* blocks, int isNight, WeatherType weather, int playerColorIndex, int selectedColorIndex, int selectedShapeIndex) {
	static GameData data = { 0 };
	memset(&data, 0, sizeof(GameData));
	data.playerPos = player->position;
	data.isNightState = isNight;
	data.weatherType = weather;
	data.playerColor = playerColorIndex;
	data.blockColor = selectedColorIndex;
	data.blockShape = selectedShapeIndex;

	int activeCount = 0;
	for (int i = 0; i < MAX_BLOCKS; i++) {
		if (blocks[i].active) {
			if (activeCount < MAX_BLOCKS) {
				data.blocksToSave[activeCount] = blocks[i];
				activeCount++;
			}
		}
	}
	data.activeBlocksCount = activeCount;

	unsigned int dataSize = sizeof(GameData);
	if (SaveFileData("level.dat", &data, dataSize)) {
		AddConsoleLog(TextFormat("Game saved successfully: %d blocks", activeCount));
	}
	else {
		AddConsoleLog("Error saving game data!");
	}
}

static void LoadGame(Player* player, Block* blocks, int* isNight, WeatherType* weather, int* playerColorIndex, int* selectedColorIndex, int* selectedShapeIndex) {
	unsigned int bytesRead = 0;
	unsigned char* fileData = LoadFileData("level.dat", &bytesRead);

	if (fileData != NULL) {
		if (bytesRead >= sizeof(GameData)) {
			static GameData data = { 0 };
			memcpy(&data, fileData, sizeof(GameData));

			player->position = data.playerPos;
			player->velocity = (Vector2){ 0, 0 };
			player->grounded = false;

			*isNight = data.isNightState;
			*weather = data.weatherType;
			*playerColorIndex = data.playerColor;
			*selectedColorIndex = data.blockColor;
			*selectedShapeIndex = data.blockShape;

			for (int i = 0; i < MAX_BLOCKS; i++) {
				blocks[i].active = 0;
			}

			for (int i = 0; i < data.activeBlocksCount; i++) {
				blocks[i] = data.blocksToSave[i];
				blocks[i].active = 1;
			}

			InitParticles();
			AddConsoleLog(TextFormat("Game loaded successfully: %d blocks", data.activeBlocksCount));
		}
		else {
			AddConsoleLog("Load failed: File size mismatch");
		}
		UnloadFileData(fileData);
	}
	else {
		AddConsoleLog("Load failed: level.dat not found");
	}
}

int main(void) {

	//SetConfigFlags(FLAG_VSYNC_HINT);
	SetExitKey(KEY_ESCAPE);
    SetConfigFlags(FLAG_WINDOW_ALWAYS_RUN);

	int screenWidth = 1280;
	int screenHeight = 720;

	for (int i = 0; i < CONSOLE_HISTORY; i++) memset(consoleLog[i], 0, 128);

	InitWindow(screenWidth, screenHeight, "SDFX Engine - Cargando...");
	InitAudioDevice();

	SetTargetFPS(60);

	AddConsoleLog("System Initialized");

	int glVersion = rlGetVersion();
	const char* glText = "Desconocido";

	if (glVersion == RL_OPENGL_11) glText = "1.1";
    else if (glVersion == RL_OPENGL_21) glText = "2.1";
    else if (glVersion == RL_OPENGL_33) glText = "3.3";
    else if (glVersion == RL_OPENGL_43) glText = "4.3";
    else if (glVersion == 4) glText = "4.x (Core)";

	SetWindowTitle("Platform");
	AddConsoleLog(TextFormat("GPU: OpenGL %s initialized correctly", glText));

	if (FileExists("images/player.png")) {
		playerTexture = LoadTexture("images/player.png");
		hasPlayerTexture = true;
		AddConsoleLog("Texture: images/player.png loaded");
	}
	else {
		AddConsoleLog("Texture: player.png not found, using default");
	}

	if (FileExists("images/cursor.png")) {
		cursorTexture = LoadTexture("images/cursor.png");
		hasCursorTexture = true;
		HideCursor();
		AddConsoleLog("Texture: images/cursor.png loaded");
	}
	else {
		AddConsoleLog("Texture: cursor.png not found, using system cursor");
	}

	for (int i = 0; i < NUM_GEARS; i++) {
		const char* fileName = TextFormat("images/gear%d.png", i + 1);
		if (FileExists(fileName)) {
			gearTextures[i] = LoadTexture(fileName);
		}
		else {
			AddConsoleLog(TextFormat("Warning: %s not found", fileName));
		}
	}

	AddConsoleLog("Gear textures initialized");

	for (int i = 0; i < NUM_CUSTOM_BLOCKS; i++) {
		const char* customName = TextFormat("custom/customblock%d.png", i + 1);
		if (FileExists(customName)) {
			customBlockTextures[i] = LoadTexture(customName);
			AddConsoleLog(TextFormat("Custom Block Loaded: %s", customName));
		}
		else {
			AddConsoleLog(TextFormat("Warning: %s not found", customName));
		}
	}

	if (FileExists("sounds/oof.mp3")) {
		fxDeath = LoadSound("sounds/oof.mp3");
		hasDeathSound = true;
		AddConsoleLog("SFX: oof.mp3 loaded");
	}
	else {
		AddConsoleLog("SFX: oof.mp3 not found");
	}

	for (int i = 0; i < SONG_COUNT; i++) {
		if (FileExists(songFiles[i])) {
			songs[i] = LoadMusicStream(songFiles[i]);
			songs[i].looping = true;
			AddConsoleLog(TextFormat("Music: %s loaded", songFiles[i]));
		}
		else {
			AddConsoleLog(TextFormat("Music: %s not found", songFiles[i]));
		}
	}

	PlayMusicStream(songs[0]);

	AddConsoleLog("Audio System Started");

	blockColors[0] = BLUE; blockColors[1] = RED; blockColors[2] = GREEN;
	blockColors[3] = YELLOW; blockColors[4] = PINK;

	playerColors[0] = GRAY; playerColors[1] = ORANGE; playerColors[2] = VIOLET;
	playerColors[3] = GOLD; playerColors[4] = LIME; playerColors[5] = BLUE;

	blocks[0].active = 1;
	blocks[0].rect = (Rectangle){ -200, 300, 800, 40 };
	blocks[0].color = GRAY;
	blocks[0].shape = SHAPE_RECT;

	Player player = { 0 };
	ResetPlayer(&player, blocks[0]);

	Camera2D camera = { 0 };
	camera.target = player.position;
	camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };
	camera.rotation = 0.0f;
	camera.zoom = 1.0f;

	int selectedColorIndex = 0;
	int selectedShapeIndex = 0;
	int playerColorIndex = 0;

	float previewTimer = 0.0f;
	bool showControls = false;
	bool isStarting = true;
	float startTimer = 0.0f;
	bool showStats = false;
	bool showDebug = false;
	int isNight = 0;
	bool hideUI = false;
	bool gamePaused = false;
	float gearSpeeds[NUM_GEARS] = { 0.0f, 0.0f, 0.0f, 0.0f, 120.0f, 120.0f, 120.0f };
	float gearAngles[NUM_GEARS] = { 0 };

	WeatherType currentWeather = WEATHER_NONE;
	GameCameraMode currentCameraMode = CAM_SMOOTH;

	InitParticles();
	Rectangle potentialBlock = { 0 };
	Rectangle playerRect = { 0 };

	while (!WindowShouldClose()) {
		float dt = GetFrameTime();
		if (dt > 0.034f) dt = 0.034f;

		for (int i = 0; i < NUM_GEARS; i++) {
			gearAngles[i] += gearSpeeds[i] * dt;
			if (gearAngles[i] >= 360.0f) gearAngles[i] -= 360.0f;
		}

		if (isStarting) {
			gamePaused = true;
			startTimer += GetFrameTime();
			if (startTimer >= 3.0f) {
				isStarting = false;
				gamePaused = false;
				AddConsoleLog("System loaded. Welcome.");
			}
		}

		if (songs[currentSongIndex].stream.buffer != NULL) {
			UpdateMusicStream(songs[currentSongIndex]);
		}

		if (IsKeyPressed(KEY_F1)) {
            AddConsoleLog("HELP: https://github.com/agustinsdfx/Platform/blob/main/doc/HELP.md");
        }

		if (IsKeyPressed(KEY_P)) {
			gamePaused = !gamePaused;
			if (gamePaused) AddConsoleLog("GAME PAUSED");
			else AddConsoleLog("GAME RESUMED");
		}

		if (!gamePaused) {

			if (IsKeyPressed(KEY_F7)) {
				if (songs[currentSongIndex].stream.buffer != NULL) StopMusicStream(songs[currentSongIndex]);
				currentSongIndex++;
				if (currentSongIndex >= SONG_COUNT) currentSongIndex = 0;
				if (songs[currentSongIndex].stream.buffer != NULL) PlayMusicStream(songs[currentSongIndex]);
				AddConsoleLog(TextFormat("Music changed to: %s", songFiles[currentSongIndex]));
			}

			if (IsKeyPressed(KEY_F8)) {
				SaveGame(&player, blocks, isNight, currentWeather, playerColorIndex, selectedColorIndex, selectedShapeIndex);
			}
			if (IsKeyPressed(KEY_F10)) showConsole = !showConsole;
			if (IsKeyPressed(KEY_F9)) {
				LoadGame(&player, blocks, &isNight, &currentWeather, &playerColorIndex, &selectedColorIndex, &selectedShapeIndex);
			}

			if (IsKeyPressed(KEY_C)) {
				hideUI = !hideUI;
				AddConsoleLog(hideUI ? "UI Hidden" : "UI Visible");
			}

			if (IsKeyPressed(KEY_F2)) showStats = !showStats;
			if (IsKeyPressed(KEY_F3)) showDebug = !showDebug;
			if (IsKeyPressed(KEY_F4)) {
				isNight = (isNight == 0);
				AddConsoleLog(isNight ? "Time: Night" : "Time: Day");
			}
			if (IsKeyPressed(KEY_F5)) {
				currentWeather = (WeatherType)(currentWeather + 1);
				if (currentWeather > 2) currentWeather = WEATHER_NONE;
				InitParticles();
				AddConsoleLog(TextFormat("Weather set to: %s", weatherNames[currentWeather]));
			}
			if (IsKeyPressed(KEY_F6)) {
				int nextMode = (int)currentCameraMode + 1;
				if (nextMode > (int)CAM_FREE) nextMode = (int)CAM_FIXED;
				currentCameraMode = (GameCameraMode)nextMode;
				AddConsoleLog(TextFormat("Camera set to: %s", cameraModeNames[currentCameraMode]));
			}

			if (IsKeyPressed(KEY_F11)) {
				if (!IsWindowState(FLAG_WINDOW_UNDECORATED)) {
					int monitor = GetCurrentMonitor();
					int mWidth = GetMonitorWidth(monitor);
					int mHeight = GetMonitorHeight(monitor);

					SetWindowState(FLAG_WINDOW_UNDECORATED);
					SetWindowSize(mWidth, mHeight);
					SetWindowPosition(0, 0);

					screenWidth = mWidth;
					screenHeight = mHeight;

					camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };

					AddConsoleLog("Fullscreen");
				}
				else {
					ClearWindowState(FLAG_WINDOW_UNDECORATED);
					SetWindowSize(1280, 720);

					int monitor = GetCurrentMonitor();
					SetWindowPosition(GetMonitorWidth(monitor) / 2 - 640, GetMonitorHeight(monitor) / 2 - 360);

					screenWidth = 1280;
					screenHeight = 720;

					camera.offset = (Vector2){ screenWidth / 2.0f, screenHeight / 2.0f };

					AddConsoleLog("Ventana");
				}
			}

			if (IsKeyPressed(KEY_F12)) {
				TakeScreenshot("screenshot.png");
				AddConsoleLog("Screenshot saved: screenshot.png");
			}

			if (IsKeyPressed(KEY_K)) {
				if (IsKeyDown(KEY_LEFT_SHIFT)) {
					memset(cheatBuffer, 0, 7);
					UpdateCheatState();
					AddConsoleLog("CHEATS: ALL CLEARED");
				}
				else {
					showCheatUI = !showCheatUI;
				}
			}

			if (showCheatUI) {
				int key;
				while ((key = GetKeyPressed()) != 0) {
					if ((key >= KEY_ZERO) && (key <= KEY_NINE)) {
						char numChar = (char)('0' + (key - KEY_ZERO));
						int len = (int)strlen(cheatBuffer);
						if (len < 6) {
							cheatBuffer[len] = numChar;
							cheatBuffer[len + 1] = '\0';
							UpdateCheatState();
						}
					}
				}
				if (IsKeyPressed(KEY_BACKSPACE)) {
					int len = (int)strlen(cheatBuffer);
					if (len > 0) {
						cheatBuffer[len - 1] = '\0';
						UpdateCheatState();
					}
				}
			}

			if (IsKeyPressed(KEY_Z)) {
				playerColorIndex++;
				if (playerColorIndex > 5) playerColorIndex = 0;
			}

			if (IsKeyPressed(KEY_X)) {
				if (hasDeathSound) PlaySound(fxDeath);
				ResetGame(&player, blocks);
			}

			if (IsKeyPressed(KEY_G)) {
				currentGearIndex++;
				if (currentGearIndex >= NUM_GEARS) currentGearIndex = 0;
				AddConsoleLog(TextFormat("Gear Changed: %d", currentGearIndex + 1));
			}

			if (!showConsole && !showCheatUI) {
				float wheel = GetMouseWheelMove();
				if (wheel != 0) {
					selectedShapeIndex += (int)wheel;
					if (selectedShapeIndex < 0) selectedShapeIndex = 16;
					if (selectedShapeIndex > 16) selectedShapeIndex = 0;
					previewTimer = 3.0f;
				}

				if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) {
					selectedColorIndex++;
					if (selectedColorIndex > 4) selectedColorIndex = 0;
					previewTimer = 3.0f;
				}
			}

			if (previewTimer > 0) previewTimer -= dt;

			if (currentCameraMode == CAM_FREE || cheatFly) {
				float moveSpeed = (cheatFly) ? PLAYER_SPEED * 1.5f : FREE_CAM_SPEED;
				float dtSpeed = moveSpeed * dt;

				Vector2* targetPos = (cheatFly) ? &player.position : &camera.target;

				if (IsKeyDown(KEY_RIGHT)) targetPos->x += dtSpeed;
				if (IsKeyDown(KEY_LEFT)) targetPos->x -= dtSpeed;
				if (IsKeyDown(KEY_UP)) targetPos->y -= dtSpeed;
				if (IsKeyDown(KEY_DOWN)) targetPos->y += dtSpeed;

				player.velocity = (Vector2){ 0, 0 };
				if (cheatFly) camera.target = player.position;
			}
			else {
				float currentSpeed = PLAYER_SPEED;
				if (IsKeyDown(KEY_DOWN)) currentSpeed = PLAYER_RUN_SPEED;

				if (IsKeyDown(KEY_RIGHT)) {
					player.velocity.x = currentSpeed;
					player.facingRight = true;
				}
				else if (IsKeyDown(KEY_LEFT)) {
					player.velocity.x = -currentSpeed;
					player.facingRight = false;
				}
				else {
					player.velocity.x = 0;
				}

				if (IsKeyPressed(KEY_UP)) {
					if (player.grounded || cheatInfJump) {
						player.velocity.y = -JUMP_FORCE;
						player.grounded = false;
					}
				}

				player.velocity.y += GRAVITY * dt;
			}

			if (!cheatFly) {
				if (player.velocity.y > MAX_FALL_SPEED) player.velocity.y = MAX_FALL_SPEED;
				player.position.x += player.velocity.x * dt;
			}

			playerRect = (Rectangle){ player.position.x, player.position.y, 40, 40 };

			if (!cheatNoClip) {
				for (int i = 0; i < MAX_BLOCKS; i++) {
					if (blocks[i].active && CheckCollisionRecs(playerRect, blocks[i].rect)) {
						if (player.velocity.x > 0) player.position.x = blocks[i].rect.x - playerRect.width;
						else if (player.velocity.x < 0) player.position.x = blocks[i].rect.x + blocks[i].rect.width;
					}
				}
			}

			if (!cheatFly) {
				player.position.y += player.velocity.y * dt;
			}

			player.grounded = false;
			playerRect.x = player.position.x;
			playerRect.y = player.position.y;

			if (!cheatNoClip) {
				for (int i = 0; i < MAX_BLOCKS; i++) {
					if (blocks[i].active && CheckCollisionRecs(playerRect, blocks[i].rect)) {
						if (player.velocity.y > 0) {
							player.position.y = blocks[i].rect.y - playerRect.height;
							player.velocity.y = 0;
							player.grounded = true;
						}
						else if (player.velocity.y < 0) {
							player.position.y = blocks[i].rect.y + blocks[i].rect.height;
							player.velocity.y = 0;
						}
					}
				}
			}

			if (player.position.y > 2000 && !cheatFly) {
				if (hasDeathSound) PlaySound(fxDeath);
				ResetPlayer(&player, blocks[0]);
				AddConsoleLog("Player died in void");
			}

			if (currentCameraMode == CAM_SMOOTH && !cheatFly) {
				camera.target.x += (player.position.x - camera.target.x) * 5.0f * dt;
				camera.target.y += (player.position.y - camera.target.y) * 5.0f * dt;
			}

			Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
			int gridX = (int)((mouseWorldPos.x < 0) ? (mouseWorldPos.x - BLOCK_SIZE) : mouseWorldPos.x) / BLOCK_SIZE * BLOCK_SIZE;
			int gridY = (int)((mouseWorldPos.y < 0) ? (mouseWorldPos.y - BLOCK_SIZE) : mouseWorldPos.y) / BLOCK_SIZE * BLOCK_SIZE;
			int currentWidth = (selectedShapeIndex == SHAPE_RECT) ? BLOCK_SIZE * 2 : BLOCK_SIZE;
			potentialBlock = (Rectangle){ (float)gridX, (float)gridY, (float)currentWidth, (float)BLOCK_SIZE };

			if (!showConsole && !showCheatUI) {
				if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
					bool freeSpace = !CheckCollisionRecs(potentialBlock, playerRect);
					for (int i = 0; i < MAX_BLOCKS; i++) {
						if (blocks[i].active && CheckCollisionRecs(potentialBlock, blocks[i].rect)) {
							freeSpace = false; break;
						}
					}
					if (freeSpace) {
						for (int i = 1; i < MAX_BLOCKS; i++) {
							if (!blocks[i].active) {
								blocks[i].active = 1;
								blocks[i].rect = potentialBlock;
								blocks[i].color = blockColors[selectedColorIndex];
								blocks[i].shape = (BlockShape)selectedShapeIndex;
								break;
							}
						}
					}
				}

				if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
					for (int i = 1; i < MAX_BLOCKS; i++) {
						if (blocks[i].active && CheckCollisionPointRec(mouseWorldPos, blocks[i].rect)) {
							blocks[i].active = 0;
						}
					}
				}
			}

			UpdateWeather(currentWeather, camera, screenWidth, screenHeight);
		}

		if (IsKeyPressed(KEY_ESCAPE)) break;

		BeginDrawing();

		ClearBackground(isNight ? (Color) { 10, 10, 30, 255 } : SKYBLUE);

		Rectangle screenView = {
			camera.target.x - (screenWidth / 2 / camera.zoom),
			camera.target.y - (screenHeight / 2 / camera.zoom),
			screenWidth / camera.zoom,
			screenHeight / camera.zoom
		};

		BeginMode2D(camera);
		for (int i = 0; i < MAX_BLOCKS; i++) {
			if (blocks[i].active) {
				if (CheckCollisionRecs(screenView, blocks[i].rect)) {
					DrawBlockShape(blocks[i]);
				}
			}
		}

		DrawPlayer(player, playerColors[playerColorIndex]);

		Texture2D currentGear = gearTextures[currentGearIndex];
		if (currentGear.id != 0) {
			float centerX = player.position.x + 20.0f;
			float gearX = centerX;
			float gearY = player.position.y + 15.0f;
			float rotation = gearAngles[currentGearIndex];

			if (player.facingRight) {
				gearX += GEAR_OFFSET_X;
			}
			else {
				gearX -= GEAR_OFFSET_X;
			}

			Rectangle sourceRec = { 0.0f, 0.0f, (float)currentGear.width, (float)currentGear.height };
			Rectangle destRec = { gearX, gearY, (float)currentGear.width, (float)currentGear.height };
			Vector2 origin = { (float)currentGear.width / 2, (float)currentGear.height / 2 };
			DrawTexturePro(currentGear, sourceRec, destRec, origin, rotation, WHITE);
		}

		DrawWeather(currentWeather);
		if (!hideUI && !gamePaused) DrawRectangleLinesEx(potentialBlock, 2, WHITE);
		EndMode2D();

		Color cToggleColor = hideUI ? GRAY : WHITE;
		DrawTextRight("C: UI (Toggle)", 10, 10, cToggleColor, screenWidth);

		if (!hideUI) {
			Color f1Color = showControls ? GRAY : WHITE;
			DrawTextRight("F1: Ayuda", 25, 10, f1Color, screenWidth);

			if (previewTimer > 0) {
				int panelSize = 140;
				int panelX = screenWidth - panelSize - 10;
				int panelY = screenHeight - panelSize - 10;

				DrawRectangle(panelX, panelY, panelSize, panelSize, Fade(BLACK, 0.5f));
				DrawText("Bloque Actual:", panelX + 10, panelY + 10, 10, WHITE);

				Block previewBlock = { 0 };

				previewBlock.color = blockColors[selectedColorIndex];
				previewBlock.shape = (BlockShape)selectedShapeIndex;

				float blockW = (previewBlock.shape == SHAPE_RECT) ? 80.0f : 40.0f;
				float blockH = 40.0f;
				float centerX = panelX + (panelSize / 2.0f) - (blockW / 2.0f);
				float centerY = panelY + (panelSize / 2.0f) - (blockH / 2.0f);

				previewBlock.rect = (Rectangle){ centerX, centerY, blockW, blockH };

				DrawBlockShape(previewBlock);
				DrawText(shapeNames[selectedShapeIndex], panelX + 10, panelY + panelSize - 20, 10, WHITE);
			}

			if (showStats) {
				DrawText("Version: 1.2.1 OFFICIAL RELEASE | Release vID 07.01.2026", 10, screenHeight - 60, 20, BLUE);
				DrawText(TextFormat("Desarrollado por AGUSTINSDFX | Build x86_64 - C - SDFX Engine - OpenGL Version: %s", glText) , 10, screenHeight - 35, 10, BLUE);
			}

			if (showDebug) {
				int activeBlocks = 0;
				for (int i = 1; i < MAX_BLOCKS; i++) if (blocks[i].active) activeBlocks++;

				DrawText(TextFormat("FPS: %i", GetFPS()), 10, 10, 20, GRAY);
				DrawText(TextFormat("Pos: [%.1f, %.1f]", player.position.x, player.position.y), 10, 35, 10, GRAY);
				DrawText(TextFormat("Bloques: %i", activeBlocks), 10, 50, 10, GRAY);

				DrawText(TextFormat("Color jug: %s", playerColorNames[playerColorIndex]), 10, 65, 10, GRAY);
				DrawText(TextFormat("Forma: %s", shapeNames[selectedShapeIndex]), 10, 80, 10, GRAY);
				DrawText(TextFormat("Color Bloque: %s", blockColorNames[selectedColorIndex]), 10, 95, 10, GRAY);
				DrawText(TextFormat("Camara: %s", cameraModeNames[currentCameraMode]), 10, 110, 10, GRAY);

				DrawText(TextFormat("Tiempo: %s", dayNightNames[isNight ? 1 : 0]), 10, 125, 10, GRAY);
				DrawText(TextFormat("Clima: %s", weatherNames[currentWeather]), 10, 140, 10, GRAY);

				DrawText(TextFormat("Music [%i/6]: %s", currentSongIndex + 1, songFiles[currentSongIndex]), 10, 155, 10, GRAY);
				DrawText(TextFormat("Player.png: %s", hasPlayerTexture ? "YES" : "NO"), 10, 170, 10, hasPlayerTexture ? GRAY : RED);
				DrawText(TextFormat("Cursor.png: %s", hasCursorTexture ? "YES" : "NO"), 10, 185, 10, hasCursorTexture ? GRAY : RED);
				DrawText(TextFormat("Gear [%d/7]: gear%d.png", currentGearIndex + 1, currentGearIndex + 1), 10, 200, 10, GRAY);
			}
		}

		if (showCheatUI) {
			int cw = 300;
			int ch = 100;
			int cx = (screenWidth - cw) / 2;
			int cy = (screenHeight - ch) / 2;

			DrawRectangle(cx, cy, cw, ch, Fade(BLACK, 0.9f));
			DrawRectangleLines(cx, cy, cw, ch, GREEN);

			DrawText("CHEAT CONSOLE (0-9)", cx + 10, cy + 10, 10, GREEN);

			DrawRectangle(cx + 10, cy + 30, cw - 20, 30, Fade(WHITE, 0.1f));
			DrawText(cheatBuffer, cx + 20, cy + 35, 20, WHITE);

			if (((int)(GetTime() * 2) % 2) == 0) {
				int txtSize = MeasureText(cheatBuffer, 20);
				DrawText("_", cx + 20 + txtSize, cy + 35, 20, GREEN);
			}

			int statusX = cx + 10;
			if (cheatFly) {
				DrawText("FLY", statusX, cy + 70, 10, YELLOW);
				statusX += MeasureText("FLY", 10) + 10;
			}
			if (cheatInfJump) {
				DrawText("INF JUMP", statusX, cy + 70, 10, YELLOW);
				statusX += MeasureText("INF JUMP", 10) + 10;
			}
			if (cheatNoClip) {
				DrawText("NOCLIP", statusX, cy + 70, 10, YELLOW);
			}

			DrawText("SHIFT+K to Clear", cx + cw - 110, cy + 85, 10, GRAY);
		}

		if (showConsole) {
			int cHeight = 20 * (CONSOLE_VISIBLE + 2);
			int cWidth = screenWidth;

			DrawRectangle(0, 0, cWidth, cHeight, Fade(BLACK, 0.85f));
			DrawRectangleLines(0, 0, cWidth, cHeight, Fade(WHITE, 0.3f));

			int btnWidth = 80;
			int btnHeight = 25;
			int btnSpacing = 10;
			int totalBtnWidth = (btnWidth * 2) + btnSpacing;
			int startBtnX = (cWidth / 2) - (totalBtnWidth / 2);
			int btnY = 5;

			Rectangle btnClear = { (float)startBtnX, (float)btnY, (float)btnWidth, (float)btnHeight };
			Rectangle btnHelp = { (float)(startBtnX + btnWidth + btnSpacing), (float)btnY, (float)btnWidth, (float)btnHeight };

			Vector2 mousePos = GetMousePosition();
			bool click = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

			int textWidthClear = MeasureText("CLEAR", 10);
			bool hoverClear = CheckCollisionPointRec(mousePos, btnClear);
			DrawRectangleRec(btnClear, hoverClear ? RED : GRAY);
			DrawRectangleLinesEx(btnClear, 1, WHITE);
			DrawText("CLEAR", (int)btnClear.x + ((btnWidth - textWidthClear) / 2), (int)btnClear.y + 7, 10, WHITE);
			if (hoverClear && click) {
				for (int i = 0; i < CONSOLE_HISTORY; i++) memset(consoleLog[i], 0, 128);
				consoleScroll = 0;
			}

			int textWidthHelp = MeasureText("HELP", 10);
			bool hoverHelp = CheckCollisionPointRec(mousePos, btnHelp);
			DrawRectangleRec(btnHelp, hoverHelp ? BLUE : GRAY);
			DrawRectangleLinesEx(btnHelp, 1, WHITE);
			DrawText("HELP", (int)btnHelp.x + ((btnWidth - textWidthHelp) / 2), (int)btnHelp.y + 7, 10, WHITE);

			if (hoverHelp && click) {
				AddConsoleLog("DOCS: https://github.com/agustinsdfx/Platform/blob/main/doc/CONSOLE.md");
			}

			int wheel = (int)GetMouseWheelMove();
			if (wheel != 0) {
				consoleScroll += wheel;
			}

			int maxScroll = CONSOLE_HISTORY - CONSOLE_VISIBLE;
			if (consoleScroll < 0) consoleScroll = 0;
			if (consoleScroll > maxScroll) consoleScroll = maxScroll;

			int scrollBarWidth = 15;
			int scrollTrackHeight = cHeight - 40;
			int scrollTrackY = 40;

			DrawRectangle(cWidth - scrollBarWidth, scrollTrackY, scrollBarWidth, scrollTrackHeight, Fade(GRAY, 0.3f));

			float scrollPercent = (float)consoleScroll / (float)maxScroll;
			float visualPercent = 1.0f - scrollPercent;

			int thumbHeight = 30;
			int availableTrack = scrollTrackHeight - thumbHeight;
			int thumbY = scrollTrackY + (int)(availableTrack * visualPercent);

			DrawRectangle(cWidth - scrollBarWidth + 2, thumbY, scrollBarWidth - 4, thumbHeight, LIGHTGRAY);

			int textStartY = 40;

			for (int i = 0; i < CONSOLE_VISIBLE; i++) {
				int lineIndex = (CONSOLE_HISTORY - 1) - consoleScroll - (CONSOLE_VISIBLE - 1 - i);

				if (lineIndex >= 0 && lineIndex < CONSOLE_HISTORY) {
					if (consoleLog[lineIndex][0] != '\0') {
						DrawText(consoleLog[lineIndex], 10, textStartY + (i * 20), 10, WHITE);
					}
				}
			}

			if (consoleScroll > 0) {
				DrawText("^ HISTORY ^", cWidth - 100, cHeight - 20, 10, YELLOW);
			}
		}

		if (gamePaused) {
			const char* pauseText = "GAME PAUSED";
			int fontSize = 50;
			int textW = MeasureText(pauseText, fontSize);
			int textX = screenWidth / 2 - textW / 2;
			int textY = screenHeight / 2 - fontSize / 2;

			DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.4f));
			DrawText(pauseText, textX, textY, fontSize, RED);
		}

		if (isStarting) {
			DrawRectangle(0, 0, screenWidth, screenHeight, (Color) { 15, 15, 15, 255 });

			const char* startText = "SDFX Engine - Initializing...";
			int fontS = 30;
			DrawText(startText, (screenWidth / 2) - (MeasureText(startText, fontS) / 2), (screenHeight / 2) - 20, fontS, WHITE);

			int bW = 400;
			DrawRectangle((screenWidth / 2) - (bW / 2), (screenHeight / 2) + 40, bW, 6, DARKGRAY);
			DrawRectangle((screenWidth / 2) - (bW / 2), (screenHeight / 2) + 40, (int)(bW * (startTimer / 3.0f)), 6, GREEN);

			DrawText("Loading textures and sounds...", (screenWidth / 2) - 70, (screenHeight / 2) + 60, 10, GRAY);
		}

		if (hasCursorTexture) {
			DrawTexture(cursorTexture, GetMouseX(), GetMouseY(), WHITE);
		}

		EndDrawing();
	}

	if (hasPlayerTexture) {
		UnloadTexture(playerTexture);
	}

	if (hasCursorTexture) {
		UnloadTexture(cursorTexture);
	}

	for (int i = 0; i < NUM_GEARS; i++) {
		if (gearTextures[i].id != 0) {
			UnloadTexture(gearTextures[i]);
		}
	}

	for (int i = 0; i < NUM_CUSTOM_BLOCKS; i++) {
		UnloadTexture(customBlockTextures[i]);
	}

	if (hasDeathSound) {
		UnloadSound(fxDeath);
	}

	for (int i = 0; i < SONG_COUNT; i++) {
		if (songs[i].stream.buffer != NULL) UnloadMusicStream(songs[i]);
	}
	CloseAudioDevice();
	CloseWindow();

	return 0;
}
