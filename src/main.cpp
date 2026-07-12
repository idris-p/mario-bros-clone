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

struct Goomba
{
    Rectangle body;
    Vector2 velocity;
    bool onGround;
    bool alive;
    bool spawned;
};

struct Coin
{
    Rectangle body;
    bool collected;
};

struct GameTextures
{
    Texture2D ground;
    Texture2D brick;
    Texture2D questionBlock;
    Texture2D stairBlock;
    Texture2D pipe;
    Texture2D coin;
    Texture2D player;
    Texture2D goomba;
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
// Level dimensions
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

    char tile = level[row][column];

    return
        tile == '#' ||
        tile == 'B' ||
        tile == 'Q' ||
        tile == 'S' ||
        tile == 'H' ||
        tile == 'h' ||
        tile == 'V' ||
        tile == 'v';
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

std::vector<Coin> FindCoinSpawns(
    const Level& level
)
{
    std::vector<Coin> coins;

    constexpr float coinWidth = 24.0f;
    constexpr float coinHeight = 32.0f;

    for (int row = 0; row < GetLevelRows(level); ++row)
    {
        for (
            int column = 0;
            column < GetLevelColumns(level);
            ++column
        )
        {
            if (level[row][column] != 'C')
            {
                continue;
            }

            float tileX =
                column * static_cast<float>(tileSize);

            float tileY =
                row * static_cast<float>(tileSize);

            coins.push_back({
                {
                    tileX +
                        (tileSize - coinWidth) / 2.0f,

                    tileY +
                        (tileSize - coinHeight) / 2.0f,

                    coinWidth,
                    coinHeight
                },
                false
            });
        }
    }

    return coins;
}

std::vector<Goomba> FindGoombaSpawns(
    const Level& level
)
{
    std::vector<Goomba> goombas;

    for (int row = 0; row < GetLevelRows(level); ++row)
    {
        for (
            int column = 0;
            column < GetLevelColumns(level);
            ++column
        )
        {
            if (level[row][column] != 'G')
            {
                continue;
            }

            Goomba goomba{
				{
					column * static_cast<float>(tileSize) + 4.0f,
					row * static_cast<float>(tileSize) + 8.0f,
					40.0f,
					40.0f
				},
				{-90.0f, 0.0f},
				false,
				true,
				false
			};

            goombas.push_back(goomba);
        }
    }

    return goombas;
}

// Collect Coins

int CollectCoins(
    const Player& player,
    std::vector<Coin>& coins
)
{
    int collectedThisFrame = 0;

    for (Coin& coin : coins)
    {
        if (
            coin.collected ||
            !CheckCollisionRecs(
                player.body,
                coin.body
            )
        )
        {
            continue;
        }

        coin.collected = true;
        ++collectedThisFrame;
    }

    return collectedThisFrame;
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

    leftColumn = std::max(0, leftColumn);

    rightColumn = std::min(
        GetLevelColumns(level) - 1,
        rightColumn
    );

    topRow = std::max(0, topRow);

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
// Player collision
// --------------------------------------------------

void ResolvePlayerHorizontalCollisions(
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

void ResolvePlayerVerticalCollisions(
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
// Goomba movement and collision
// --------------------------------------------------

void ResolveGoombaHorizontalCollisions(
    Goomba& goomba,
    const Level& level,
    float deltaTime
)
{
    float previousX = goomba.body.x;

    goomba.body.x +=
        goomba.velocity.x * deltaTime;

    std::vector<Rectangle> nearbyTiles =
        GetNearbySolidTiles(
            level,
            goomba.body
        );

    for (const Rectangle& tile : nearbyTiles)
    {
        if (!CheckCollisionRecs(goomba.body, tile))
        {
            continue;
        }

        if (goomba.velocity.x > 0.0f)
        {
            float previousRight =
                previousX +
                goomba.body.width;

            if (previousRight <= tile.x)
            {
                goomba.body.x =
                    tile.x -
                    goomba.body.width;

                goomba.velocity.x =
                    -std::abs(goomba.velocity.x);

                return;
            }
        }
        else if (goomba.velocity.x < 0.0f)
        {
            float tileRight =
                tile.x + tile.width;

            if (previousX >= tileRight)
            {
                goomba.body.x = tileRight;

                goomba.velocity.x =
                    std::abs(goomba.velocity.x);

                return;
            }
        }
    }

    float levelWidth =
        GetLevelWidth(level);

    if (goomba.body.x < 0.0f)
    {
        goomba.body.x = 0.0f;
        goomba.velocity.x =
            std::abs(goomba.velocity.x);
    }

    if (
        goomba.body.x +
            goomba.body.width >
        levelWidth
    )
    {
        goomba.body.x =
            levelWidth -
            goomba.body.width;

        goomba.velocity.x =
            -std::abs(goomba.velocity.x);
    }
}

void ResolveGoombaVerticalCollisions(
    Goomba& goomba,
    const Level& level,
    float deltaTime
)
{
    float previousY = goomba.body.y;

    goomba.body.y +=
        goomba.velocity.y * deltaTime;

    goomba.onGround = false;

    std::vector<Rectangle> nearbyTiles =
        GetNearbySolidTiles(
            level,
            goomba.body
        );

    for (const Rectangle& tile : nearbyTiles)
    {
        if (!CheckCollisionRecs(goomba.body, tile))
        {
            continue;
        }

        if (goomba.velocity.y > 0.0f)
        {
            float previousBottom =
                previousY +
                goomba.body.height;

            if (previousBottom <= tile.y)
            {
                goomba.body.y =
                    tile.y -
                    goomba.body.height;

                goomba.velocity.y = 0.0f;
                goomba.onGround = true;
            }
        }
        else if (goomba.velocity.y < 0.0f)
        {
            float tileBottom =
                tile.y + tile.height;

            if (previousY >= tileBottom)
            {
                goomba.body.y = tileBottom;
                goomba.velocity.y = 0.0f;
            }
        }
    }
}

void UpdateGoombas(
    std::vector<Goomba>& goombas,
    const Level& level,
    float deltaTime,
    float gravity
)
{
    for (Goomba& goomba : goombas)
    {
        if (
			!goomba.alive ||
			!goomba.spawned
		)
		{
			continue;
		}

        goomba.velocity.y +=
            gravity * deltaTime;

        ResolveGoombaHorizontalCollisions(
            goomba,
            level,
            deltaTime
        );

        ResolveGoombaVerticalCollisions(
            goomba,
            level,
            deltaTime
        );

        if (
            goomba.body.y >
            GetLevelHeight(level) + 200.0f
        )
        {
            goomba.alive = false;
        }
    }
}

void SpawnGoombasNearCamera(
    std::vector<Goomba>& goombas,
    const Camera2D& camera
)
{
    constexpr float spawnMargin = 96.0f;

    float halfVisibleWidth =
        virtualWidth / (2.0f * camera.zoom);

    float cameraRight =
        camera.target.x + halfVisibleWidth;

    for (Goomba& goomba : goombas)
    {
        if (
            goomba.spawned ||
            !goomba.alive
        )
        {
            continue;
        }

        // Spawn shortly before entering the screen.
        if (goomba.body.x <= cameraRight + spawnMargin)
        {
            goomba.spawned = true;
        }
    }
}

// --------------------------------------------------
// Player/Goomba interaction
// --------------------------------------------------

bool HandlePlayerGoombaCollisions(
    Player& player,
    std::vector<Goomba>& goombas,
    float previousPlayerBottom
)
{
    constexpr float stompTolerance = 12.0f;
    constexpr float stompBounceSpeed = 450.0f;

    for (Goomba& goomba : goombas)
    {
        if (
			!goomba.alive ||
			!goomba.spawned ||
			!CheckCollisionRecs(
				player.body,
				goomba.body
			)
		)
        {
            continue;
        }

        bool falling =
            player.velocity.y > 0.0f;

        bool wasAboveGoomba =
            previousPlayerBottom <=
            goomba.body.y +
                stompTolerance;

        if (falling && wasAboveGoomba)
        {
            goomba.alive = false;

            player.body.y =
                goomba.body.y -
                player.body.height;

            player.velocity.y =
                -stompBounceSpeed;

            player.onGround = false;

            return false;
        }

        // The player touched the Goomba from the
        // side or underneath.
        return true;
    }

    return false;
}

// --------------------------------------------------
// Drawing
// --------------------------------------------------

Rectangle GetFirstSquareFrame(const Texture2D& texture)
{
    // Handles both:
    // - a single square image
    // - a horizontal sprite sheet containing square frames
    float frameSize = static_cast<float>(
        std::min(texture.width, texture.height)
    );

    return {
        0.0f,
        0.0f,
        frameSize,
        frameSize
    };
}

void DrawTextureInsideRectangle(
    const Texture2D& texture,
    Rectangle source,
    const Rectangle& destination,
    bool flipHorizontally = false
)
{
    if (flipHorizontally)
    {
        source.x += source.width;
        source.width = -source.width;
    }

    DrawTexturePro(
        texture,
        source,
        destination,
        {0.0f, 0.0f},
        0.0f,
        WHITE
    );
}

void DrawTextureAsTile(
    const Texture2D& texture,
    const Rectangle& destination
)
{
    Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(texture.width),
        static_cast<float>(texture.height)
    };

    DrawTexturePro(
        texture,
        source,
        destination,
        {0.0f, 0.0f},
        0.0f,
        WHITE
    );
}

void DrawLevel(
    const Level& level,
    const GameTextures& textures
)
{
    for (int row = 0; row < GetLevelRows(level); ++row)
    {
        for (
            int column = 0;
            column < GetLevelColumns(level);
            ++column
        )
        {
            char tile = level[row][column];

            Rectangle destination =
                GetTileRectangle(row, column);

            switch (tile)
            {
                case '#':
                    DrawTextureAsTile(
                        textures.ground,
                        destination
                    );
                    break;

                case 'B':
                    DrawTextureAsTile(
                        textures.brick,
                        destination
                    );
                    break;

                case 'Q':
                    DrawTextureAsTile(
                        textures.questionBlock,
                        destination
                    );
                    break;

                case 'S':
                    DrawTextureAsTile(
                        textures.stairBlock,
                        destination
                    );
                    break;

                case 'H':
                {
                    // Draw the complete pipe once from
                    // its top-left marker.
                    Rectangle pipeDestination{
                        destination.x,
                        destination.y,
                        tileSize * 2.0f,
                        tileSize * 2.0f
                    };

                    DrawTextureAsTile(
                        textures.pipe,
                        pipeDestination
                    );

                    break;
                }

                // These form the remaining solid pipe
                // collision cells. The image is drawn by H.
                case 'h':
                case 'V':
                case 'v':
                    break;

                default:
                    break;
            }
        }
    }
}

void DrawPlayer(
    const Player& player,
    const Texture2D& texture
)
{
    Rectangle source{
        0.0f,
        0.0f,
        12.0f,
        16.0f
    };

    constexpr float spriteHeight = 48.0f;

    constexpr float spriteWidth =
        spriteHeight * (12.0f / 16.0f);

    Rectangle destination{
        player.body.x +
            (player.body.width - spriteWidth) / 2.0f,

        player.body.y +
            player.body.height -
            spriteHeight,

        spriteWidth,
        spriteHeight
    };

    bool facingLeft =
        player.velocity.x < -1.0f;

    DrawTextureInsideRectangle(
        texture,
        source,
        destination,
        facingLeft
    );
}

void DrawCoins(
    const std::vector<Coin>& coins,
    const Texture2D& texture
)
{
    Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(texture.width),
        static_cast<float>(texture.height)
    };

    for (const Coin& coin : coins)
    {
        if (coin.collected)
        {
            continue;
        }

        DrawTexturePro(
            texture,
            source,
            coin.body,
            {0.0f, 0.0f},
            0.0f,
            WHITE
        );
    }
}

void DrawGoomba(
    const Goomba& goomba,
    const Texture2D& texture
)
{
    if (
        !goomba.alive ||
        !goomba.spawned
    )
    {
        return;
    }

    Rectangle source =
        GetFirstSquareFrame(texture);

    Rectangle destination{
        goomba.body.x - 4.0f,
        goomba.body.y +
            goomba.body.height -
            static_cast<float>(tileSize),

        static_cast<float>(tileSize),
        static_cast<float>(tileSize)
    };

    bool movingRight =
        goomba.velocity.x > 0.0f;

    DrawTextureInsideRectangle(
        texture,
        source,
        destination,
        movingRight
    );
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

    float desiredTargetX =
        player.body.x +
        player.body.width / 2.0f;

    desiredTargetX = std::clamp(
        desiredTargetX,
        halfVisibleWidth,
        levelWidth - halfVisibleWidth
    );

    // Camera may only move to the right.
    desiredTargetX = std::max(
        desiredTargetX,
        camera.target.x
    );

    camera.target.x = desiredTargetX;

    camera.target.y =
        virtualHeight / 2.0f;

    camera.offset = {
        virtualWidth / 2.0f,
        virtualHeight / 2.0f
    };
}

// --------------------------------------------------
// Virtual resolution scaling
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
// Respawn helper
// --------------------------------------------------

void RespawnPlayer(
    Player& player,
    const Vector2& spawn,
    Camera2D& camera,
    std::vector<Coin>& coins,
    const std::vector<Coin>& initialCoins,
    int& coinCount,
    std::vector<Goomba>& goombas,
    const std::vector<Goomba>& initialGoombas,
    float& jumpBufferTimer,
    float& coyoteTimer
)
{
    player.body = {
        spawn.x + 8.0f,
        spawn.y + 4.0f,
        32.0f,
        44.0f
    };

    player.velocity = {0.0f, 0.0f};
    player.onGround = false;

    jumpBufferTimer = 0.0f;
    coyoteTimer = 0.0f;

    camera.target = {
        virtualWidth / 2.0f,
        virtualHeight / 2.0f
    };

    coins = initialCoins;
    coinCount = 0;

    goombas = initialGoombas;
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
        "........................................................................................................................",
        "........................................................................................................................",
        ".................................................C.C.C..................................................................",
        "....................BBBBQBBBB.......................................................C....................................",
        "..........................................C.C.C.................................................BBBB.....................",
        ".........Q...B...Q......................................................................................................",
        "..............................................................C.C.C.....................................................",
        "............................BBBBB..................................................QBBBBQ.................................",
        "........................................................................................................................",
        ".P.........C.C.C....................Hh.........................G......................................Hh................",
        "........BBBBQBBBB...................Vv...............................................SSS..............Vv....G...........",
        "##################..############################..##############################....SSSS..........#######################",
        "##################..############################..##############################...SSSSS..........#######################",
        "##################..############################..##############################...SSSSS..........#######################",
        "##################..############################..##############################...SSSSS..........#######################"
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

    GameTextures textures{
        LoadTexture("resources/Blocks/Ground.png"),
        LoadTexture("resources/Blocks/Brick.png"),
        LoadTexture("resources/Blocks/Q_Block.png"),
        LoadTexture("resources/Blocks/Stair_Block.png"),
        LoadTexture("resources/Pipe.png"),
        LoadTexture("resources/Coin.png"),
        LoadTexture("resources/Mario/Small_Mario.png"),
        LoadTexture("resources/Enemies/Goomba.png")
    };

    SetTextureFilter(
        textures.ground,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.brick,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.questionBlock,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.stairBlock,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.pipe,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.coin,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.player,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.goomba,
        TEXTURE_FILTER_POINT
    );

    if (
        textures.ground.id == 0 ||
        textures.player.id == 0 ||
        textures.goomba.id == 0
    )
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load one or more game textures."
        );

        UnloadTexture(textures.ground);
        UnloadTexture(textures.player);
        UnloadTexture(textures.goomba);

        CloseWindow();
        return 1;
    }

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
            spawn.x + 8.0f,
            spawn.y + 4.0f,
            32.0f,
            44.0f
        },
        {0.0f, 0.0f},
        false
    };

    std::vector<Coin> initialCoins =
        FindCoinSpawns(level);

    std::vector<Coin> coins =
        initialCoins;

    int coinCount = 0;

    std::vector<Goomba> initialGoombas =
        FindGoombaSpawns(level);

    std::vector<Goomba> goombas =
        initialGoombas;

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
            std::min(
                GetFrameTime(),
                0.05f
            );

        bool wasOnGround =
            player.onGround;

        float previousPlayerBottom =
            player.body.y +
            player.body.height;

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
        // Jump buffer
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

        // Collect Coins

        coinCount += CollectCoins(
            player,
            coins
        );

        // --------------------------------
        // Player physics
        // --------------------------------

        player.velocity.y +=
            gravity * deltaTime;

        ResolvePlayerHorizontalCollisions(
            player,
            level,
            deltaTime
        );

        ResolvePlayerVerticalCollisions(
            player,
            level,
            deltaTime
        );

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
        // Enemy physics
        // --------------------------------

		SpawnGoombasNearCamera(
			goombas,
			camera
		);
		
        UpdateGoombas(
            goombas,
            level,
            deltaTime,
            gravity
        );

        bool playerHitGoomba =
            HandlePlayerGoombaCollisions(
                player,
                goombas,
                previousPlayerBottom
            );

        // --------------------------------
        // Respawn
        // --------------------------------

        bool playerFell =
            player.body.y >
            GetLevelHeight(level) +
                200.0f;

        if (playerFell || playerHitGoomba)
        {
            RespawnPlayer(
                player,
                spawn,
                camera,
                coins,
                initialCoins,
                coinCount,
                goombas,
                initialGoombas,
                jumpBufferTimer,
                coyoteTimer
            );
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
            virtualWidth /
                (2.0f * camera.zoom) +
            5.0f;

        if (player.body.x < cameraLeft)
        {
            player.body.x = cameraLeft;
            player.velocity.x = 0.0f;
        }

        // Remove enemies left behind by the camera.
        for (Goomba& goomba : goombas)
        {
            if (
                goomba.alive &&
                goomba.body.x +
                    goomba.body.width <
                cameraLeft
            )
            {
                goomba.alive = false;
            }
        }

        // --------------------------------
        // Draw virtual game screen
        // --------------------------------

        BeginTextureMode(gameTexture);

        ClearBackground(SKYBLUE);

        BeginMode2D(camera);

        DrawLevel(
            level,
            textures
        );

        DrawCoins(
            coins,
            textures.coin
        );

        for (const Goomba& goomba : goombas)
        {
            DrawGoomba(
                goomba,
                textures.goomba
            );
        }

        DrawPlayer(
            player,
            textures.player
        );

        if (IsKeyDown(KEY_F1))
        {
            DrawRectangleLinesEx(
                player.body,
                2.0f,
                RED
            );

            for (const Goomba& goomba : goombas)
            {
                if (goomba.alive && goomba.spawned)
                {
                    DrawRectangleLinesEx(
                        goomba.body,
                        2.0f,
                        RED
                    );
                }
            }
        }

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
                "Coins: %d",
                coinCount
            ),
            20,
            100,
            18,
            BLACK
        );

        DrawText(
            TextFormat(
                "Goombas remaining: %d",
                static_cast<int>(
                    std::count_if(
                        goombas.begin(),
                        goombas.end(),
                        [](const Goomba& goomba)
                        {
                            return goomba.alive;
                        }
                    )
                )
            ),
            20,
            100,
            18,
            BLACK
        );

        EndTextureMode();

        // --------------------------------
        // Scale virtual screen to window
        // --------------------------------

        BeginDrawing();

        ClearBackground(BLACK);

        DrawRenderTextureToWindow(
            gameTexture
        );

        EndDrawing();
    }

    UnloadTexture(textures.ground);
    UnloadTexture(textures.brick);
    UnloadTexture(textures.questionBlock);
    UnloadTexture(textures.stairBlock);
    UnloadTexture(textures.pipe);
    UnloadTexture(textures.coin);
    UnloadTexture(textures.player);
    UnloadTexture(textures.goomba);

    UnloadRenderTexture(gameTexture);
    CloseWindow();

    return 0;
}