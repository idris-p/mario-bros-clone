#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

struct Player
{
    Rectangle body;
    Vector2 velocity;
    bool onGround;
};

using Level = std::vector<std::string>;

// --------------------------------------------------
// Window and rendering
// --------------------------------------------------

constexpr int initialWindowWidth = 1600;
constexpr int initialWindowHeight = 900;

constexpr int tileSize = 48;

constexpr int visibleTilesAcross = 24;
constexpr float visibleTilesHigh = 13.5f;

constexpr int virtualWidth =
    visibleTilesAcross * tileSize;

constexpr int virtualHeight =
    static_cast<int>(
        visibleTilesHigh * tileSize
    );

constexpr float cameraZoom = 1.0f;

// --------------------------------------------------
// Level
// --------------------------------------------------

int GetLevelRows(const Level& level)
{
    return static_cast<int>(level.size());
}

int GetLevelColumns(const Level& level)
{
    if (level.empty())
    {
        return 0;
    }

    return static_cast<int>(level.front().size());
}

float GetLevelWidth(const Level& level)
{
    return static_cast<float>(
        GetLevelColumns(level) * tileSize
    );
}

float GetLevelHeight(const Level& level)
{
    return static_cast<float>(
        GetLevelRows(level) * tileSize
    );
}

// Makes every row the same width by adding empty tiles.
void NormaliseLevel(Level& level)
{
    std::size_t widestRow = 0;

    for (const std::string& row : level)
    {
        widestRow = std::max(
            widestRow,
            row.size()
        );
    }

    for (std::string& row : level)
    {
        row.resize(widestRow, '.');
    }
}

// --------------------------------------------------
// Tile helpers
// --------------------------------------------------

bool IsSolidTile(
    const Level& level,
    int row,
    int column
)
{
    if (
        row < 0 ||
        row >= GetLevelRows(level) ||
        column < 0 ||
        column >= GetLevelColumns(level)
    )
    {
        return false;
    }

    return level[row][column] == '#';
}

Rectangle GetTileRectangle(
    int row,
    int column
)
{
    return {
        static_cast<float>(column * tileSize),
        static_cast<float>(row * tileSize),
        static_cast<float>(tileSize),
        static_cast<float>(tileSize)
    };
}

Vector2 FindPlayerSpawn(const Level& level)
{
    for (int row = 0; row < GetLevelRows(level); ++row)
    {
        for (
            int column = 0;
            column < GetLevelColumns(level);
            ++column
        )
        {
            if (level[row][column] == 'P')
            {
                return {
                    static_cast<float>(
                        column * tileSize
                    ),
                    static_cast<float>(
                        row * tileSize
                    )
                };
            }
        }
    }

    return {100.0f, 300.0f};
}

// --------------------------------------------------
// Nearby tile lookup
// --------------------------------------------------

std::vector<Rectangle> GetNearbySolidTiles(
    const Level& level,
    const Rectangle& body
)
{
    std::vector<Rectangle> nearbyTiles;

    int leftColumn =
        static_cast<int>(
            std::floor(body.x / tileSize)
        ) - 1;

    int rightColumn =
        static_cast<int>(
            std::floor(
                (body.x + body.width) /
                tileSize
            )
        ) + 1;

    int topRow =
        static_cast<int>(
            std::floor(body.y / tileSize)
        ) - 1;

    int bottomRow =
        static_cast<int>(
            std::floor(
                (body.y + body.height) /
                tileSize
            )
        ) + 1;

    leftColumn = std::max(
        0,
        leftColumn
    );

    rightColumn = std::min(
        GetLevelColumns(level) - 1,
        rightColumn
    );

    topRow = std::max(
        0,
        topRow
    );

    bottomRow = std::min(
        GetLevelRows(level) - 1,
        bottomRow
    );

    for (
        int row = topRow;
        row <= bottomRow;
        ++row
    )
    {
        for (
            int column = leftColumn;
            column <= rightColumn;
            ++column
        )
        {
            if (IsSolidTile(level, row, column))
            {
                nearbyTiles.push_back(
                    GetTileRectangle(
                        row,
                        column
                    )
                );
            }
        }
    }

    return nearbyTiles;
}

// --------------------------------------------------
// Collision
// --------------------------------------------------

