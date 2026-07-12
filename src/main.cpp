#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

// For erase_if (C++20)
namespace std {
    template<typename Container, typename Predicate>
    typename Container::size_type erase_if(Container& c, Predicate pred) {
        auto it = std::remove_if(c.begin(), c.end(), pred);
        auto count = std::distance(it, c.end());
        c.erase(it, c.end());
        return count;
    }
}

struct Player
{
    Rectangle body;
    Vector2 velocity;
    bool onGround;
    bool big;
    bool fire;
    float invulnerabilityTimer;
    bool sprinting;
    bool facingLeft;
    bool dying;
    bool ducking;
    float walkAnimationTimer;
    int walkAnimationFrame;
    float fireThrowTimer;
    bool skidding;
};

struct SuperMushroom
{
    Rectangle body;
    Vector2 velocity;
    float targetY;
    bool emerging;
    bool collected;
    bool oneUp;
};

struct FireFlower
{
    Rectangle body;
    float targetY;
    bool emerging;
    bool collected;
};

struct FireBall
{
    Rectangle body;
    Vector2 velocity;
    bool alive;
    float rotation;
};

struct Goomba
{
    Rectangle body;
    Vector2 velocity;
    bool onGround;
    bool alive;
    bool spawned;
    bool dying;
};

struct Koopa
{
    Rectangle body;
    Vector2 velocity;
    bool onGround;
    bool alive;
    bool spawned;
    bool shelled;
    bool dying;
};

void KillGoomba(Goomba& goomba)
{
    if (goomba.alive && !goomba.dying)
    {
        goomba.dying = true;
        goomba.velocity = {0.0f, -240.0f};
    }
}

void KillKoopa(Koopa& koopa)
{
    if (koopa.alive && !koopa.dying)
    {
        koopa.dying = true;
        koopa.velocity = {0.0f, -240.0f};
    }
}

void KillPlayer(Player& player)
{
    if (!player.dying)
    {
        player.dying = true;
        player.velocity = {0.0f, -650.0f};
        player.onGround = false;
        player.sprinting = false;
        player.ducking = false;
        player.fireThrowTimer = 0.0f;
        player.skidding = false;
    }
}

struct Coin
{
    Rectangle body;
    bool collected;
};

struct BlockCoin
{
    Rectangle body;
    float blockTopY;
    float age;
};

struct BlockBump
{
    int row;
    int column;
    float age;
    float offsetY;
    float previousOffsetY;
};

std::vector<BlockBump> blockBumps;

struct GameTextures
{
    Texture2D ground;
    Texture2D brick;
    Texture2D questionBlock;
    Texture2D emptyBlock;
    Texture2D stairBlock;
    Texture2D pipe;
    Texture2D pipeBase;
    Texture2D coin;
    Texture2D player;
    Texture2D smallMarioWalk1;
    Texture2D smallMarioWalk2;
    Texture2D smallMarioWalk3;
    Texture2D smallMarioJump;
    Texture2D smallMarioSwitch;
    Texture2D marioDead;
    Texture2D superMario;
    Texture2D superMarioDuck;
    Texture2D superMarioWalk1;
    Texture2D superMarioWalk2;
    Texture2D superMarioWalk3;
    Texture2D superMarioJump;
    Texture2D superMarioSwitch;
    Texture2D fireMario;
    Texture2D fireMarioDuck;
    Texture2D fireMarioWalk1;
    Texture2D fireMarioWalk2;
    Texture2D fireMarioWalk3;
    Texture2D fireMarioJump;
    Texture2D fireMarioSwitch;
    Texture2D fireMarioThrow;
    Texture2D goomba;
    Texture2D koopa;
    Texture2D greenShell;
    Texture2D superMushroom;
    Texture2D oneUpMushroom;
    Texture2D fireFlower;
    Texture2D fireBall;
    Texture2D flag;
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
        tile == 'M' ||
        tile == 'A' ||
        tile == 'E' ||
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
    float offsetY = 0.0f;

    for (const BlockBump& bump : blockBumps)
    {
        if (bump.row == row && bump.column == column)
        {
            offsetY = bump.offsetY;
            break;
        }
    }

    return {
        static_cast<float>(column * tileSize),
        static_cast<float>(row * tileSize) + offsetY,
        static_cast<float>(tileSize),
        static_cast<float>(tileSize)
    };
}

void StartBlockBump(int row, int column)
{
    for (const BlockBump& bump : blockBumps)
    {
        if (bump.row == row && bump.column == column)
        {
            return;
        }
    }

    blockBumps.push_back({row, column, 0.0f, 0.0f, 0.0f});
}

void UpdateBlockBumps(
    float deltaTime,
    std::vector<SuperMushroom>& mushrooms,
    std::vector<FireFlower>& fireFlowers,
    std::vector<Goomba>& goombas,
    std::vector<Koopa>& koopas
)
{
    constexpr float bumpDuration = 0.22f;
    constexpr float bumpHeight = 10.0f;

    for (BlockBump& bump : blockBumps)
    {
        bump.previousOffsetY = bump.offsetY;
        bump.age += deltaTime;

        float progress = std::clamp(
            bump.age / bumpDuration,
            0.0f,
            1.0f
        );

        bump.offsetY =
            -std::sin(progress * PI) * bumpHeight;

        float movementY =
            bump.offsetY - bump.previousOffsetY;

        Rectangle previousBlock{
            static_cast<float>(bump.column * tileSize),
            static_cast<float>(bump.row * tileSize) +
                bump.previousOffsetY,
            static_cast<float>(tileSize),
            static_cast<float>(tileSize)
        };

        auto carryBody =
            [&previousBlock, movementY](Rectangle& body)
            {
                constexpr float footTolerance = 4.0f;

                bool horizontallyOverlapping =
                    body.x + body.width > previousBlock.x &&
                    body.x < previousBlock.x + previousBlock.width;

                float bodyBottom = body.y + body.height;

                if (
                    horizontallyOverlapping &&
                    std::abs(bodyBottom - previousBlock.y) <=
                        footTolerance
                )
                {
                    body.y += movementY;
                }
            };

        for (SuperMushroom& mushroom : mushrooms)
        {
            if (!mushroom.collected && !mushroom.emerging)
            {
                carryBody(mushroom.body);
            }
        }

        for (FireFlower& flower : fireFlowers)
        {
            if (!flower.collected && !flower.emerging)
            {
                carryBody(flower.body);
            }
        }

        for (Goomba& goomba : goombas)
        {
            if (goomba.alive && !goomba.dying && goomba.spawned)
            {
                carryBody(goomba.body);
            }
        }

        for (Koopa& koopa : koopas)
        {
            if (koopa.alive && !koopa.dying && koopa.spawned)
            {
                carryBody(koopa.body);
            }
        }
    }

    std::erase_if(
        blockBumps,
        [bumpDuration](const BlockBump& bump)
        {
            return bump.age >= bumpDuration;
        }
    );
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
				false,
				false
			};

            goombas.push_back(goomba);
        }
    }

    return goombas;
}

std::vector<Koopa> FindKoopaSpawns(
    const Level& level
)
{
    std::vector<Koopa> koopas;

    for (int row = 0; row < GetLevelRows(level); ++row)
    {
        for (int column = 0; column < GetLevelColumns(level); ++column)
        {
            if (level[row][column] != 'K')
            {
                continue;
            }

            koopas.push_back({
                {
                    column * static_cast<float>(tileSize) + 6.0f,
                    row * static_cast<float>(tileSize),
                    36.0f,
                    48.0f
                },
                {-90.0f, 0.0f},
                false,
                true,
                false,
                false,
                false
            });
        }
    }

    return koopas;
}

// Collect Coins

std::vector<Rectangle> GetNearbySolidTiles(
    const Level& level,
    const Rectangle& body
);

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

void UpdateBlockCoins(
    std::vector<BlockCoin>& blockCoins,
    float deltaTime
)
{
    constexpr float animationDuration = 0.65f;
    constexpr float riseHeight = 42.0f;

    for (BlockCoin& coin : blockCoins)
    {
        coin.age += deltaTime;

        float progress = std::clamp(
            coin.age / animationDuration,
            0.0f,
            1.0f
        );

        coin.body.y =
            coin.blockTopY -
            coin.body.height -
            std::sin(progress * PI) * riseHeight;
    }

    blockCoins.erase(
        std::remove_if(
            blockCoins.begin(),
            blockCoins.end(),
            [animationDuration](const BlockCoin& coin)
            {
                return coin.age >= animationDuration;
            }
        ),
        blockCoins.end()
    );
}

void UpdateSuperMushrooms(
    std::vector<SuperMushroom>& mushrooms,
    const Level& level,
    float deltaTime,
    float gravity
)
{
    constexpr float emergeSpeed = 48.0f;
    constexpr float moveSpeed = 100.0f;

    for (SuperMushroom& mushroom : mushrooms)
    {
        if (mushroom.collected)
        {
            continue;
        }

        if (mushroom.emerging)
        {
            mushroom.body.y = std::max(
                mushroom.targetY,
                mushroom.body.y - emergeSpeed * deltaTime
            );

            if (mushroom.body.y <= mushroom.targetY)
            {
                mushroom.emerging = false;
                mushroom.velocity.x = moveSpeed;
            }

            continue;
        }

        float previousX = mushroom.body.x;
        mushroom.body.x += mushroom.velocity.x * deltaTime;

        for (const Rectangle& tile : GetNearbySolidTiles(level, mushroom.body))
        {
            if (!CheckCollisionRecs(mushroom.body, tile))
            {
                continue;
            }

            if (mushroom.velocity.x > 0.0f && previousX + mushroom.body.width <= tile.x)
            {
                mushroom.body.x = tile.x - mushroom.body.width;
                mushroom.velocity.x = -moveSpeed;
            }
            else if (mushroom.velocity.x < 0.0f && previousX >= tile.x + tile.width)
            {
                mushroom.body.x = tile.x + tile.width;
                mushroom.velocity.x = moveSpeed;
            }
        }

        float previousY = mushroom.body.y;
        mushroom.velocity.y += gravity * deltaTime;
        mushroom.body.y += mushroom.velocity.y * deltaTime;

        for (const Rectangle& tile : GetNearbySolidTiles(level, mushroom.body))
        {
            if (!CheckCollisionRecs(mushroom.body, tile))
            {
                continue;
            }

            if (
                mushroom.velocity.y > 0.0f &&
                previousY + mushroom.body.height <= tile.y + 10.0f
            )
            {
                mushroom.body.y = tile.y - mushroom.body.height;
                mushroom.velocity.y = 0.0f;
            }
            else if (mushroom.velocity.y < 0.0f && previousY >= tile.y + tile.height)
            {
                mushroom.body.y = tile.y + tile.height;
                mushroom.velocity.y = 0.0f;
            }
        }
    }
}

void CollectSuperMushrooms(
    Player& player,
    std::vector<SuperMushroom>& mushrooms
)
{
    for (SuperMushroom& mushroom : mushrooms)
    {
        if (
            mushroom.collected ||
            mushroom.emerging ||
            !CheckCollisionRecs(player.body, mushroom.body)
        )
        {
            continue;
        }

        mushroom.collected = true;

        if (mushroom.oneUp)
        {
            continue;
        }

        if (!player.big)
        {
            float bottom = player.body.y + player.body.height;
            float centre = player.body.x + player.body.width / 2.0f;

            player.body.width = 40.0f;
            player.body.height = 88.0f;
            player.body.x = centre - player.body.width / 2.0f;
            player.body.y = bottom - player.body.height;
            player.big = true;
            player.ducking = false;
        }
    }
}

void UpdateFireFlowers(
    std::vector<FireFlower>& flowers,
    float deltaTime
)
{
    constexpr float emergeSpeed = 48.0f;

    for (FireFlower& flower : flowers)
    {
        if (flower.collected || !flower.emerging)
        {
            continue;
        }

        flower.body.y = std::max(
            flower.targetY,
            flower.body.y - emergeSpeed * deltaTime
        );

        if (flower.body.y <= flower.targetY)
        {
            flower.emerging = false;
        }
    }
}

void CollectFireFlowers(
    Player& player,
    std::vector<FireFlower>& flowers
)
{
    for (FireFlower& flower : flowers)
    {
        if (
            flower.collected ||
            flower.emerging ||
            !CheckCollisionRecs(player.body, flower.body)
        )
        {
            continue;
        }

        flower.collected = true;
        player.big = true;
        player.fire = true;
    }
}