void ResolveHorizontalCollisions(
    Player& player,
    const Level& level,
    float deltaTime
)
{
    float previousX = player.body.x;

    player.body.x +=
        player.velocity.x * deltaTime;

    std::vector<Rectangle> nearbyTiles =
        GetNearbySolidTiles(
            level,
            player.body
        );

    for (const Rectangle& tile : nearbyTiles)
    {
        if (!CheckCollisionRecs(player.body, tile))
        {
            continue;
        }

        // Moving right.
        if (player.velocity.x > 0.0f)
        {
            float previousRight =
                previousX +
                player.body.width;

            if (previousRight <= tile.x)
            {
                player.body.x =
                    tile.x -
                    player.body.width;

                player.velocity.x = 0.0f;
            }
        }
        // Moving left.
        else if (player.velocity.x < 0.0f)
        {
            float tileRight =
                tile.x + tile.width;

            if (previousX >= tileRight)
            {
                player.body.x = tileRight;
                player.velocity.x = 0.0f;
            }
        }
    }

    float levelWidth =
        GetLevelWidth(level);

    if (player.body.x < 0.0f)
    {
        player.body.x = 0.0f;
        player.velocity.x = 0.0f;
    }

    if (
        player.body.x +
            player.body.width >
        levelWidth
    )
    {
        player.body.x =
            levelWidth -
            player.body.width;

        player.velocity.x = 0.0f;
    }
}

void ResolveVerticalCollisions(
    Player& player,
    const Level& level,
    float deltaTime
)
{
    float previousY = player.body.y;

    player.body.y +=
        player.velocity.y * deltaTime;

    player.onGround = false;

    std::vector<Rectangle> nearbyTiles =
        GetNearbySolidTiles(
            level,
            player.body
        );

    for (const Rectangle& tile : nearbyTiles)
    {
        if (!CheckCollisionRecs(player.body, tile))
        {
            continue;
        }

        // Falling: land on the top.
        if (player.velocity.y > 0.0f)
        {
            float previousBottom =
                previousY +
                player.body.height;

            if (previousBottom <= tile.y)
            {
                player.body.y =
                    tile.y -
                    player.body.height;

                player.velocity.y = 0.0f;
                player.onGround = true;
            }
        }
        // Rising: hit the underside.
        else if (player.velocity.y < 0.0f)
        {
            float tileBottom =
                tile.y + tile.height;

            if (previousY >= tileBottom)
            {
                player.body.y = tileBottom;
                player.velocity.y = 0.0f;
            }
        }
    }
}

// --------------------------------------------------
// Drawing
// --------------------------------------------------

void DrawLevel(const Level& level)
{
    for (int row = 0; row < GetLevelRows(level); ++row)
    {
        for (
            int column = 0;
            column < GetLevelColumns(level);
            ++column
        )
        {
            if (!IsSolidTile(level, row, column))
            {
                continue;
            }

            Rectangle tile =
                GetTileRectangle(
                    row,
                    column
                );

            DrawRectangleRec(
                tile,
                DARKBROWN
            );

            DrawRectangleLinesEx(
                tile,
                2.0f,
                BLACK
            );
        }
    }
}

// --------------------------------------------------
// Camera
// --------------------------------------------------

void UpdateCamera(
    Camera2D& camera,
    const Player& player,
    const Level& level
)
{
    const float levelWidth =
        GetLevelWidth(level);

    const float visibleWorldWidth =
        virtualWidth / camera.zoom;

    const float halfVisibleWidth =
        visibleWorldWidth / 2.0f;

    // Keep the player around the horizontal centre
    // once the camera begins scrolling.
    float desiredTargetX =
        player.body.x +
        player.body.width / 2.0f;

    // Prevent showing space outside the left or right
    // edges of the level.
    desiredTargetX = std::clamp(
        desiredTargetX,
        halfVisibleWidth,
        levelWidth - halfVisibleWidth
    );

    // The camera is only allowed to move right.
    desiredTargetX = std::max(
        desiredTargetX,
        camera.target.x
    );

    camera.target.x = desiredTargetX;

    // Never move vertically.
    //
    // target.y = virtualHeight / 2 means:
    // top of view    = 0
    // bottom of view = 648
    //
    // 648 / 48 = 13.5 tiles.
    camera.target.y =
        virtualHeight / 2.0f;

    camera.offset = {
        virtualWidth / 2.0f,
        virtualHeight / 2.0f
    };
}

// --------------------------------------------------
// Scale virtual resolution to real window
// --------------------------------------------------

void DrawRenderTextureToWindow(
    const RenderTexture2D& renderTexture
)
{
    float scale = std::min(
        static_cast<float>(
            GetScreenWidth()
        ) / virtualWidth,

        static_cast<float>(
            GetScreenHeight()
        ) / virtualHeight
    );

    float destinationWidth =
        virtualWidth * scale;

    float destinationHeight =
        virtualHeight * scale;

    float offsetX =
        (
            GetScreenWidth() -
            destinationWidth
        ) / 2.0f;

    float offsetY =
        (
            GetScreenHeight() -
            destinationHeight
        ) / 2.0f;

    Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(
            renderTexture.texture.width
        ),
        -static_cast<float>(
            renderTexture.texture.height
        )
    };

    Rectangle destination{
        offsetX,
        offsetY,
        destinationWidth,
        destinationHeight
    };

    DrawTexturePro(
        renderTexture.texture,
        source,
        destination,
        {0.0f, 0.0f},
        0.0f,
        WHITE
    );
}