void UpdateFireBalls(
    std::vector<FireBall>& fireBalls,
    const Level& level,
    std::vector<Goomba>& goombas,
    std::vector<Koopa>& koopas,
    const Camera2D& camera,
    float deltaTime
)
{
    constexpr float fireBallGravity = 3375.0f;
    constexpr float maximumFallSpeed = 540.0f;
    constexpr float bounceSpeed = 540.0f;

    for (FireBall& fireBall : fireBalls)
    {
        if (!fireBall.alive)
        {
            continue;
        }

        float previousX = fireBall.body.x;
        fireBall.body.x += fireBall.velocity.x * deltaTime;

        for (const Rectangle& tile : GetNearbySolidTiles(level, fireBall.body))
        {
            if (!CheckCollisionRecs(fireBall.body, tile))
            {
                continue;
            }

            bool hitFromSide =
                (fireBall.velocity.x > 0.0f &&
                    previousX + fireBall.body.width <= tile.x) ||
                (fireBall.velocity.x < 0.0f &&
                    previousX >= tile.x + tile.width);

            if (hitFromSide)
            {
                fireBall.alive = false;
                break;
            }
        }

        if (!fireBall.alive)
        {
            continue;
        }

        float previousY = fireBall.body.y;
        fireBall.velocity.y = std::min(
            maximumFallSpeed,
            fireBall.velocity.y + fireBallGravity * deltaTime
        );
        fireBall.body.y += fireBall.velocity.y * deltaTime;

        for (const Rectangle& tile : GetNearbySolidTiles(level, fireBall.body))
        {
            if (!CheckCollisionRecs(fireBall.body, tile))
            {
                continue;
            }

            if (
                fireBall.velocity.y > 0.0f &&
                previousY + fireBall.body.height <= tile.y
            )
            {
                fireBall.body.y = tile.y - fireBall.body.height;
                fireBall.velocity.y = -bounceSpeed;
                fireBall.rotation = std::fmod(
                    fireBall.rotation + 180.0f,
                    360.0f
                );
                break;
            }

            if (
                fireBall.velocity.y < 0.0f &&
                previousY >= tile.y + tile.height
            )
            {
                fireBall.alive = false;
                break;
            }
        }

        for (Goomba& goomba : goombas)
        {
            if (
                goomba.alive &&
                !goomba.dying &&
                goomba.spawned &&
                CheckCollisionRecs(fireBall.body, goomba.body)
            )
            {
                KillGoomba(goomba);
                fireBall.alive = false;
                break;
            }
        }

        for (Koopa& koopa : koopas)
        {
            if (
                fireBall.alive &&
                koopa.alive &&
                !koopa.dying &&
                koopa.spawned &&
                CheckCollisionRecs(fireBall.body, koopa.body)
            )
            {
                KillKoopa(koopa);
                fireBall.alive = false;
                break;
            }
        }

        float halfVisibleWidth =
            virtualWidth / (2.0f * camera.zoom);

        float halfVisibleHeight =
            virtualHeight / (2.0f * camera.zoom);

        float cameraLeft = camera.target.x - halfVisibleWidth;
        float cameraRight = camera.target.x + halfVisibleWidth;
        float cameraTop = camera.target.y - halfVisibleHeight;
        float cameraBottom = camera.target.y + halfVisibleHeight;

        bool outsideCamera =
            fireBall.body.x + fireBall.body.width < cameraLeft ||
            fireBall.body.x > cameraRight ||
            fireBall.body.y + fireBall.body.height < cameraTop ||
            fireBall.body.y > cameraBottom;

        if (outsideCamera)
        {
            fireBall.alive = false;
        }
    }

    std::erase_if(
        fireBalls,
        [](const FireBall& fireBall)
        {
            return !fireBall.alive;
        }
    );
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

void SetPlayerDucking(
    Player& player,
    bool ducking,
    const Level& level
)
{
    if (!player.big || player.ducking == ducking)
    {
        return;
    }

    float bottom = player.body.y + player.body.height;
    float centre = player.body.x + player.body.width / 2.0f;

    Rectangle newBody = ducking
        ? Rectangle{
            centre - 16.0f,
            bottom - 44.0f,
            32.0f,
            44.0f
        }
        : Rectangle{
            centre - 20.0f,
            bottom - 88.0f,
            40.0f,
            88.0f
        };

    if (!ducking)
    {
        for (const Rectangle& tile :
            GetNearbySolidTiles(level, newBody))
        {
            if (CheckCollisionRecs(newBody, tile))
            {
                return;
            }
        }
    }

    player.body = newBody;
    player.ducking = ducking;
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

bool IsEntityStandingOnBlock(
    const Rectangle& entity,
    const Rectangle& block
)
{
    constexpr float footTolerance = 4.0f;

    bool horizontallyOverlapping =
        entity.x + entity.width > block.x &&
        entity.x < block.x + block.width;

    float entityBottom = entity.y + entity.height;

    return
        horizontallyOverlapping &&
        std::abs(entityBottom - block.y) <= footTolerance;
}

void HitEntitiesAboveBlock(
    const Rectangle& block,
    std::vector<SuperMushroom>& mushrooms,
    std::vector<Goomba>& goombas,
    std::vector<Koopa>& koopas
)
{
    constexpr float mushroomBounceSpeed = 500.0f;

    for (SuperMushroom& mushroom : mushrooms)
    {
        if (
            !mushroom.collected &&
            !mushroom.emerging &&
            IsEntityStandingOnBlock(mushroom.body, block)
        )
        {
            mushroom.velocity.y = -mushroomBounceSpeed;
        }
    }

    for (Goomba& goomba : goombas)
    {
        if (
            goomba.alive &&
            goomba.spawned &&
            IsEntityStandingOnBlock(goomba.body, block)
        )
        {
            KillGoomba(goomba);
        }
    }

    for (Koopa& koopa : koopas)
    {
        if (
            koopa.alive &&
            koopa.spawned &&
            IsEntityStandingOnBlock(koopa.body, block)
        )
        {
            KillKoopa(koopa);
        }
    }
}

void ResolvePlayerVerticalCollisions(
    Player& player,
    Level& level,
    std::vector<SuperMushroom>& mushrooms,
    std::vector<FireFlower>& fireFlowers,
    std::vector<Goomba>& goombas,
    std::vector<Koopa>& koopas,
    std::vector<BlockCoin>& blockCoins,
    int& coinCount,
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

    if (player.velocity.y < 0.0f)
    {
        int leftColumn = std::max(
            0,
            static_cast<int>(std::floor(player.body.x / tileSize)) - 1
        );

        int rightColumn = std::min(
            GetLevelColumns(level) - 1,
            static_cast<int>(std::floor(
                (player.body.x + player.body.width) / tileSize
            )) + 1
        );

        int topRow = std::max(
            0,
            static_cast<int>(std::floor(player.body.y / tileSize)) - 1
        );

        int bottomRow = std::min(
            GetLevelRows(level) - 1,
            static_cast<int>(std::floor(
                (player.body.y + player.body.height) / tileSize
            )) + 1
        );

        for (int row = topRow; row <= bottomRow; ++row)
        {
            for (int column = leftColumn; column <= rightColumn; ++column)
            {
                if (level[row][column] == 'I')
                {
                    nearbyTiles.push_back(
                        GetTileRectangle(row, column)
                    );
                }
            }
        }
    }

    Rectangle closestCeilingTile{};
    bool hasClosestCeilingTile = false;

    if (player.velocity.y < 0.0f)
    {
        float playerCentreX =
            player.body.x + player.body.width / 2.0f;

        float closestDistance = 0.0f;

        for (const Rectangle& tile : nearbyTiles)
        {
            if (
                !CheckCollisionRecs(player.body, tile) ||
                previousY < tile.y + tile.height
            )
            {
                continue;
            }

            float tileCentreX =
                tile.x + tile.width / 2.0f;

            float distance =
                std::abs(playerCentreX - tileCentreX);

            if (
                !hasClosestCeilingTile ||
                distance < closestDistance
            )
            {
                closestCeilingTile = tile;
                closestDistance = distance;
                hasClosestCeilingTile = true;
            }
        }
    }

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

            if (previousBottom <= tile.y + 10.0f)
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
            if (
                hasClosestCeilingTile &&
                (
                    tile.x != closestCeilingTile.x ||
                    tile.y != closestCeilingTile.y
                )
            )
            {
                continue;
            }

            int ceilingColumn =
                static_cast<int>(std::lround(tile.x / tileSize));

            int ceilingRow =
                static_cast<int>(std::lround(tile.y / tileSize));

            float playerCentreX =
                player.body.x + player.body.width / 2.0f;

            float tileCentreX =
                tile.x + tile.width / 2.0f;

            bool gapOnLeft =
                ceilingColumn > 0 &&
                !IsSolidTile(
                    level,
                    ceilingRow,
                    ceilingColumn - 1
                );

            bool gapOnRight =
                ceilingColumn + 1 < GetLevelColumns(level) &&
                !IsSolidTile(
                    level,
                    ceilingRow,
                    ceilingColumn + 1
                );

            constexpr float cornerSeparation = 0.1f;
            bool correctedIntoGap = false;

            float overlapWidth = std::max(
                0.0f,
                std::min(
                    player.body.x + player.body.width,
                    tile.x + tile.width
                ) -
                std::max(player.body.x, tile.x)
            );

            bool lessThanHalfUnderBlock =
                overlapWidth < player.body.width / 2.0f;

            if (
                lessThanHalfUnderBlock &&
                playerCentreX < tileCentreX &&
                gapOnLeft
            )
            {
                player.body.x =
                    tile.x - player.body.width - cornerSeparation;

                correctedIntoGap = true;
            }
            else if (
                lessThanHalfUnderBlock &&
                playerCentreX > tileCentreX &&
                gapOnRight
            )
            {
                player.body.x =
                    tile.x + tile.width + cornerSeparation;

                correctedIntoGap = true;
            }

            if (correctedIntoGap)
            {
                player.body.x = std::clamp(
                    player.body.x,
                    0.0f,
                    GetLevelWidth(level) - player.body.width
                );

                continue;
            }

            float tileBottom =
                tile.y + tile.height;

            if (previousY >= tileBottom)
            {
                player.body.y = tileBottom;
                player.velocity.y = 0.0f;

                int tileColumn =
                    static_cast<int>(std::lround(tile.x / tileSize));

                int tileRow =
                    static_cast<int>(std::lround(tile.y / tileSize));

                char& hitTile =
                    level[tileRow][tileColumn];

                if (
                    hitTile == 'B' ||
                    hitTile == 'M' ||
                    hitTile == 'A' ||
                    hitTile == 'I' ||
                    hitTile == 'Q'
                )
                {
                    HitEntitiesAboveBlock(
                        tile,
                        mushrooms,
                        goombas,
                        koopas
                    );
                }

                if (hitTile == 'B' && player.big)
                {
                    hitTile = '.';
                }
                else if (hitTile == 'B')
                {
                    StartBlockBump(tileRow, tileColumn);
                }
                else if (hitTile == 'M')
                {
                    hitTile = 'E';
                    StartBlockBump(tileRow, tileColumn);

                    constexpr float mushroomSize =
                        static_cast<float>(tileSize);

                    constexpr float powerUpHitboxWidth =
                        mushroomSize + 2.0f;

                    constexpr float blockBumpHeight = 10.0f;

                    mushrooms.push_back({
                        {
                            tile.x - 1.0f,
                            tile.y - blockBumpHeight,
                            powerUpHitboxWidth,
                            mushroomSize
                        },
                        {0.0f, 0.0f},
                        tile.y - mushroomSize,
                        true,
                        false,
                        false
                    });
                }
                else if (hitTile == 'A')
                {
                    hitTile = 'E';
                    StartBlockBump(tileRow, tileColumn);

                    constexpr float itemSize =
                        static_cast<float>(tileSize);

                    constexpr float powerUpHitboxWidth =
                        itemSize + 2.0f;

                    constexpr float blockBumpHeight = 10.0f;

                    if (player.big)
                    {
                        fireFlowers.push_back({
                            {
                                tile.x - 1.0f,
                                tile.y - blockBumpHeight,
                                powerUpHitboxWidth,
                                itemSize
                            },
                            tile.y - itemSize,
                            true,
                            false
                        });
                    }
                    else
                    {
                        mushrooms.push_back({
                            {
                                tile.x - 1.0f,
                                tile.y - blockBumpHeight,
                                powerUpHitboxWidth,
                                itemSize
                            },
                            {0.0f, 0.0f},
                            tile.y - itemSize,
                            true,
                            false,
                            false
                        });
                    }
                }
                else if (hitTile == 'I')
                {
                    hitTile = 'E';
                    StartBlockBump(tileRow, tileColumn);

                    constexpr float itemSize =
                        static_cast<float>(tileSize);

                    constexpr float hitboxWidth = itemSize + 2.0f;
                    constexpr float blockBumpHeight = 10.0f;

                    mushrooms.push_back({
                        {
                            tile.x - 1.0f,
                            tile.y - blockBumpHeight,
                            hitboxWidth,
                            itemSize
                        },
                        {0.0f, 0.0f},
                        tile.y - itemSize,
                        true,
                        false,
                        true
                    });
                }
                else if (hitTile == 'Q')
                {
                    hitTile = 'E';
                    StartBlockBump(tileRow, tileColumn);
                    ++coinCount;

                    constexpr float coinWidth = 24.0f;
                    constexpr float coinHeight = 32.0f;

                    blockCoins.push_back({
                        {
                            tile.x +
                                (tileSize - coinWidth) / 2.0f,
                            tile.y,
                            coinWidth,
                            coinHeight
                        },
                        tile.y,
                        0.0f
                    });
                }
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

            if (previousBottom <= tile.y + 10.0f)
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

        if (goomba.dying)
        {
            goomba.velocity.y += gravity * deltaTime;
            goomba.body.y += goomba.velocity.y * deltaTime;

            if (goomba.body.y > GetLevelHeight(level) + 200.0f)
            {
                goomba.alive = false;
            }

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

void ResolveKoopaHorizontalCollisions(
    Koopa& koopa,
    Level& level,
    float deltaTime
)
{
    float previousX = koopa.body.x;
    koopa.body.x += koopa.velocity.x * deltaTime;

    for (const Rectangle& tile : GetNearbySolidTiles(level, koopa.body))
    {
        if (!CheckCollisionRecs(koopa.body, tile))
        {
            continue;
        }

        int tileColumn =
            static_cast<int>(std::lround(tile.x / tileSize));

        int tileRow =
            static_cast<int>(std::lround(tile.y / tileSize));

        if (
            koopa.shelled &&
            level[tileRow][tileColumn] == 'B'
        )
        {
            level[tileRow][tileColumn] = '.';
        }

        if (koopa.velocity.x > 0.0f && previousX + koopa.body.width <= tile.x)
        {
            koopa.body.x = tile.x - koopa.body.width;
            koopa.velocity.x = -std::abs(koopa.velocity.x);
            return;
        }

        if (koopa.velocity.x < 0.0f && previousX >= tile.x + tile.width)
        {
            koopa.body.x = tile.x + tile.width;
            koopa.velocity.x = std::abs(koopa.velocity.x);
            return;
        }
    }

    if (koopa.body.x + koopa.body.width > GetLevelWidth(level))
    {
        koopa.body.x = GetLevelWidth(level) - koopa.body.width;
        koopa.velocity.x = -std::abs(koopa.velocity.x);
    }
}

void ResolveKoopaVerticalCollisions(
    Koopa& koopa,
    const Level& level,
    float deltaTime
)
{
    float previousY = koopa.body.y;
    koopa.body.y += koopa.velocity.y * deltaTime;
    koopa.onGround = false;

    for (const Rectangle& tile : GetNearbySolidTiles(level, koopa.body))
    {
        if (!CheckCollisionRecs(koopa.body, tile))
        {
            continue;
        }

        if (
            koopa.velocity.y > 0.0f &&
            previousY + koopa.body.height <= tile.y + 10.0f
        )
        {
            koopa.body.y = tile.y - koopa.body.height;
            koopa.velocity.y = 0.0f;
            koopa.onGround = true;
        }
        else if (koopa.velocity.y < 0.0f && previousY >= tile.y + tile.height)
        {
            koopa.body.y = tile.y + tile.height;
            koopa.velocity.y = 0.0f;
        }
    }
}

void UpdateKoopas(
    std::vector<Koopa>& koopas,
    Level& level,
    float deltaTime,
    float gravity
)
{
    for (Koopa& koopa : koopas)
    {
        if (!koopa.alive || !koopa.spawned)
        {
            continue;
        }

        if (koopa.dying)
        {
            koopa.velocity.y += gravity * deltaTime;
            koopa.body.y += koopa.velocity.y * deltaTime;

            if (koopa.body.y > GetLevelHeight(level) + 200.0f)
            {
                koopa.alive = false;
            }

            continue;
        }

        koopa.velocity.y += gravity * deltaTime;

        if (!koopa.shelled || std::abs(koopa.velocity.x) > 1.0f)
        {
            ResolveKoopaHorizontalCollisions(koopa, level, deltaTime);
        }

        ResolveKoopaVerticalCollisions(koopa, level, deltaTime);

        if (koopa.body.y > GetLevelHeight(level) + 200.0f)
        {
            koopa.alive = false;
        }
    }
}

void SpawnKoopasNearCamera(
    std::vector<Koopa>& koopas,
    const Camera2D& camera
)
{
    constexpr float spawnMargin = 96.0f;
    float cameraRight = camera.target.x + virtualWidth / (2.0f * camera.zoom);

    for (Koopa& koopa : koopas)
    {
        if (!koopa.spawned && koopa.alive && koopa.body.x <= cameraRight + spawnMargin)
        {
            koopa.spawned = true;
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
			goomba.dying ||
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
        return player.invulnerabilityTimer <= 0.0f;
    }

    return false;
}

bool HandlePlayerKoopaCollisions(
    Player& player,
    std::vector<Koopa>& koopas,
    float previousPlayerBottom
)
{
    constexpr float stompTolerance = 12.0f;
    constexpr float stompBounceSpeed = 450.0f;
    constexpr float shellSpeed = 520.0f;

    for (Koopa& koopa : koopas)
    {
        if (
            !koopa.alive ||
            koopa.dying ||
            !koopa.spawned ||
            !CheckCollisionRecs(player.body, koopa.body)
        )
        {
            continue;
        }

        bool landedOnTop =
            player.velocity.y >= 0.0f &&
            previousPlayerBottom <= koopa.body.y + stompTolerance;

        if (!koopa.shelled && landedOnTop)
        {
            float feet = koopa.body.y + koopa.body.height;
            koopa.shelled = true;
            koopa.velocity = {0.0f, 0.0f};
            koopa.body = {koopa.body.x, feet - 32.0f, 40.0f, 32.0f};

            player.body.y = koopa.body.y - player.body.height;
            player.velocity.y = -stompBounceSpeed;
            player.onGround = false;
            return false;
        }

        if (!koopa.shelled)
        {
            return player.invulnerabilityTimer <= 0.0f;
        }

        bool shellMoving =
            std::abs(koopa.velocity.x) > 1.0f;

        if (landedOnTop)
        {
            player.body.y = koopa.body.y - player.body.height;

            if (shellMoving)
            {
                // Stomping a moving shell brings it to a halt.
                koopa.velocity.x = 0.0f;
            }
            else
            {
                // Kick away from whichever side of the shell Mario is on.
                float playerCentre =
                    player.body.x + player.body.width / 2.0f;

                float shellCentre =
                    koopa.body.x + koopa.body.width / 2.0f;

                koopa.velocity.x =
                    playerCentre < shellCentre
                        ? shellSpeed
                        : -shellSpeed;
            }

            player.velocity.y = -stompBounceSpeed;
            player.onGround = false;
            return false;
        }

        if (shellMoving)
        {
            // Any non-stomp contact with a moving shell hurts Mario.
            return player.invulnerabilityTimer <= 0.0f;
        }

        float overlapLeft = player.body.x + player.body.width - koopa.body.x;
        float overlapRight = koopa.body.x + koopa.body.width - player.body.x;
        float overlapTop = player.body.y + player.body.height - koopa.body.y;
        float overlapBottom = koopa.body.y + koopa.body.height - player.body.y;
        float smallest = std::min({overlapLeft, overlapRight, overlapTop, overlapBottom});

        if (smallest == overlapLeft)
        {
            player.body.x = koopa.body.x - player.body.width;
            player.velocity.x = 0.0f;
            koopa.velocity.x = shellSpeed;
        }
        else if (smallest == overlapRight)
        {
            player.body.x = koopa.body.x + koopa.body.width;
            player.velocity.x = 0.0f;
            koopa.velocity.x = -shellSpeed;
        }
        else if (smallest == overlapBottom)
        {
            player.body.y = koopa.body.y + koopa.body.height;
            player.velocity.y = 0.0f;
        }
    }

    return false;
}

void HandleMovingShellEnemyCollisions(
    std::vector<Koopa>& koopas,
    std::vector<Goomba>& goombas
)
{
    for (Koopa& shell : koopas)
    {
        if (
            !shell.alive ||
            shell.dying ||
            !shell.spawned ||
            !shell.shelled ||
            std::abs(shell.velocity.x) <= 1.0f
        )
        {
            continue;
        }

        for (Goomba& goomba : goombas)
        {
            if (
                goomba.alive &&
                !goomba.dying &&
                goomba.spawned &&
                CheckCollisionRecs(shell.body, goomba.body)
            )
            {
                KillGoomba(goomba);
            }
        }

        for (Koopa& enemy : koopas)
        {
            if (
                &enemy != &shell &&
                enemy.alive &&
                !enemy.dying &&
                enemy.spawned &&
                CheckCollisionRecs(shell.body, enemy.body)
            )
            {
                bool enemyIsMovingShell =
                    enemy.shelled &&
                    std::abs(enemy.velocity.x) > 1.0f;

                KillKoopa(enemy);

                if (enemyIsMovingShell)
                {
                    KillKoopa(shell);
                    break;
                }
            }
        }
    }
}

void HandleStationaryShellEnemyCollisions(
    std::vector<Koopa>& koopas,
    std::vector<Goomba>& goombas
)
{
    for (Koopa& shell : koopas)
    {
        if (
            !shell.alive ||
            shell.dying ||
            !shell.spawned ||
            !shell.shelled ||
            std::abs(shell.velocity.x) > 1.0f
        )
        {
            continue;
        }

        for (Goomba& goomba : goombas)
        {
            if (
                !goomba.alive ||
                goomba.dying ||
                !goomba.spawned ||
                !CheckCollisionRecs(shell.body, goomba.body)
            )
            {
                continue;
            }

            if (goomba.velocity.x > 0.0f)
            {
                goomba.body.x = shell.body.x - goomba.body.width;
                goomba.velocity.x = -std::abs(goomba.velocity.x);
            }
            else
            {
                goomba.body.x = shell.body.x + shell.body.width;
                goomba.velocity.x = std::abs(goomba.velocity.x);
            }
        }

        for (Koopa& enemy : koopas)
        {
            if (
                &enemy == &shell ||
                !enemy.alive ||
                enemy.dying ||
                !enemy.spawned ||
                enemy.shelled ||
                !CheckCollisionRecs(shell.body, enemy.body)
            )
            {
                continue;
            }

            if (enemy.velocity.x > 0.0f)
            {
                enemy.body.x = shell.body.x - enemy.body.width;
                enemy.velocity.x = -std::abs(enemy.velocity.x);
            }
            else
            {
                enemy.body.x = shell.body.x + shell.body.width;
                enemy.velocity.x = std::abs(enemy.velocity.x);
            }
        }
    }
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
                case 'M':
                case 'A':
                    DrawTextureAsTile(
                        textures.questionBlock,
                        destination
                    );
                    break;

                case 'E':
                    DrawTextureAsTile(
                        textures.emptyBlock,
                        destination
                    );
                    break;

                case 'F':
                {
                    constexpr float flagScale = 3.0f;

                    Rectangle source{
                        0.0f,
                        0.0f,
                        static_cast<float>(textures.flag.width),
                        static_cast<float>(textures.flag.height)
                    };

                    Rectangle flagDestination{
                        destination.x - 8.0f * flagScale,
                        destination.y + tileSize * 2.0f -
                            textures.flag.height * flagScale,
                        textures.flag.width * flagScale,
                        textures.flag.height * flagScale
                    };

                    DrawTexturePro(
                        textures.flag,
                        source,
                        flagDestination,
                        {0.0f, 0.0f},
                        0.0f,
                        WHITE
                    );
                    break;
                }

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
                case 'v':
                    break;

                case 'V':
                {
                    Rectangle pipeBaseDestination{
                        destination.x,
                        destination.y,
                        tileSize * 2.0f,
                        static_cast<float>(tileSize)
                    };

                    DrawTextureAsTile(
                        textures.pipeBase,
                        pipeBaseDestination
                    );
                    break;
                }

                default:
                    break;
            }
        }
    }
}

void DrawPlayer(
    const Player& player,
    const GameTextures& textures
)
{
    const Texture2D* texture = &textures.player;

    const Texture2D* smallWalkTextures[] = {
        &textures.smallMarioWalk1,
        &textures.smallMarioWalk2,
        &textures.smallMarioWalk3
    };

    const Texture2D* superWalkTextures[] = {
        &textures.superMarioWalk1,
        &textures.superMarioWalk2,
        &textures.superMarioWalk3
    };

    const Texture2D* fireWalkTextures[] = {
        &textures.fireMarioWalk1,
        &textures.fireMarioWalk2,
        &textures.fireMarioWalk3
    };

    if (player.dying)
    {
        texture = &textures.marioDead;
    }
    else if (player.ducking)
    {
        texture = player.fire
            ? &textures.fireMarioDuck
            : &textures.superMarioDuck;
    }
    else if (player.fireThrowTimer > 0.0f && player.fire)
    {
        texture = &textures.fireMarioThrow;
    }
    else if (!player.onGround)
    {
        texture = player.fire
            ? &textures.fireMarioJump
            : (player.big
                ? &textures.superMarioJump
                : &textures.smallMarioJump);
    }
    else if (player.skidding)
    {
        texture = player.fire
            ? &textures.fireMarioSwitch
            : (player.big
                ? &textures.superMarioSwitch
                : &textures.smallMarioSwitch);
    }
    else if (std::abs(player.velocity.x) > 5.0f)
    {
        int frame = player.walkAnimationFrame % 3;
        texture = player.fire
            ? fireWalkTextures[frame]
            : (player.big
                ? superWalkTextures[frame]
                : smallWalkTextures[frame]);
    }
    else if (player.fire)
    {
        texture = &textures.fireMario;
    }
    else if (player.big)
    {
        texture = &textures.superMario;
    }

    Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(texture->width),
        static_cast<float>(texture->height)
    };

    constexpr float marioSpriteScale = 3.0f;

    float spriteWidth =
        texture->width * marioSpriteScale;

    float spriteHeight =
        texture->height * marioSpriteScale;

    Rectangle destination{
        player.body.x +
            (player.body.width - spriteWidth) / 2.0f,

        player.body.y +
            player.body.height -
            spriteHeight,

        spriteWidth,
        spriteHeight
    };

    DrawTextureInsideRectangle(
        *texture,
        source,
        destination,
        player.facingLeft
    );
}

void DrawFireFlowers(
    const std::vector<FireFlower>& flowers,
    const Texture2D& texture
)
{
    Rectangle source{0.0f, 0.0f, 16.0f, 16.0f};

    for (const FireFlower& flower : flowers)
    {
        if (!flower.collected)
        {
            Rectangle spriteBody = flower.body;
            spriteBody.x += 1.0f;
            spriteBody.width -= 2.0f;

            DrawTextureInsideRectangle(texture, source, spriteBody);
        }
    }
}

void DrawFireBalls(
    const std::vector<FireBall>& fireBalls,
    const Texture2D& texture
)
{
    Rectangle source{0.0f, 0.0f, 8.0f, 8.0f};

    for (const FireBall& fireBall : fireBalls)
    {
        Rectangle destination{
            fireBall.body.x + fireBall.body.width / 2.0f,
            fireBall.body.y + fireBall.body.height / 2.0f,
            fireBall.body.width,
            fireBall.body.height
        };

        DrawTexturePro(
            texture,
            source,
            destination,
            {
                fireBall.body.width / 2.0f,
                fireBall.body.height / 2.0f
            },
            fireBall.rotation,
            WHITE
        );
    }
}

void DrawSuperMushrooms(
    const std::vector<SuperMushroom>& mushrooms,
    const Texture2D& superTexture,
    const Texture2D& oneUpTexture
)
{
    Rectangle source{0.0f, 0.0f, 16.0f, 16.0f};

    for (const SuperMushroom& mushroom : mushrooms)
    {
        if (!mushroom.collected)
        {
            const Texture2D& texture = mushroom.oneUp
                ? oneUpTexture
                : superTexture;

            Rectangle spriteBody = mushroom.body;
            spriteBody.x += 1.0f;
            spriteBody.width -= 2.0f;

            DrawTextureInsideRectangle(
                texture,
                source,
                spriteBody
            );
        }
    }
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

void DrawBlockCoins(
    const std::vector<BlockCoin>& blockCoins,
    const Texture2D& texture
)
{
    Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(texture.width),
        static_cast<float>(texture.height)
    };

    for (const BlockCoin& coin : blockCoins)
    {
        // Briefly flicker as the coin pops above the used block.
        if (std::fmod(coin.age, 0.12f) < 0.035f)
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

    if (goomba.dying)
    {
        source.y += source.height;
        source.height = -source.height;
    }

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

void DrawKoopa(
    const Koopa& koopa,
    const Texture2D& koopaTexture,
    const Texture2D& shellTexture
)
{
    if (!koopa.alive || !koopa.spawned)
    {
        return;
    }

    const Texture2D& texture =
        koopa.shelled ? shellTexture : koopaTexture;

    Rectangle source = koopa.shelled
        ? Rectangle{0.0f, 0.0f, 16.0f, 14.0f}
        : Rectangle{0.0f, 0.0f, 16.0f, 23.0f};

    if (koopa.dying)
    {
        source.y += source.height;
        source.height = -source.height;
    }

    constexpr float spriteWidth = 48.0f;

    float spriteHeight = koopa.shelled
        ? spriteWidth * (14.0f / 16.0f)
        : spriteWidth * (23.0f / 16.0f);

    Rectangle destination{
        koopa.body.x +
            (koopa.body.width - spriteWidth) / 2.0f,

        koopa.body.y +
            koopa.body.height -
            spriteHeight,

        spriteWidth,
        spriteHeight
    };

    DrawTextureInsideRectangle(
        texture,
        source,
        destination,
        !koopa.shelled && koopa.velocity.x > 0.0f
    );
}

// --------------------------------------------------
// Camera
// --------------------------------------------------

void UpdateCamera(
    Camera2D& camera,
    const Player& player,
    const Level& level,
    float deltaTime
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

    constexpr float cameraFollowSpeed = 520.0f;

    camera.target.x = std::min(
        desiredTargetX,
        camera.target.x + cameraFollowSpeed * deltaTime
    );

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
    Level& level,
    const Level& initialLevel,
    std::vector<Coin>& coins,
    const std::vector<Coin>& initialCoins,
    std::vector<BlockCoin>& blockCoins,
    int& coinCount,
    std::vector<Goomba>& goombas,
    const std::vector<Goomba>& initialGoombas,
    std::vector<Koopa>& koopas,
    const std::vector<Koopa>& initialKoopas,
    std::vector<SuperMushroom>& mushrooms,
    std::vector<FireFlower>& fireFlowers,
    std::vector<FireBall>& fireBalls,
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
    player.big = false;
    player.fire = false;
    player.invulnerabilityTimer = 0.0f;
    player.sprinting = false;
    player.facingLeft = false;
    player.dying = false;
    player.ducking = false;
    player.walkAnimationTimer = 0.0f;
    player.walkAnimationFrame = 0;
    player.fireThrowTimer = 0.0f;
    player.skidding = false;

    jumpBufferTimer = 0.0f;
    coyoteTimer = 0.0f;

    camera.target = {
        virtualWidth / 2.0f,
        virtualHeight / 2.0f
    };

    level = initialLevel;

    coins = initialCoins;
    blockCoins.clear();
    coinCount = 0;

    goombas = initialGoombas;
    koopas = initialKoopas;
    mushrooms.clear();
    fireFlowers.clear();
    fireBalls.clear();
    blockBumps.clear();
}

// --------------------------------------------------
// Main
// --------------------------------------------------

int main()
{
    constexpr float walkSpeed = 280.0f;
    constexpr float sprintSpeed = 450.0f;

    constexpr float walkingAcceleration = 750.0f;
    constexpr float sprintAcceleration = 950.0f;
    constexpr float friction = 1050.0f;

    constexpr float jumpSpeed = 850.0f;
    constexpr float gravity = 1800.0f;

    // SMB-style jumps use a lighter force while rising and a
    // substantially stronger force after the apex. Releasing jump
    // early applies the strongest force, producing the original
    // game's variable-height, snappy jump arc.
    constexpr float risingGravity = 1550.0f;
    constexpr float fallingGravity = 2850.0f;
    constexpr float releasedJumpGravity = 3600.0f;

    constexpr float jumpBufferDuration = 0.12f;
    constexpr float coyoteTimeDuration = 0.06f;

    Level level{
        "....................................................................................................................................................................................................................",
        "....................................................................................................................................................................................................................",
        "....................................................................................................................................................................................................................",
        "..................................................................................G.................................................................................................................................",
        "......................Q.........................................................BBBBBBBB...BBBQ..............M...........BBB....BQQB........................................................SS......................",
        "...........................................................................................................................................................................................SSS......................",
        "...............................................................................G..........................................................................................................SSSS......................",
        "................................................................I........................................................................................................................SSSSS......................",
        "................Q...BMBQB.....................Hh.........Hh..................BAB..............B.....BB....Q..Q..Q.....B..........BB......S..S..........SS..S............BBQB............SSSSSS......................",
        "......................................Hh......Vv.........Vv.............................................................................SS..SS........SSS..SS..........................SSSSSSS......................",
        "............................Hh........Vv......Vv.........Vv............................................................................SSS..SSS......SSSS..SSS.....Hh..............Hh.SSSSSSSS........F.............",
        "...P.................G......Vv........Vv.G....Vv.....G.G.Vv....................................G.G........K.................GG.G.G....SSSS..SSSS....SSSSS..SSSS....Vv.........GG...VvSSSSSSSSS........S.............",
        "#####################################################################..###############...################################################################..#########################################################",
        "#####################################################################..###############...################################################################..#########################################################",
        "#####################################################################..###############...################################################################..#########################################################",
        "#####################################################################..###############...################################################################..#########################################################"
    };

    NormaliseLevel(level);

    const Level initialLevel = level;

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
        LoadTexture("resources/Blocks/Empty_Block.png"),
        LoadTexture("resources/Blocks/Stair_Block.png"),
        LoadTexture("resources/Pipe.png"),
        LoadTexture("resources/Pipe_Base.png"),
        LoadTexture("resources/Coin.png"),
        LoadTexture("resources/Mario/Small_Mario.png"),
        LoadTexture("resources/Mario/Small_Mario_Walk_1.png"),
        LoadTexture("resources/Mario/Small_Mario_Walk_2.png"),
        LoadTexture("resources/Mario/Small_Mario_Walk_3.png"),
        LoadTexture("resources/Mario/Small_Mario_Jump.png"),
        LoadTexture("resources/Mario/Small_Mario_Switch.png"),
        LoadTexture("resources/Mario/Mario_Dead.png"),
        LoadTexture("resources/Mario/Super_Mario.png"),
        LoadTexture("resources/Mario/Super_Mario_Duck.png"),
        LoadTexture("resources/Mario/Super_Mario_Walk_1.png"),
        LoadTexture("resources/Mario/Super_Mario_Walk_2.png"),
        LoadTexture("resources/Mario/Super_Mario_Walk_3.png"),
        LoadTexture("resources/Mario/Super_Mario_Jump.png"),
        LoadTexture("resources/Mario/Super_Mario_Switch.png"),
        LoadTexture("resources/Mario/Fire_Mario.png"),
        LoadTexture("resources/Mario/Fire_Mario_Duck.png"),
        LoadTexture("resources/Mario/Fire_Mario_Walk_1.png"),
        LoadTexture("resources/Mario/Fire_Mario_Walk_2.png"),
        LoadTexture("resources/Mario/Fire_Mario_Walk_3.png"),
        LoadTexture("resources/Mario/Fire_Mario_Jump.png"),
        LoadTexture("resources/Mario/Fire_Mario_Switch.png"),
        LoadTexture("resources/Mario/Fire_Mario_Fire.png"),
        LoadTexture("resources/Enemies/Goomba.png"),
        LoadTexture("resources/Enemies/Green_Koopa.png"),
        LoadTexture("resources/Green_Shell.png"),
        LoadTexture("resources/Items/Super_Mushroom.png"),
        LoadTexture("resources/Items/1_Up_Mushroom.png"),
        LoadTexture("resources/Items/Fire_Flower.png"),
        LoadTexture("resources/Fire_Ball.png"),
        LoadTexture("resources/Flag.png")
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

    SetTextureFilter(textures.emptyBlock, TEXTURE_FILTER_POINT);

    SetTextureFilter(
        textures.stairBlock,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.pipe,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(textures.pipeBase, TEXTURE_FILTER_POINT);

    SetTextureFilter(
        textures.coin,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.player,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(textures.smallMarioWalk1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallMarioWalk2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallMarioWalk3, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallMarioJump, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallMarioSwitch, TEXTURE_FILTER_POINT);

    SetTextureFilter(textures.marioDead, TEXTURE_FILTER_POINT);

    SetTextureFilter(textures.superMario, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMarioDuck, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMarioWalk1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMarioWalk2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMarioWalk3, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMarioJump, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMarioSwitch, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMario, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMarioDuck, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMarioWalk1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMarioWalk2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMarioWalk3, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMarioJump, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMarioSwitch, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMarioThrow, TEXTURE_FILTER_POINT);

    SetTextureFilter(
        textures.goomba,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(textures.koopa, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.greenShell, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMushroom, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.oneUpMushroom, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireFlower, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireBall, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.flag, TEXTURE_FILTER_POINT);

    if (
        textures.ground.id == 0 ||
        textures.pipeBase.id == 0 ||
        textures.emptyBlock.id == 0 ||
        textures.player.id == 0 ||
        textures.smallMarioWalk1.id == 0 ||
        textures.smallMarioWalk2.id == 0 ||
        textures.smallMarioWalk3.id == 0 ||
        textures.smallMarioJump.id == 0 ||
        textures.smallMarioSwitch.id == 0 ||
        textures.marioDead.id == 0 ||
        textures.superMario.id == 0 ||
        textures.superMarioDuck.id == 0 ||
        textures.superMarioWalk1.id == 0 ||
        textures.superMarioWalk2.id == 0 ||
        textures.superMarioWalk3.id == 0 ||
        textures.superMarioJump.id == 0 ||
        textures.superMarioSwitch.id == 0 ||
        textures.fireMario.id == 0 ||
        textures.fireMarioDuck.id == 0 ||
        textures.fireMarioWalk1.id == 0 ||
        textures.fireMarioWalk2.id == 0 ||
        textures.fireMarioWalk3.id == 0 ||
        textures.fireMarioJump.id == 0 ||
        textures.fireMarioSwitch.id == 0 ||
        textures.fireMarioThrow.id == 0 ||
        textures.goomba.id == 0 ||
        textures.koopa.id == 0 ||
        textures.greenShell.id == 0 ||
        textures.superMushroom.id == 0 ||
        textures.oneUpMushroom.id == 0 ||
        textures.fireFlower.id == 0 ||
        textures.fireBall.id == 0 ||
        textures.flag.id == 0
    )
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load one or more game textures."
        );

        UnloadTexture(textures.ground);
        UnloadTexture(textures.pipeBase);
        UnloadTexture(textures.emptyBlock);
        UnloadTexture(textures.player);
        UnloadTexture(textures.smallMarioWalk1);
        UnloadTexture(textures.smallMarioWalk2);
        UnloadTexture(textures.smallMarioWalk3);
        UnloadTexture(textures.smallMarioJump);
        UnloadTexture(textures.smallMarioSwitch);
        UnloadTexture(textures.marioDead);
        UnloadTexture(textures.superMario);
        UnloadTexture(textures.superMarioDuck);
        UnloadTexture(textures.superMarioWalk1);
        UnloadTexture(textures.superMarioWalk2);
        UnloadTexture(textures.superMarioWalk3);
        UnloadTexture(textures.superMarioJump);
        UnloadTexture(textures.superMarioSwitch);
        UnloadTexture(textures.fireMario);
        UnloadTexture(textures.fireMarioDuck);
        UnloadTexture(textures.fireMarioWalk1);
        UnloadTexture(textures.fireMarioWalk2);
        UnloadTexture(textures.fireMarioWalk3);
        UnloadTexture(textures.fireMarioJump);
        UnloadTexture(textures.fireMarioSwitch);
        UnloadTexture(textures.fireMarioThrow);
        UnloadTexture(textures.goomba);
        UnloadTexture(textures.koopa);
        UnloadTexture(textures.greenShell);
        UnloadTexture(textures.superMushroom);
        UnloadTexture(textures.oneUpMushroom);
        UnloadTexture(textures.fireFlower);
        UnloadTexture(textures.fireBall);
        UnloadTexture(textures.flag);

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
        false,
        false,
        false,
        0.0f,
        false,
        false,
        false,
        false,
        0.0f,
        0,
        0.0f,
        false
    };

    std::vector<SuperMushroom> mushrooms;
    std::vector<FireFlower> fireFlowers;
    std::vector<FireBall> fireBalls;

    std::vector<Coin> initialCoins =
        FindCoinSpawns(level);

    std::vector<Coin> coins =
        initialCoins;

    std::vector<BlockCoin> blockCoins;

    int coinCount = 0;

    std::vector<Goomba> initialGoombas =
        FindGoombaSpawns(level);

    std::vector<Goomba> goombas =
        initialGoombas;

    std::vector<Koopa> initialKoopas =
        FindKoopaSpawns(level);

    std::vector<Koopa> koopas =
        initialKoopas;

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

        player.invulnerabilityTimer = std::max(
            0.0f,
            player.invulnerabilityTimer - deltaTime
        );

        player.fireThrowTimer = std::max(
            0.0f,
            player.fireThrowTimer - deltaTime
        );

        UpdateBlockBumps(
            deltaTime,
            mushrooms,
            fireFlowers,
            goombas,
            koopas
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

        if (direction < 0.0f)
        {
            player.facingLeft = true;
        }
        else if (direction > 0.0f)
        {
            player.facingLeft = false;
        }

        bool wantsToDuck =
            IsKeyDown(KEY_S) ||
            IsKeyDown(KEY_DOWN);

        if (player.big)
        {
            if (wantsToDuck && wasOnGround)
            {
                SetPlayerDucking(player, true, level);
            }
            else if (!wantsToDuck)
            {
                SetPlayerDucking(player, false, level);
            }
        }

        if (player.ducking)
        {
            direction = 0.0f;
            player.sprinting = false;
        }

        bool sprintKeyDown =
            IsKeyDown(KEY_LEFT_SHIFT) ||
            IsKeyDown(KEY_RIGHT_SHIFT);

        if (!sprintKeyDown)
        {
            player.sprinting = false;
        }
        else if (wasOnGround && !player.ducking)
        {
            player.sprinting = true;
        }

        bool sprinting = player.sprinting;

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
            !player.dying &&
            player.fire &&
            !player.ducking &&
            IsKeyPressed(KEY_SPACE) &&
            fireBalls.size() < 2
        )
        {
            constexpr float fireBallSize = 24.0f;
            constexpr float fireBallSpeed = 720.0f;

            bool shootLeft = player.facingLeft;

            float fireBallX = shootLeft
                ? player.body.x - fireBallSize
                : player.body.x + player.body.width;

            fireBalls.push_back({
                {
                    fireBallX,
                    player.body.y + player.body.height * 0.35f,
                    fireBallSize,
                    fireBallSize
                },
                {
                    shootLeft ? -fireBallSpeed : fireBallSpeed,
                    540.0f
                },
                true,
                0.0f
            });

            player.fireThrowTimer = 0.14f;
        }

        if (
            !player.dying &&
            (
                (IsKeyPressed(KEY_SPACE) && !player.fire) ||
                IsKeyPressed(KEY_W)
            )
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

        player.skidding =
            !player.dying &&
            !player.ducking &&
            wasOnGround &&
            direction != 0.0f &&
            player.velocity.x * direction < 0.0f &&
            std::abs(player.velocity.x) > 5.0f;

        if (direction != 0.0f)
        {
            bool reversingDirection =
                player.velocity.x * direction < 0.0f;

            // SMB doubles its fractional friction while Mario faces
            // opposite his current movement direction, producing the
            // characteristic skid before acceleration resumes.
            float appliedAcceleration = reversingDirection
                ? currentAcceleration * 2.0f
                : currentAcceleration;

            player.velocity.x +=
                direction *
                appliedAcceleration *
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

        if (!player.dying)
        {
            coinCount += CollectCoins(player, coins);
        }

        // --------------------------------
        // Player physics
        // --------------------------------

        if (player.dying)
        {
            player.velocity.y += gravity * deltaTime;
            player.body.y += player.velocity.y * deltaTime;
        }
        else
        {
        bool jumpHeld =
            (IsKeyDown(KEY_SPACE) && !player.fire) ||
            IsKeyDown(KEY_W);

        float playerGravity = fallingGravity;

        if (player.velocity.y < 0.0f)
        {
            playerGravity = jumpHeld
                ? risingGravity
                : releasedJumpGravity;
        }

        player.velocity.y +=
            playerGravity * deltaTime;

        ResolvePlayerHorizontalCollisions(
            player,
            level,
            deltaTime
        );

        ResolvePlayerVerticalCollisions(
            player,
            level,
            mushrooms,
            fireFlowers,
            goombas,
            koopas,
            blockCoins,
            coinCount,
            deltaTime
        );

        UpdateBlockCoins(blockCoins, deltaTime);

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
        }

        bool marioWalking =
            !player.dying &&
            !player.ducking &&
            !player.skidding &&
            player.onGround &&
            std::abs(player.velocity.x) > 5.0f;

        if (marioWalking)
        {
            float speedRatio = std::clamp(
                std::abs(player.velocity.x) / sprintSpeed,
                0.0f,
                1.0f
            );

            float frameDuration =
                0.14f - speedRatio * 0.07f;

            player.walkAnimationTimer += deltaTime;

            while (player.walkAnimationTimer >= frameDuration)
            {
                player.walkAnimationTimer -= frameDuration;
                player.walkAnimationFrame =
                    (player.walkAnimationFrame + 1) % 3;
            }
        }
        else
        {
            player.walkAnimationTimer = 0.0f;
            player.walkAnimationFrame = 0;
        }

        // --------------------------------
        // Enemy physics
        // --------------------------------

		SpawnGoombasNearCamera(
			goombas,
			camera
		);

        SpawnKoopasNearCamera(koopas, camera);
		
        UpdateGoombas(
            goombas,
            level,
            deltaTime,
            gravity
        );

        UpdateKoopas(koopas, level, deltaTime, gravity);

        UpdateSuperMushrooms(
            mushrooms,
            level,
            deltaTime,
            gravity
        );

        if (!player.dying)
        {
            CollectSuperMushrooms(player, mushrooms);
        }

        UpdateFireFlowers(fireFlowers, deltaTime);
        if (!player.dying)
        {
            CollectFireFlowers(player, fireFlowers);
        }

        UpdateFireBalls(
            fireBalls,
            level,
            goombas,
            koopas,
            camera,
            deltaTime
        );

        HandleStationaryShellEnemyCollisions(
            koopas,
            goombas
        );

        HandleMovingShellEnemyCollisions(
            koopas,
            goombas
        );

        bool playerHitGoomba = !player.dying &&
            HandlePlayerGoombaCollisions(
                player,
                goombas,
                previousPlayerBottom
            );

        bool playerHitKoopa = !player.dying &&
            HandlePlayerKoopaCollisions(
                player,
                koopas,
                previousPlayerBottom
            );

        bool playerHitEnemy =
            playerHitGoomba || playerHitKoopa;

        if (playerHitEnemy && player.big)
        {
            constexpr float damageInvulnerability = 1.5f;

            float bottom =
                player.body.y + player.body.height;

            float centre =
                player.body.x + player.body.width / 2.0f;

            player.body.width = 32.0f;
            player.body.height = 44.0f;
            player.body.x = centre - player.body.width / 2.0f;
            player.body.y = bottom - player.body.height;
            player.big = false;
            player.fire = false;
            player.ducking = false;
            player.invulnerabilityTimer =
                damageInvulnerability;

            playerHitEnemy = false;
        }

        // --------------------------------
        // Respawn
        // --------------------------------

        float cameraBottom =
            camera.target.y +
            virtualHeight / (2.0f * camera.zoom);

        bool playerFell =
            !player.dying &&
            player.body.y > cameraBottom;

        if (playerHitEnemy)
        {
            KillPlayer(player);
        }

        bool deathAnimationFinished =
            player.dying &&
            player.velocity.y > 0.0f &&
            player.body.y > cameraBottom + 100.0f;

        if (playerFell || deathAnimationFinished)
        {
            RespawnPlayer(
                player,
                spawn,
                camera,
                level,
                initialLevel,
                coins,
                initialCoins,
                blockCoins,
                coinCount,
                goombas,
                initialGoombas,
                koopas,
                initialKoopas,
                mushrooms,
                fireFlowers,
                fireBalls,
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
            level,
            deltaTime
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

        bool cameraAtLevelStart =
            camera.target.x <= virtualWidth / 2.0f + 0.1f;

        if (cameraAtLevelStart)
        {
            float viewportLeft =
                camera.target.x -
                virtualWidth / (2.0f * camera.zoom);

            for (Goomba& goomba : goombas)
            {
                if (
                    goomba.alive &&
                    goomba.body.x + goomba.body.width < viewportLeft
                )
                {
                    goomba.alive = false;
                }
            }

            for (Koopa& koopa : koopas)
            {
                if (
                    koopa.alive &&
                    koopa.body.x + koopa.body.width < viewportLeft
                )
                {
                    koopa.alive = false;
                }
            }

            for (SuperMushroom& mushroom : mushrooms)
            {
                if (
                    !mushroom.collected &&
                    mushroom.body.x + mushroom.body.width < viewportLeft
                )
                {
                    mushroom.collected = true;
                }
            }
        }

        // --------------------------------
        // Draw virtual game screen
        // --------------------------------

        BeginTextureMode(gameTexture);

        ClearBackground(SKYBLUE);

        BeginMode2D(camera);

        // Draw items behind tiles so mushrooms rise out of blocks.
        DrawSuperMushrooms(
            mushrooms,
            textures.superMushroom,
            textures.oneUpMushroom
        );

        DrawFireFlowers(
            fireFlowers,
            textures.fireFlower
        );

        DrawLevel(
            level,
            textures
        );

        DrawCoins(
            coins,
            textures.coin
        );

        DrawBlockCoins(
            blockCoins,
            textures.coin
        );

        for (const Goomba& goomba : goombas)
        {
            DrawGoomba(
                goomba,
                textures.goomba
            );
        }


        for (const Koopa& koopa : koopas)
        {
            DrawKoopa(
                koopa,
                textures.koopa,
                textures.greenShell
            );
        }

        DrawFireBalls(
            fireBalls,
            textures.fireBall
        );

        DrawPlayer(
            player,
            textures
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


            for (const Koopa& koopa : koopas)
            {
                if (koopa.alive && koopa.spawned)
                {
                    DrawRectangleLinesEx(koopa.body, 2.0f, GREEN);
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
            player.fire
                ? "W: jump / Space: fire"
                : "Space or W: jump",
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
            120,
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
    UnloadTexture(textures.emptyBlock);
    UnloadTexture(textures.stairBlock);
    UnloadTexture(textures.pipe);
    UnloadTexture(textures.pipeBase);
    UnloadTexture(textures.coin);
    UnloadTexture(textures.player);
    UnloadTexture(textures.smallMarioWalk1);
    UnloadTexture(textures.smallMarioWalk2);
    UnloadTexture(textures.smallMarioWalk3);
    UnloadTexture(textures.smallMarioJump);
    UnloadTexture(textures.smallMarioSwitch);
    UnloadTexture(textures.marioDead);
    UnloadTexture(textures.superMario);
    UnloadTexture(textures.superMarioDuck);
    UnloadTexture(textures.superMarioWalk1);
    UnloadTexture(textures.superMarioWalk2);
    UnloadTexture(textures.superMarioWalk3);
    UnloadTexture(textures.superMarioJump);
    UnloadTexture(textures.superMarioSwitch);
    UnloadTexture(textures.fireMario);
    UnloadTexture(textures.fireMarioDuck);
    UnloadTexture(textures.fireMarioWalk1);
    UnloadTexture(textures.fireMarioWalk2);
    UnloadTexture(textures.fireMarioWalk3);
    UnloadTexture(textures.fireMarioJump);
    UnloadTexture(textures.fireMarioSwitch);
    UnloadTexture(textures.fireMarioThrow);
    UnloadTexture(textures.goomba);
    UnloadTexture(textures.koopa);
    UnloadTexture(textures.greenShell);
    UnloadTexture(textures.superMushroom);
    UnloadTexture(textures.oneUpMushroom);
    UnloadTexture(textures.fireFlower);
    UnloadTexture(textures.fireBall);
    UnloadTexture(textures.flag);

    UnloadRenderTexture(gameTexture);
    CloseWindow();

    return 0;
}