// --------------------------------------------------
// Main
// --------------------------------------------------

int main()
{
    constexpr float walkSpeed = 280.0f;
    constexpr float sprintSpeed = 450.0f;

    constexpr float walkingAcceleration = 1400.0f;
    constexpr float sprintAcceleration = 1900.0f;
    constexpr float friction = 1800.0f;

    constexpr float jumpSpeed = 850.0f;
    constexpr float gravity = 1800.0f;

    constexpr float jumpBufferDuration = 0.12f;
    constexpr float coyoteTimeDuration = 0.06f;

    Level level{
        "........................................................................................................................",
        ".....#####..............................................................................................................",
        "........................####......................................................#####...................................",
        "..............................................#####.....................................................................",
        "...###..............................###.....................................###...........................................",
        "..............####........................................####...........................................................",
        ".................................................................#####..................................................",
        ".........####....................#####.................................................####..............................",
        "............................###............................###............................................................",
        "........................................#####....................................................#####..................",
        ".P..................###...............................................###................................................",
        "........................................................................................................................",
        "######..###################..#####################..############..#######################..####################..########",
        "######..###################..#####################..############..#######################..####################..########",
        "######..###################..#####################..############..#######################..####################..########",
        "######..###################..#####################..############..#######################..####################..########"
    };

    NormaliseLevel(level);

    if (
        level.empty() ||
        GetLevelColumns(level) == 0
    )
    {
        TraceLog(
            LOG_ERROR,
            "The level cannot be empty."
        );

        return 1;
    }

    SetConfigFlags(
        FLAG_WINDOW_RESIZABLE |
        FLAG_VSYNC_HINT
    );

    InitWindow(
        initialWindowWidth,
        initialWindowHeight,
        "Super Mario Bros. C++"
    );

    SetWindowMinSize(640, 360);
    SetTargetFPS(60);

    RenderTexture2D gameTexture =
        LoadRenderTexture(
            virtualWidth,
            virtualHeight
        );

    SetTextureFilter(
        gameTexture.texture,
        TEXTURE_FILTER_POINT
    );

    Vector2 spawn =
        FindPlayerSpawn(level);

    Player player{
        {
            spawn.x + 4.0f,
            spawn.y - 2.0f,
            40.0f,
            50.0f
        },
        {0.0f, 0.0f},
        false
    };

    Camera2D camera{};

    camera.target = {
		virtualWidth / 2.0f,
		virtualHeight / 2.0f
	};

    camera.offset = {
        virtualWidth / 2.0f,
        virtualHeight / 2.0f
    };

    camera.rotation = 0.0f;
    camera.zoom = cameraZoom;

    float jumpBufferTimer = 0.0f;
    float coyoteTimer = 0.0f;

    while (!WindowShouldClose())
    {
        float deltaTime =
            GetFrameTime();

        deltaTime = std::min(
            deltaTime,
            0.05f
        );

        bool wasOnGround =
            player.onGround;

        // --------------------------------
        // Input
        // --------------------------------

        float direction = 0.0f;

        if (
            IsKeyDown(KEY_A) ||
            IsKeyDown(KEY_LEFT)
        )
        {
            direction -= 1.0f;
        }

        if (
            IsKeyDown(KEY_D) ||
            IsKeyDown(KEY_RIGHT)
        )
        {
            direction += 1.0f;
        }

        bool sprinting =
            IsKeyDown(KEY_LEFT_SHIFT) ||
            IsKeyDown(KEY_RIGHT_SHIFT);

        float currentMaxSpeed =
            sprinting
                ? sprintSpeed
                : walkSpeed;

        float currentAcceleration =
            sprinting
                ? sprintAcceleration
                : walkingAcceleration;

        // --------------------------------
        // Jump buffering
        // --------------------------------

        if (
            IsKeyPressed(KEY_SPACE) ||
            IsKeyPressed(KEY_W)
        )
        {
            jumpBufferTimer =
                jumpBufferDuration;
        }
        else
        {
            jumpBufferTimer =
                std::max(
                    0.0f,
                    jumpBufferTimer -
                        deltaTime
                );
        }

        // --------------------------------
        // Coyote time
        // --------------------------------

        if (wasOnGround)
        {
            coyoteTimer =
                coyoteTimeDuration;
        }
        else
        {
            coyoteTimer =
                std::max(
                    0.0f,
                    coyoteTimer -
                        deltaTime
                );
        }

        // --------------------------------
        // Horizontal movement
        // --------------------------------

        if (direction != 0.0f)
        {
            player.velocity.x +=
                direction *
                currentAcceleration *
                deltaTime;

            player.velocity.x =
                std::clamp(
                    player.velocity.x,
                    -currentMaxSpeed,
                    currentMaxSpeed
                );
        }
        else
        {
            if (player.velocity.x > 0.0f)
            {
                player.velocity.x =
                    std::max(
                        0.0f,
                        player.velocity.x -
                            friction *
                            deltaTime
                    );
            }
            else if (player.velocity.x < 0.0f)
            {
                player.velocity.x =
                    std::min(
                        0.0f,
                        player.velocity.x +
                            friction *
                            deltaTime
                    );
            }
        }

        // Reduce sprint speed gradually when
        // the sprint button is released.
        if (!sprinting)
        {
            if (player.velocity.x > walkSpeed)
            {
                player.velocity.x =
                    std::max(
                        walkSpeed,
                        player.velocity.x -
                            friction *
                            deltaTime
                    );
            }
            else if (
                player.velocity.x <
                -walkSpeed
            )
            {
                player.velocity.x =
                    std::min(
                        -walkSpeed,
                        player.velocity.x +
                            friction *
                            deltaTime
                    );
            }
        }

        // --------------------------------
        // Jump
        // --------------------------------

        if (
            jumpBufferTimer > 0.0f &&
            coyoteTimer > 0.0f
        )
        {
            player.velocity.y =
                -jumpSpeed;

            player.onGround = false;

            jumpBufferTimer = 0.0f;
            coyoteTimer = 0.0f;
        }

        // --------------------------------
        // Physics
        // --------------------------------

        player.velocity.y +=
            gravity * deltaTime;

        ResolveHorizontalCollisions(
            player,
            level,
            deltaTime
        );

        ResolveVerticalCollisions(
            player,
            level,
            deltaTime
        );

        // Buffered jump immediately after landing.
        if (
            player.onGround &&
            jumpBufferTimer > 0.0f
        )
        {
            player.velocity.y =
                -jumpSpeed;

            player.onGround = false;

            jumpBufferTimer = 0.0f;
            coyoteTimer = 0.0f;
        }

        // --------------------------------
        // Respawn
        // --------------------------------

        if (
            player.body.y >
            GetLevelHeight(level) +
                200.0f
        )
        {
            player.body = {
                spawn.x + 4.0f,
                spawn.y - 2.0f,
                40.0f,
                50.0f
            };

            player.velocity =
                {0.0f, 0.0f};

            player.onGround = false;

            jumpBufferTimer = 0.0f;
            coyoteTimer = 0.0f;

            camera.target = {
				virtualWidth / 2.0f,
				virtualHeight / 2.0f
			};
        }

        // --------------------------------
        // Camera
        // --------------------------------

        UpdateCamera(
            camera,
            player,
            level
        );

		float cameraLeft =
			camera.target.x -
			virtualWidth / (2.0f * camera.zoom)
			+ 5.0f;

		if (player.body.x < cameraLeft)
		{
			player.body.x = cameraLeft;
			player.velocity.x = 0.0f;
		}

        // --------------------------------
        // Draw to virtual resolution
        // --------------------------------

        BeginTextureMode(gameTexture);

        ClearBackground(SKYBLUE);

        BeginMode2D(camera);

        DrawLevel(level);

        DrawRectangleRec(
            player.body,
            RED
        );

        DrawRectangleLinesEx(
            player.body,
            2.0f,
            BLACK
        );

        EndMode2D();

        DrawRectangle(
            10,
            10,
            300,
            125,
            Fade(RAYWHITE, 0.8f)
        );

        DrawText(
            "A/D or arrows: move",
            20,
            20,
            20,
            BLACK
        );

        DrawText(
            "Shift: sprint",
            20,
            45,
            20,
            BLACK
        );

        DrawText(
            "Space or W: jump",
            20,
            70,
            20,
            BLACK
        );

        DrawText(
            TextFormat(
                "Position: %.0f, %.0f",
                player.body.x,
                player.body.y
            ),
            20,
            100,
            18,
            BLACK
        );

        EndTextureMode();

        // --------------------------------
        // Scale virtual resolution
        // --------------------------------

        BeginDrawing();

        ClearBackground(BLACK);

        DrawRenderTextureToWindow(
            gameTexture
        );

        EndDrawing();
    }

    UnloadRenderTexture(gameTexture);
    CloseWindow();

    return 0;
}