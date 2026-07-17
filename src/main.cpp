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

constexpr float nesNtscFrameRate = 60.0988f;
constexpr float koopaShellReviveDuration =
    16.0f * 21.0f / nesNtscFrameRate;
constexpr float koopaShellWakeDuration =
    4.0f * 21.0f / nesNtscFrameRate;
constexpr float growthTransformDuration =
    59.0f / nesNtscFrameRate;
constexpr float growthAnimationDelay =
    7.0f / nesNtscFrameRate;
constexpr float marioDeathSoundDuration = 2.7123f;
constexpr float stageClearSoundDuration = 5.6415f;

struct GameSounds
{
    Sound oneUp;
    Sound breakBlock;
    Sound bump;
    Sound coin;
    Sound fireBall;
    Sound flagPole;
    Sound jumpSmall;
    Sound jumpSuper;
    Sound kick;
    Sound marioDie;
    Sound pipe;
    Sound powerUp;
    Sound powerUpAppears;
    Sound stageClear;
    Sound stomp;
};

GameSounds gameSounds{};

void PlayGameSound(const Sound& sound)
{
    if (IsSoundValid(sound))
    {
        PlaySound(sound);
    }
}

bool AreGameSoundsLoaded(const GameSounds& sounds)
{
    return
        IsSoundValid(sounds.oneUp) &&
        IsSoundValid(sounds.breakBlock) &&
        IsSoundValid(sounds.bump) &&
        IsSoundValid(sounds.coin) &&
        IsSoundValid(sounds.fireBall) &&
        IsSoundValid(sounds.flagPole) &&
        IsSoundValid(sounds.jumpSmall) &&
        IsSoundValid(sounds.jumpSuper) &&
        IsSoundValid(sounds.kick) &&
        IsSoundValid(sounds.marioDie) &&
        IsSoundValid(sounds.pipe) &&
        IsSoundValid(sounds.powerUp) &&
        IsSoundValid(sounds.powerUpAppears) &&
        IsSoundValid(sounds.stageClear) &&
        IsSoundValid(sounds.stomp);
}

void UnloadGameSounds(const GameSounds& sounds)
{
    const Sound allSounds[] = {
        sounds.oneUp,
        sounds.breakBlock,
        sounds.bump,
        sounds.coin,
        sounds.fireBall,
        sounds.flagPole,
        sounds.jumpSmall,
        sounds.jumpSuper,
        sounds.kick,
        sounds.marioDie,
        sounds.pipe,
        sounds.powerUp,
        sounds.powerUpAppears,
        sounds.stageClear,
        sounds.stomp
    };

    for (const Sound& sound : allSounds)
    {
        if (IsSoundValid(sound))
        {
            UnloadSound(sound);
        }
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
    bool star;
    float starTimer;
    float damageTransformTimer;
    float growthTransformTimer;
    float fireTransformTimer;
};

bool IsPlayerTransforming(const Player& player)
{
    return
        player.growthTransformTimer > 0.0f ||
        player.fireTransformTimer > 0.0f;
}

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

struct SuperStar
{
    Rectangle body;
    Vector2 velocity;
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
    bool impacting;
    float impactTimer;
};

struct Goomba
{
    Rectangle body;
    Vector2 velocity;
    bool onGround;
    bool alive;
    bool spawned;
    bool dying;
    bool stomped;
    float deathTimer;
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
    float shellReviveTimer;
};

void KillGoomba(Goomba& goomba)
{
    if (goomba.alive && !goomba.dying)
    {
        PlayGameSound(gameSounds.kick);
        goomba.dying = true;
        goomba.stomped = false;
        goomba.velocity = {0.0f, -240.0f};
    }
}

void KillKoopa(Koopa& koopa)
{
    if (koopa.alive && !koopa.dying)
    {
        PlayGameSound(gameSounds.kick);
        koopa.dying = true;
        koopa.velocity = {0.0f, -240.0f};
    }
}

void KillPlayer(Player& player)
{
    if (!player.dying)
    {
        PlayGameSound(gameSounds.marioDie);
        player.dying = true;
        player.velocity = {0.0f, -650.0f};
        player.onGround = false;
        player.sprinting = false;
        player.ducking = false;
        player.fireThrowTimer = 0.0f;
        player.skidding = false;
        player.star = false;
        player.starTimer = 0.0f;
        player.damageTransformTimer = 0.0f;
        player.growthTransformTimer = 0.0f;
        player.fireTransformTimer = 0.0f;
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

struct BrickFragment
{
    Vector2 position;
    Vector2 velocity;
};

struct BrokenBrickAnimation
{
    Rectangle originalBlock;
    BrickFragment fragments[4];
    float age;
    bool blue;
};

std::vector<BrokenBrickAnimation> brokenBrickAnimations;

struct MultiCoinBlock
{
    int row;
    int column;
    int coinsRemaining;
    float timeRemaining;
    bool finalCoinOnly;
};

std::vector<MultiCoinBlock> multiCoinBlocks;

struct StarMarioTextures
{
    Texture2D small;
    Texture2D smallJump;
    Texture2D smallSwitch;
    Texture2D smallWalk1;
    Texture2D smallWalk2;
    Texture2D smallWalk3;
    Texture2D super;
    Texture2D superDuck;
    Texture2D superFire;
    Texture2D superJump;
    Texture2D superSwitch;
    Texture2D superWalk1;
    Texture2D superWalk2;
    Texture2D superWalk3;
};

struct GameTextures
{
    Texture2D ground;
    Texture2D brick;
    Texture2D brokenBrick;
    Texture2D brickPiece1;
    Texture2D brickPiece2;
    Texture2D blueGround;
    Texture2D blueBrick;
    Texture2D blueBrokenBrick;
    Texture2D questionBlock1;
    Texture2D questionBlock2;
    Texture2D questionBlock3;
    Texture2D emptyBlock;
    Texture2D stairBlock;
    Texture2D pipe;
    Texture2D pipeBase;
    Texture2D undergroundPipeConnector;
    Texture2D undergroundPipeBase;
    Texture2D coin;
    Texture2D coinFlash1;
    Texture2D coinFlash2;
    Texture2D coinFlash3;
    Texture2D coinFlash4;
    Texture2D blueCoin1;
    Texture2D blueCoin2;
    Texture2D blueCoin3;
    Texture2D bigHill;
    Texture2D smallHill;
    Texture2D singleBush;
    Texture2D doubleBush;
    Texture2D tripleBush;
    Texture2D singleCloud;
    Texture2D doubleCloud;
    Texture2D tripleCloud;
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
    Texture2D goombaDead;
    Texture2D koopa;
    Texture2D koopaWalk;
    Texture2D greenShell;
    Texture2D greenShellWake;
    Texture2D superMushroom;
    Texture2D oneUpMushroom;
    Texture2D fireFlower1;
    Texture2D fireFlower2;
    Texture2D fireFlower3;
    Texture2D fireFlower4;
    Texture2D star1;
    Texture2D star2;
    Texture2D star3;
    Texture2D star4;
    StarMarioTextures brownStarMario;
    StarMarioTextures greenStarMario;
    StarMarioTextures redStarMario;
    Texture2D fireBall;
    Texture2D fireBallHit1;
    Texture2D fireBallHit2;
    Texture2D fireBallHit3;
    Texture2D flag;
    Texture2D flagPole;
    Texture2D smallMarioFlag1;
    Texture2D smallMarioFlag2;
    Texture2D superMarioFlag1;
    Texture2D superMarioFlag2;
    Texture2D fireMarioFlag1;
    Texture2D fireMarioFlag2;
    Texture2D castle;
    Texture2D title;
};

void SetStarMarioTextureFilter(const StarMarioTextures& textures)
{
    SetTextureFilter(textures.small, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallJump, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallSwitch, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallWalk1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallWalk2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallWalk3, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.super, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superDuck, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superFire, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superJump, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superSwitch, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superWalk1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superWalk2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superWalk3, TEXTURE_FILTER_POINT);
}

bool AreStarMarioTexturesLoaded(const StarMarioTextures& textures)
{
    return textures.small.id != 0 &&
        textures.smallJump.id != 0 &&
        textures.smallSwitch.id != 0 &&
        textures.smallWalk1.id != 0 &&
        textures.smallWalk2.id != 0 &&
        textures.smallWalk3.id != 0 &&
        textures.super.id != 0 &&
        textures.superDuck.id != 0 &&
        textures.superFire.id != 0 &&
        textures.superJump.id != 0 &&
        textures.superSwitch.id != 0 &&
        textures.superWalk1.id != 0 &&
        textures.superWalk2.id != 0 &&
        textures.superWalk3.id != 0;
}

void UnloadStarMarioTextures(const StarMarioTextures& textures)
{
    UnloadTexture(textures.small);
    UnloadTexture(textures.smallJump);
    UnloadTexture(textures.smallSwitch);
    UnloadTexture(textures.smallWalk1);
    UnloadTexture(textures.smallWalk2);
    UnloadTexture(textures.smallWalk3);
    UnloadTexture(textures.super);
    UnloadTexture(textures.superDuck);
    UnloadTexture(textures.superFire);
    UnloadTexture(textures.superJump);
    UnloadTexture(textures.superSwitch);
    UnloadTexture(textures.superWalk1);
    UnloadTexture(textures.superWalk2);
    UnloadTexture(textures.superWalk3);
}

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
        tile == 'X' ||
        tile == 'B' ||
        tile == 'b' ||
        tile == 'N' ||
        tile == 'Q' ||
        tile == 'M' ||
        tile == 'A' ||
        tile == 'T' ||
        tile == 'E' ||
        tile == 'S' ||
        tile == 'H' ||
        tile == 'h' ||
        tile == 'V' ||
        tile == 'v' ||
        tile == 'R' ||
        tile == 'r';
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
    std::vector<SuperStar>& stars,
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

        for (SuperStar& star : stars)
        {
            if (!star.collected && !star.emerging)
            {
                carryBody(star.body);
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

void StartBrokenBrickAnimation(const Rectangle& block, bool blue = false)
{
    PlayGameSound(gameSounds.breakBlock);

    // Original units, scaled from the NES's 16-pixel tile to 48 pixels:
    // horizontal speed is 1 px/frame, while the two rows launch at
    // 6 px/frame and 4 px/frame respectively.
    constexpr float horizontalSpeed = 3.0f * nesNtscFrameRate;
    constexpr float upperLaunchSpeed = -6.0f * 3.0f * nesNtscFrameRate;
    constexpr float lowerLaunchSpeed = -4.0f * 3.0f * nesNtscFrameRate;
    constexpr float rightFragmentOffset = 6.0f * 3.0f;
    constexpr float lowerFragmentOffset = 8.0f * 3.0f;

    brokenBrickAnimations.push_back({
        block,
        {
            {{block.x, block.y}, {-horizontalSpeed, upperLaunchSpeed}},
            {{block.x + rightFragmentOffset, block.y},
                {horizontalSpeed, upperLaunchSpeed}},
            {{block.x, block.y + lowerFragmentOffset},
                {-horizontalSpeed, lowerLaunchSpeed}},
            {{block.x + rightFragmentOffset, block.y + lowerFragmentOffset},
                {horizontalSpeed, lowerLaunchSpeed}}
        },
        0.0f,
        blue
    });
}

void UpdateBrokenBrickAnimations(float deltaTime)
{
    // The NES adds $50/256 px of downward force per frame and caps
    // falling chunks at 8 px/frame.
    constexpr float fragmentGravity =
        (80.0f / 256.0f) * 3.0f *
        nesNtscFrameRate * nesNtscFrameRate;
    constexpr float maximumFallSpeed =
        8.0f * 3.0f * nesNtscFrameRate;
    constexpr float maximumLifetime = 2.0f;

    for (BrokenBrickAnimation& animation : brokenBrickAnimations)
    {
        animation.age += deltaTime;

        for (BrickFragment& fragment : animation.fragments)
        {
            fragment.velocity.y = std::min(
                maximumFallSpeed,
                fragment.velocity.y + fragmentGravity * deltaTime
            );
            fragment.position.x += fragment.velocity.x * deltaTime;
            fragment.position.y += fragment.velocity.y * deltaTime;
        }
    }

    std::erase_if(
        brokenBrickAnimations,
        [maximumLifetime](const BrokenBrickAnimation& animation)
        {
            return animation.age >= maximumLifetime;
        }
    );
}

bool WasBrickJustBrokenAt(int row, int column)
{
    // A broken brick should not immediately become a "helpful" opening for
    // ceiling corner correction.  Keep it logically occupied for the short
    // upward rebound caused by the same hit; later jumps can use the gap.
    constexpr float suppressionDuration = 0.3f;

    return std::ranges::any_of(
        brokenBrickAnimations,
        [row, column, suppressionDuration](
            const BrokenBrickAnimation& animation
        )
        {
            int brokenColumn = static_cast<int>(std::lround(
                animation.originalBlock.x / tileSize
            ));
            int brokenRow = static_cast<int>(std::lround(
                animation.originalBlock.y / tileSize
            ));

            return animation.age < suppressionDuration &&
                brokenRow == row &&
                brokenColumn == column;
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
    const Level& level,
    bool fullTile = false
)
{
    std::vector<Coin> coins;

    float coinWidth = fullTile ? tileSize : 24.0f;
    float coinHeight = fullTile ? tileSize : 32.0f;

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
				false,
				false,
				0.0f
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
                false,
                0.0f
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

    if (collectedThisFrame > 0)
    {
        PlayGameSound(gameSounds.coin);
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

void SpawnBlockCoin(
    std::vector<BlockCoin>& blockCoins,
    const Rectangle& block
)
{
    constexpr float coinSize = static_cast<float>(tileSize);

    blockCoins.push_back({
        {
            block.x,
            block.y,
            coinSize,
            coinSize
        },
        block.y,
        0.0f
    });
}

void UpdateMultiCoinBlocks(float deltaTime)
{
    for (MultiCoinBlock& block : multiCoinBlocks)
    {
        if (block.finalCoinOnly)
        {
            continue;
        }

        block.timeRemaining = std::max(
            0.0f,
            block.timeRemaining - deltaTime
        );

        if (block.timeRemaining <= 0.0f)
        {
            block.finalCoinOnly = true;
        }
    }
}

void UpdateSuperMushrooms(
    std::vector<SuperMushroom>& mushrooms,
    const Level& level,
    float deltaTime
)
{
    constexpr float emergeSpeed =
        0.25f * 3.0f * nesNtscFrameRate;
    constexpr float moveSpeed =
        3.0f * nesNtscFrameRate;
    constexpr float mushroomGravity =
        (61.0f / 256.0f) *
        3.0f * nesNtscFrameRate * nesNtscFrameRate;
    constexpr float maximumFallSpeed =
        3.0f * 3.0f * nesNtscFrameRate;

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
        mushroom.velocity.y = std::min(
            maximumFallSpeed,
            mushroom.velocity.y + mushroomGravity * deltaTime
        );
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
            PlayGameSound(gameSounds.oneUp);
            continue;
        }

        PlayGameSound(gameSounds.powerUp);

        if (!player.big)
        {
            player.growthTransformTimer = growthTransformDuration;
            player.sprinting = false;
            player.skidding = false;
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
        PlayGameSound(gameSounds.powerUp);

        if (!player.big)
        {
            player.fire = true;
            player.growthTransformTimer = growthTransformDuration;
            player.sprinting = false;
            player.skidding = false;
            player.ducking = false;
        }
        else if (!player.fire)
        {
            player.fireTransformTimer = growthTransformDuration;
            player.sprinting = false;
            player.skidding = false;
            player.ducking = false;
        }
    }
}

void UpdateSuperStars(
    std::vector<SuperStar>& stars,
    const Level& level,
    float deltaTime
)
{
    constexpr float emergeSpeed =
        0.25f * 3.0f * nesNtscFrameRate;
    constexpr float moveSpeed =
        3.0f * nesNtscFrameRate;
    constexpr float starGravity =
        (28.0f / 256.0f) *
        3.0f * nesNtscFrameRate * nesNtscFrameRate;
    constexpr float maximumFallSpeed =
        3.0f * 3.0f * nesNtscFrameRate;
    constexpr float bounceSpeed = maximumFallSpeed;

    for (SuperStar& star : stars)
    {
        if (star.collected)
        {
            continue;
        }

        if (star.emerging)
        {
            star.body.y = std::max(
                star.targetY,
                star.body.y - emergeSpeed * deltaTime
            );

            if (star.body.y <= star.targetY)
            {
                star.emerging = false;
                star.velocity.x = moveSpeed;
            }

            continue;
        }

        float previousX = star.body.x;
        star.body.x += star.velocity.x * deltaTime;

        for (const Rectangle& tile : GetNearbySolidTiles(level, star.body))
        {
            if (!CheckCollisionRecs(star.body, tile))
            {
                continue;
            }

            if (star.velocity.x > 0.0f && previousX + star.body.width <= tile.x)
            {
                star.body.x = tile.x - star.body.width;
                star.velocity.x = -moveSpeed;
            }
            else if (star.velocity.x < 0.0f && previousX >= tile.x + tile.width)
            {
                star.body.x = tile.x + tile.width;
                star.velocity.x = moveSpeed;
            }
        }

        float previousY = star.body.y;
        star.velocity.y = std::min(
            maximumFallSpeed,
            star.velocity.y + starGravity * deltaTime
        );
        star.body.y += star.velocity.y * deltaTime;

        for (const Rectangle& tile : GetNearbySolidTiles(level, star.body))
        {
            if (!CheckCollisionRecs(star.body, tile))
            {
                continue;
            }

            if (
                star.velocity.y > 0.0f &&
                previousY + star.body.height <= tile.y + 10.0f
            )
            {
                star.body.y = tile.y - star.body.height;
                star.velocity.y = -bounceSpeed;
            }
        }
    }
}

bool CollectSuperStars(Player& player, std::vector<SuperStar>& stars)
{
    constexpr float starDuration = 12.0f;
    bool collectedAny = false;

    for (SuperStar& star : stars)
    {
        if (
            star.collected ||
            star.emerging ||
            !CheckCollisionRecs(player.body, star.body)
        )
        {
            continue;
        }

        star.collected = true;
        collectedAny = true;
        PlayGameSound(gameSounds.powerUp);
        player.star = true;
        player.starTimer = starDuration;
    }

    return collectedAny;
}

void StartFireBallImpact(FireBall& fireBall)
{
    fireBall.impacting = true;
    fireBall.impactTimer = 0.0f;
    fireBall.velocity = {0.0f, 0.0f};
    fireBall.rotation = 0.0f;
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
    constexpr float impactDuration = 12.0f / nesNtscFrameRate;

    for (FireBall& fireBall : fireBalls)
    {
        if (!fireBall.alive)
        {
            continue;
        }

        if (fireBall.impacting)
        {
            fireBall.impactTimer += deltaTime;

            if (fireBall.impactTimer >= impactDuration)
            {
                fireBall.alive = false;
            }

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
                fireBall.body.x = previousX;
                StartFireBallImpact(fireBall);
                break;
            }
        }

        if (fireBall.impacting)
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
                fireBall.body.y = previousY;
                StartFireBallImpact(fireBall);
                break;
            }
        }

        if (fireBall.impacting)
        {
            continue;
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
                StartFireBallImpact(fireBall);
                break;
            }
        }

        for (Koopa& koopa : koopas)
        {
            if (
                !fireBall.impacting &&
                koopa.alive &&
                !koopa.dying &&
                koopa.spawned &&
                CheckCollisionRecs(fireBall.body, koopa.body)
            )
            {
                KillKoopa(koopa);
                StartFireBallImpact(fireBall);
                break;
            }
        }

        if (fireBall.impacting)
        {
            continue;
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
    std::vector<SuperStar>& stars,
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
                    Rectangle invisibleBlock =
                        GetTileRectangle(row, column);

                    float horizontalOverlap = std::max(
                        0.0f,
                        std::min(
                            player.body.x + player.body.width,
                            invisibleBlock.x + invisibleBlock.width
                        ) -
                        std::max(player.body.x, invisibleBlock.x)
                    );

                    // Hidden blocks in SMB only catch Mario when he is
                    // sufficiently centred beneath them. A glancing overlap
                    // of half his width or less passes through unrevealed.
                    if (horizontalOverlap > player.body.width / 2.0f)
                    {
                        nearbyTiles.push_back(invisibleBlock);
                    }
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

            bool allowCornerCorrection =
                level[ceilingRow][ceilingColumn] != 'I';

            bool gapOnLeft =
                ceilingColumn > 0 &&
                !IsSolidTile(
                    level,
                    ceilingRow,
                    ceilingColumn - 1
                ) &&
                !WasBrickJustBrokenAt(
                    ceilingRow,
                    ceilingColumn - 1
                );

            bool gapOnRight =
                ceilingColumn + 1 < GetLevelColumns(level) &&
                !IsSolidTile(
                    level,
                    ceilingRow,
                    ceilingColumn + 1
                ) &&
                !WasBrickJustBrokenAt(
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
                allowCornerCorrection &&
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
                allowCornerCorrection &&
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
                    hitTile == 'b' ||
                    hitTile == 'N' ||
                    hitTile == 'M' ||
                    hitTile == 'A' ||
                    hitTile == 'T' ||
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

                if (
                    (hitTile == 'B' || hitTile == 'b') &&
                    player.big
                )
                {
                    bool blueBrick = hitTile == 'b';
                    hitTile = '.';
                    StartBrokenBrickAnimation(tile, blueBrick);

                    // SMB gives Mario a small upward rebound when the
                    // brick gives way instead of stopping him dead.
                    player.velocity.y =
                        -2.0f * 3.0f * nesNtscFrameRate;
                }
                else if (hitTile == 'B' || hitTile == 'b')
                {
                    StartBlockBump(tileRow, tileColumn);
                    PlayGameSound(gameSounds.bump);
                }
                else if (hitTile == 'M')
                {
                    hitTile = 'E';
                    StartBlockBump(tileRow, tileColumn);
                    PlayGameSound(gameSounds.powerUpAppears);

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
                    PlayGameSound(gameSounds.powerUpAppears);

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
                    PlayGameSound(gameSounds.powerUpAppears);

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
                else if (hitTile == 'T')
                {
                    hitTile = 'E';
                    StartBlockBump(tileRow, tileColumn);
                    PlayGameSound(gameSounds.powerUpAppears);

                    constexpr float itemSize =
                        static_cast<float>(tileSize);
                    constexpr float hitboxWidth = itemSize + 2.0f;
                    constexpr float blockBumpHeight = 10.0f;

                    stars.push_back({
                        {
                            tile.x - 1.0f,
                            tile.y - blockBumpHeight,
                            hitboxWidth,
                            itemSize
                        },
                        {0.0f, 0.0f},
                        tile.y - itemSize,
                        true,
                        false
                    });
                }
                else if (hitTile == 'N')
                {
                    constexpr int maximumCoins = 10;
                    constexpr float activeDuration = 4.0f;

                    StartBlockBump(tileRow, tileColumn);
                    ++coinCount;
                    PlayGameSound(gameSounds.coin);
                    SpawnBlockCoin(blockCoins, tile);

                    auto activeBlock = std::find_if(
                        multiCoinBlocks.begin(),
                        multiCoinBlocks.end(),
                        [tileRow, tileColumn](const MultiCoinBlock& block)
                        {
                            return
                                block.row == tileRow &&
                                block.column == tileColumn;
                        }
                    );

                    if (activeBlock == multiCoinBlocks.end())
                    {
                        multiCoinBlocks.push_back({
                            tileRow,
                            tileColumn,
                            maximumCoins - 1,
                            activeDuration,
                            false
                        });
                    }
                    else
                    {
                        if (activeBlock->finalCoinOnly)
                        {
                            hitTile = 'E';
                            multiCoinBlocks.erase(activeBlock);
                        }
                        else
                        {
                            --activeBlock->coinsRemaining;

                            if (activeBlock->coinsRemaining <= 0)
                            {
                                hitTile = 'E';
                                multiCoinBlocks.erase(activeBlock);
                            }
                        }
                    }
                }
                else if (hitTile == 'Q')
                {
                    hitTile = 'E';
                    StartBlockBump(tileRow, tileColumn);
                    ++coinCount;
                    PlayGameSound(gameSounds.coin);

                    SpawnBlockCoin(blockCoins, tile);
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
            if (goomba.stomped)
            {
                goomba.deathTimer -= deltaTime;

                if (goomba.deathTimer <= 0.0f)
                {
                    goomba.alive = false;
                }

                continue;
            }

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
    // SMB uses a value of $10 in an interval timer that advances once
    // every 21 NTSC frames. The shell changes to its wake-up graphic
    // below $05, during the final four intervals.
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

        if (koopa.shelled)
        {
            koopa.shellReviveTimer = std::max(
                0.0f,
                koopa.shellReviveTimer - deltaTime
            );

            bool stationaryShell =
                std::abs(koopa.velocity.x) <= 1.0f;

            if (
                stationaryShell &&
                koopa.shellReviveTimer <= 0.0f
            )
            {
                float feet = koopa.body.y + koopa.body.height;
                float centre = koopa.body.x + koopa.body.width / 2.0f;

                koopa.shelled = false;
                koopa.body = {
                    centre - 18.0f,
                    feet - 48.0f,
                    36.0f,
                    48.0f
                };

                int nesFrame = static_cast<int>(GetTime() * nesNtscFrameRate);
                koopa.velocity.x = (nesFrame & 1) == 0 ? -90.0f : 90.0f;
            }
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

void HitEnemiesAsStarMario(
    const Player& player,
    std::vector<Goomba>& goombas,
    std::vector<Koopa>& koopas
)
{
    if (!player.star)
    {
        return;
    }

    for (Goomba& goomba : goombas)
    {
        if (
            goomba.alive && goomba.spawned && !goomba.dying &&
            CheckCollisionRecs(player.body, goomba.body)
        )
        {
            KillGoomba(goomba);
        }
    }

    for (Koopa& koopa : koopas)
    {
        if (
            koopa.alive && koopa.spawned && !koopa.dying &&
            CheckCollisionRecs(player.body, koopa.body)
        )
        {
            KillKoopa(koopa);
        }
    }
}

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
            goomba.dying = true;
            goomba.stomped = true;
            goomba.deathTimer = 0.4f;
            goomba.velocity = {0.0f, 0.0f};

            player.body.y =
                goomba.body.y -
                player.body.height;

            player.velocity.y =
                -stompBounceSpeed;

            player.onGround = false;
            PlayGameSound(gameSounds.stomp);

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
            koopa.shellReviveTimer = koopaShellReviveDuration;

            player.body.y = koopa.body.y - player.body.height;
            player.velocity.y = -stompBounceSpeed;
            player.onGround = false;
            PlayGameSound(gameSounds.stomp);
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
                koopa.shellReviveTimer = koopaShellReviveDuration;
                PlayGameSound(gameSounds.stomp);
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
                PlayGameSound(gameSounds.kick);
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
            PlayGameSound(gameSounds.kick);
        }
        else if (smallest == overlapRight)
        {
            player.body.x = koopa.body.x + koopa.body.width;
            player.velocity.x = 0.0f;
            koopa.velocity.x = -shellSpeed;
            PlayGameSound(gameSounds.kick);
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

void HandleOrdinaryEnemyCollisions(
    std::vector<Goomba>& goombas,
    std::vector<Koopa>& koopas,
    const Camera2D& camera
)
{
    float halfVisibleWidth =
        virtualWidth / (2.0f * camera.zoom);
    float halfVisibleHeight =
        virtualHeight / (2.0f * camera.zoom);
    float viewportLeft = camera.target.x - halfVisibleWidth;
    float viewportRight = camera.target.x + halfVisibleWidth;
    float viewportTop = camera.target.y - halfVisibleHeight;
    float viewportBottom = camera.target.y + halfVisibleHeight;

    auto fullyVisible = [=](const Rectangle& body)
    {
        return
            body.x >= viewportLeft &&
            body.x + body.width <= viewportRight &&
            body.y >= viewportTop &&
            body.y + body.height <= viewportBottom;
    };

    auto turnBothAround = [](
        Rectangle& firstBody,
        Vector2& firstVelocity,
        Rectangle& secondBody,
        Vector2& secondVelocity
    )
    {
        if (!CheckCollisionRecs(firstBody, secondBody))
        {
            return;
        }

        // SMB remembers a contact until the bounding boxes separate so the
        // enemies only reverse once. Separating the overlapping bodies here
        // provides the same one-contact response without persistent pair bits.
        float overlap = std::min(
            firstBody.x + firstBody.width,
            secondBody.x + secondBody.width
        ) - std::max(firstBody.x, secondBody.x);

        float firstCentre = firstBody.x + firstBody.width / 2.0f;
        float secondCentre = secondBody.x + secondBody.width / 2.0f;
        float firstCorrection = overlap / 2.0f;
        float secondCorrection = overlap - firstCorrection;

        if (firstCentre <= secondCentre)
        {
            firstBody.x -= firstCorrection;
            secondBody.x += secondCorrection;
        }
        else
        {
            firstBody.x += secondCorrection;
            secondBody.x -= firstCorrection;
        }

        firstVelocity.x = -firstVelocity.x;
        secondVelocity.x = -secondVelocity.x;
    };

    for (std::size_t first = 0; first < goombas.size(); ++first)
    {
        Goomba& firstGoomba = goombas[first];
        if (
            !firstGoomba.alive ||
            firstGoomba.dying ||
            !firstGoomba.spawned ||
            !fullyVisible(firstGoomba.body)
        )
        {
            continue;
        }

        for (std::size_t second = first + 1; second < goombas.size(); ++second)
        {
            Goomba& secondGoomba = goombas[second];
            if (
                secondGoomba.alive &&
                !secondGoomba.dying &&
                secondGoomba.spawned &&
                fullyVisible(secondGoomba.body)
            )
            {
                turnBothAround(
                    firstGoomba.body,
                    firstGoomba.velocity,
                    secondGoomba.body,
                    secondGoomba.velocity
                );
            }
        }

        for (Koopa& koopa : koopas)
        {
            if (
                koopa.alive &&
                !koopa.dying &&
                koopa.spawned &&
                !koopa.shelled &&
                fullyVisible(koopa.body)
            )
            {
                turnBothAround(
                    firstGoomba.body,
                    firstGoomba.velocity,
                    koopa.body,
                    koopa.velocity
                );
            }
        }
    }

    for (std::size_t first = 0; first < koopas.size(); ++first)
    {
        Koopa& firstKoopa = koopas[first];
        if (
            !firstKoopa.alive ||
            firstKoopa.dying ||
            !firstKoopa.spawned ||
            firstKoopa.shelled ||
            !fullyVisible(firstKoopa.body)
        )
        {
            continue;
        }

        for (std::size_t second = first + 1; second < koopas.size(); ++second)
        {
            Koopa& secondKoopa = koopas[second];
            if (
                secondKoopa.alive &&
                !secondKoopa.dying &&
                secondKoopa.spawned &&
                !secondKoopa.shelled &&
                fullyVisible(secondKoopa.body)
            )
            {
                turnBothAround(
                    firstKoopa.body,
                    firstKoopa.velocity,
                    secondKoopa.body,
                    secondKoopa.velocity
                );
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
    const GameTextures& textures,
    bool underground,
    float flagY
)
{
    const Texture2D* questionBlockFrames[] = {
        &textures.questionBlock1,
        &textures.questionBlock2,
        &textures.questionBlock3
    };
    // SMB updates this palette every eight frames, using a six-entry
    // table whose first colour appears three times: 1, 1, 1, 2, 3, 2.
    constexpr int questionBlockSequence[] = {0, 0, 0, 1, 2, 1};
    int nesFrame = static_cast<int>(GetTime() * nesNtscFrameRate);
    int sequenceFrame = (nesFrame / 8) % 6;
    const Texture2D& questionBlockTexture =
        *questionBlockFrames[questionBlockSequence[sequenceFrame]];

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

                case 'X':
                    DrawTextureAsTile(
                        textures.blueGround,
                        destination
                    );
                    break;

                case 'B':
                case 'N':
                    DrawTextureAsTile(
                        textures.brick,
                        destination
                    );
                    break;

                case 'b':
                    DrawTextureAsTile(
                        textures.blueBrick,
                        destination
                    );
                    break;

                case 'Q':
                case 'M':
                case 'A':
                    DrawTextureAsTile(
                        questionBlockTexture,
                        destination
                    );
                    break;

                case 'T':
                    DrawTextureAsTile(
                        textures.brick,
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

                    Rectangle poleSource{
                        0.0f,
                        0.0f,
                        static_cast<float>(textures.flagPole.width),
                        static_cast<float>(textures.flagPole.height)
                    };

                    Rectangle poleDestination{
                        destination.x - 8.0f * flagScale,
                        destination.y + tileSize * 2.0f -
                            textures.flagPole.height * flagScale,
                        textures.flagPole.width * flagScale,
                        textures.flagPole.height * flagScale
                    };

                    DrawTexturePro(
                        textures.flagPole,
                        poleSource,
                        poleDestination,
                        {0.0f, 0.0f},
                        0.0f,
                        WHITE
                    );

                    Rectangle flagSource{
                        0.0f,
                        0.0f,
                        static_cast<float>(textures.flag.width),
                        static_cast<float>(textures.flag.height)
                    };

                    Rectangle flagDestination{
                        destination.x - 8.0f * flagScale,
                        flagY,
                        textures.flag.width * flagScale,
                        textures.flag.height * flagScale
                    };

                    DrawTexturePro(
                        textures.flag,
                        flagSource,
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
                    bool coveredByUndergroundConnector =
                        underground &&
                        column >= 2 &&
                        (
                            level[row][column - 2] == 'R' ||
                            (
                                row > 0 &&
                                level[row - 1][column - 2] == 'R'
                            )
                        );

                    if (coveredByUndergroundConnector)
                    {
                        break;
                    }

                    Rectangle pipeBaseDestination{
                        destination.x,
                        destination.y,
                        tileSize * 2.0f,
                        static_cast<float>(tileSize)
                    };

                    DrawTextureAsTile(
                        underground
                            ? textures.undergroundPipeBase
                            : textures.pipeBase,
                        pipeBaseDestination
                    );
                    break;
                }

                case 'R':
                {
                    Rectangle connectorDestination{
                        destination.x,
                        destination.y,
                        tileSize * 4.0f,
                        tileSize * 2.0f
                    };

                    DrawTextureAsTile(
                        textures.undergroundPipeConnector,
                        connectorDestination
                    );
                    break;
                }

                case 'r':
                    break;

                default:
                    break;
            }
        }
    }
}

void DrawBackgroundScenery(
    const Level& level,
    const GameTextures& textures
)
{
    // SMB's first background-scene pattern repeats every 48 original
    // 16-pixel columns.  This clone omits the NES HUD row, so reference-map
    // Y coordinates are shifted upward by one original tile.
    constexpr float spriteScale = 3.0f;
    constexpr float sceneryCycleWidth = 768.0f * spriteScale;
    constexpr float verticalMapOffset = 16.0f;
    constexpr float horizontalWorldOffset = 8.0f * tileSize;

    auto drawScenerySprite = [spriteScale, horizontalWorldOffset](
        const Texture2D& texture,
        float sourceX,
        float sourceY
    )
    {
        Rectangle source{
            0.0f,
            0.0f,
            static_cast<float>(texture.width),
            static_cast<float>(texture.height)
        };
        Rectangle destination{
            sourceX * spriteScale + horizontalWorldOffset,
            (sourceY - verticalMapOffset) * spriteScale,
            texture.width * spriteScale,
            texture.height * spriteScale
        };

        DrawTexturePro(
            texture,
            source,
            destination,
            {0.0f, 0.0f},
            0.0f,
            WHITE
        );
    };

    for (
        float cycleX = 0.0f;
        cycleX < GetLevelWidth(level);
        cycleX += sceneryCycleWidth
    )
    {
        float sourceCycleX = cycleX / spriteScale;

        drawScenerySprite(textures.bigHill, sourceCycleX, 173.0f);
        drawScenerySprite(
            textures.smallHill,
            sourceCycleX + 256.0f,
            189.0f
        );

        drawScenerySprite(
            textures.tripleBush,
            sourceCycleX + 184.0f,
            192.0f
        );
        drawScenerySprite(
            textures.singleBush,
            sourceCycleX + 376.0f,
            192.0f
        );
        drawScenerySprite(
            textures.doubleBush,
            sourceCycleX + 664.0f,
            192.0f
        );

        drawScenerySprite(
            textures.singleCloud,
            sourceCycleX + 136.0f,
            48.0f
        );
        drawScenerySprite(
            textures.singleCloud,
            sourceCycleX + 312.0f,
            32.0f
        );
        drawScenerySprite(
            textures.tripleCloud,
            sourceCycleX + 440.0f,
            48.0f
        );
        drawScenerySprite(
            textures.doubleCloud,
            sourceCycleX + 584.0f,
            32.0f
        );
    }
}

void DrawOpeningTitle(const Texture2D& texture)
{
    constexpr float scale = 3.0f;
    float width = texture.width * scale;
    float height = texture.height * scale;

    Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(texture.width),
        static_cast<float>(texture.height)
    };

    Rectangle destination{
        (virtualWidth - width) / 2.0f,
        (virtualHeight - height) / 2.0f - tileSize,
        width,
        height
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

void DrawCastle(const Texture2D& texture)
{
    // World 1-1 places the castle four columns after the flagpole. The
    // clone's course geometry is offset eight tiles from the NES map.
    constexpr int castleColumn = 210;
    constexpr int castleRow = 7;

    Rectangle destination{
        castleColumn * static_cast<float>(tileSize),
        castleRow * static_cast<float>(tileSize),
        tileSize * 5.0f,
        tileSize * 5.0f
    };

    DrawTextureAsTile(texture, destination);
}

void DrawBrokenBrickAnimations(
    const std::vector<BrokenBrickAnimation>& animations,
    const Texture2D& brokenBrick,
    const Texture2D& blueBrokenBrick,
    const Texture2D& piece1,
    const Texture2D& piece2
)
{
    constexpr float impactFrameDuration = 1.0f / nesNtscFrameRate;
    constexpr float fragmentSize = 8.0f * 3.0f;
    Rectangle pieceSource{4.0f, 4.0f, 8.0f, 8.0f};

    // The NES rotates every chunk in sync using bits 3-2 of its frame
    // counter, so the diagonal pose changes every four frames.
    int nesFrame = static_cast<int>(GetTime() * nesNtscFrameRate);
    const Texture2D& pieceTexture =
        ((nesFrame / 4) & 1) == 0 ? piece1 : piece2;

    for (const BrokenBrickAnimation& animation : animations)
    {
        if (animation.age < impactFrameDuration)
        {
            const Texture2D& impactTexture = animation.blue
                ? blueBrokenBrick
                : brokenBrick;
            Rectangle source{
                0.0f,
                0.0f,
                static_cast<float>(impactTexture.width),
                static_cast<float>(impactTexture.height)
            };
            DrawTextureInsideRectangle(
                impactTexture,
                source,
                animation.originalBlock
            );
            continue;
        }

        for (int index = 0; index < 4; ++index)
        {
            const BrickFragment& fragment = animation.fragments[index];
            Rectangle destination{
                fragment.position.x,
                fragment.position.y,
                fragmentSize,
                fragmentSize
            };
            if (animation.blue)
            {
                Rectangle bluePieceSource{
                    static_cast<float>((index % 2) * 8),
                    static_cast<float>((index / 2) * 8),
                    8.0f,
                    8.0f
                };
                DrawTextureInsideRectangle(
                    blueBrokenBrick,
                    bluePieceSource,
                    destination
                );
            }
            else
            {
                DrawTextureInsideRectangle(
                    pieceTexture,
                    pieceSource,
                    destination
                );
            }
        }
    }
}

void DrawPlayer(
    const Player& player,
    const GameTextures& textures,
    bool forceIdle = false,
    int forcedStarPalette = -1
)
{
    if (
        !player.dying &&
        player.invulnerabilityTimer > 0.0f &&
        (static_cast<int>(GetTime() * nesNtscFrameRate) & 0x04) != 0
    )
    {
        return;
    }

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

    bool usingSpecialStarPalette = false;

    if (!player.dying && (player.star || forcedStarPalette >= 0))
    {
        // SMB cycles four sprite palettes from the global frame counter.
        // The final eight 21-frame intervals slow from every two frames
        // to every eight frames to warn that star power is ending.
        constexpr float slowCycleDuration =
            8.0f * 21.0f / nesNtscFrameRate;
        int palette = forcedStarPalette;

        if (palette < 0)
        {
            int framesPerColour =
                player.starTimer <= slowCycleDuration ? 8 : 2;
            int nesFrame = static_cast<int>(GetTime() * nesNtscFrameRate);
            palette = (nesFrame / framesPerColour) & 0x03;
        }

        const StarMarioTextures* starTextures = nullptr;

        if (palette == 1)
        {
            starTextures = &textures.greenStarMario;
        }
        else if (palette == 2)
        {
            starTextures = &textures.redStarMario;
        }
        else if (palette == 3)
        {
            starTextures = &textures.brownStarMario;
        }

        if (starTextures != nullptr)
        {
            usingSpecialStarPalette = true;
            const Texture2D* starSmallWalkTextures[] = {
                &starTextures->smallWalk1,
                &starTextures->smallWalk2,
                &starTextures->smallWalk3
            };
            const Texture2D* starSuperWalkTextures[] = {
                &starTextures->superWalk1,
                &starTextures->superWalk2,
                &starTextures->superWalk3
            };
            int frame = player.walkAnimationFrame % 3;

            if (player.big)
            {
                if (forceIdle)
                {
                    texture = &starTextures->super;
                }
                else if (player.ducking)
                {
                    texture = &starTextures->superDuck;
                }
                else if (player.fireThrowTimer > 0.0f && player.fire)
                {
                    texture = &starTextures->superFire;
                }
                else if (!player.onGround)
                {
                    texture = &starTextures->superJump;
                }
                else if (player.skidding)
                {
                    texture = &starTextures->superSwitch;
                }
                else if (std::abs(player.velocity.x) > 5.0f)
                {
                    texture = starSuperWalkTextures[frame];
                }
                else
                {
                    texture = &starTextures->super;
                }
            }
            else if (forceIdle)
            {
                texture = &starTextures->small;
            }
            else if (!player.onGround)
            {
                texture = &starTextures->smallJump;
            }
            else if (player.skidding)
            {
                texture = &starTextures->smallSwitch;
            }
            else if (std::abs(player.velocity.x) > 5.0f)
            {
                texture = starSmallWalkTextures[frame];
            }
            else
            {
                texture = &starTextures->small;
            }
        }
    }

    if (player.dying)
    {
        texture = &textures.marioDead;
    }
    else if (usingSpecialStarPalette)
    {
        // The state-specific special-palette texture was chosen above.
    }
    else if (forceIdle)
    {
        texture = player.fire
            ? &textures.fireMario
            : (player.big
                ? &textures.superMario
                : &textures.player);
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

void DrawPlayerOnFlagPole(
    const Player& player,
    const GameTextures& textures,
    bool alternateFrame,
    bool flippedHorizontally
)
{
    const Texture2D* texture = nullptr;

    if (player.fire)
    {
        texture = alternateFrame
            ? &textures.fireMarioFlag2
            : &textures.fireMarioFlag1;
    }
    else if (player.big)
    {
        texture = alternateFrame
            ? &textures.superMarioFlag2
            : &textures.superMarioFlag1;
    }
    else
    {
        texture = alternateFrame
            ? &textures.smallMarioFlag2
            : &textures.smallMarioFlag1;
    }

    Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(texture->width),
        static_cast<float>(texture->height)
    };

    constexpr float marioSpriteScale = 3.0f;
    float spriteWidth = texture->width * marioSpriteScale;
    float spriteHeight = texture->height * marioSpriteScale;
    Rectangle destination{
        player.body.x + (player.body.width - spriteWidth) / 2.0f,
        player.body.y + player.body.height - spriteHeight,
        spriteWidth,
        spriteHeight
    };

    DrawTextureInsideRectangle(
        *texture,
        source,
        destination,
        flippedHorizontally
    );
}

void DrawFireFlowers(
    const std::vector<FireFlower>& flowers,
    const Texture2D& texture1,
    const Texture2D& texture2,
    const Texture2D& texture3,
    const Texture2D& texture4
)
{
    const Texture2D* frames[] = {
        &texture1, &texture2, &texture3, &texture4
    };
    int frame = static_cast<int>(GetTime() * 10.0) % 4;
    const Texture2D& texture = *frames[frame];
    Rectangle source = GetFirstSquareFrame(texture);

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

void DrawSuperStars(
    const std::vector<SuperStar>& stars,
    const Texture2D& texture1,
    const Texture2D& texture2,
    const Texture2D& texture3,
    const Texture2D& texture4
)
{
    const Texture2D* frames[] = {
        &texture1, &texture2, &texture3, &texture4
    };
    int frame = static_cast<int>(GetTime() * 12.0) % 4;
    const Texture2D& texture = *frames[frame];
    Rectangle source = GetFirstSquareFrame(texture);

    for (const SuperStar& star : stars)
    {
        if (!star.collected)
        {
            Rectangle spriteBody = star.body;
            spriteBody.x += 1.0f;
            spriteBody.width -= 2.0f;
            DrawTextureInsideRectangle(texture, source, spriteBody);
        }
    }
}

void DrawFireBalls(
    const std::vector<FireBall>& fireBalls,
    const Texture2D& texture,
    const Texture2D& hit1,
    const Texture2D& hit2,
    const Texture2D& hit3
)
{
    Rectangle source{0.0f, 0.0f, 8.0f, 8.0f};
    const Texture2D* impactFrames[] = {&hit1, &hit2, &hit3};

    for (const FireBall& fireBall : fireBalls)
    {
        if (fireBall.impacting)
        {
            int impactFrame = std::min(
                2,
                static_cast<int>(
                    fireBall.impactTimer * nesNtscFrameRate / 4.0f
                )
            );

            const Texture2D& impactTexture = *impactFrames[impactFrame];
            constexpr float impactSize = 16.0f * 3.0f;

            Rectangle impactSource{
                0.0f,
                0.0f,
                static_cast<float>(impactTexture.width),
                static_cast<float>(impactTexture.height)
            };

            Rectangle impactDestination{
                fireBall.body.x + fireBall.body.width / 2.0f -
                    impactSize / 2.0f,
                fireBall.body.y + fireBall.body.height / 2.0f -
                    impactSize / 2.0f,
                impactSize,
                impactSize
            };

            DrawTexturePro(
                impactTexture,
                impactSource,
                impactDestination,
                {0.0f, 0.0f},
                0.0f,
                WHITE
            );
            continue;
        }

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

void DrawBlueCoins(
    const std::vector<Coin>& coins,
    const Texture2D& texture1,
    const Texture2D& texture2,
    const Texture2D& texture3
)
{
    const Texture2D* frames[] = {&texture1, &texture2, &texture3};
    constexpr int sequence[] = {0, 0, 0, 1, 2, 1};
    int nesFrame = static_cast<int>(GetTime() * nesNtscFrameRate);
    const Texture2D& texture = *frames[sequence[(nesFrame / 8) % 6]];
    Rectangle source{
        0.0f,
        0.0f,
        static_cast<float>(texture.width),
        static_cast<float>(texture.height)
    };

    for (const Coin& coin : coins)
    {
        if (!coin.collected)
        {
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
}

void DrawBlockCoins(
    const std::vector<BlockCoin>& blockCoins,
    const Texture2D& texture1,
    const Texture2D& texture2,
    const Texture2D& texture3,
    const Texture2D& texture4
)
{
    const Texture2D* frames[] = {
        &texture1, &texture2, &texture3, &texture4
    };

    for (const BlockCoin& coin : blockCoins)
    {
        int frame = static_cast<int>(coin.age * 16.0f) % 4;
        const Texture2D& texture = *frames[frame];
        Rectangle source{
            0.0f,
            0.0f,
            static_cast<float>(texture.width),
            static_cast<float>(texture.height)
        };

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
    const Texture2D& texture,
    const Texture2D& deadTexture,
    bool alternateWalkFrame
)
{
    if (
        !goomba.alive ||
        !goomba.spawned
    )
    {
        return;
    }

    const Texture2D& displayedTexture =
        goomba.stomped ? deadTexture : texture;

    Rectangle source =
        GetFirstSquareFrame(displayedTexture);

    if (goomba.dying && !goomba.stomped)
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
        displayedTexture,
        source,
        destination,
        !goomba.dying && alternateWalkFrame
            ? !movingRight
            : movingRight
    );
}

void DrawKoopa(
    const Koopa& koopa,
    const Texture2D& koopaTexture,
    const Texture2D& koopaWalkTexture,
    bool alternateWalkFrame,
    const Texture2D& shellTexture,
    const Texture2D& shellWakeTexture
)
{
    if (!koopa.alive || !koopa.spawned)
    {
        return;
    }

    bool waking =
        koopa.shelled &&
        std::abs(koopa.velocity.x) <= 1.0f &&
        koopa.shellReviveTimer <= koopaShellWakeDuration;

    const Texture2D& texture = koopa.shelled
        ? (waking ? shellWakeTexture : shellTexture)
        : (!koopa.dying && alternateWalkFrame
            ? koopaWalkTexture
            : koopaTexture);

    Rectangle source = koopa.shelled
        ? Rectangle{
            0.0f,
            0.0f,
            static_cast<float>(texture.width),
            static_cast<float>(texture.height)
        }
        : Rectangle{
            0.0f,
            0.0f,
            static_cast<float>(texture.width),
            static_cast<float>(texture.height)
        };

    if (koopa.dying)
    {
        source.y += source.height;
        source.height = -source.height;
    }

    constexpr float spriteWidth = 48.0f;

    float spriteHeight = koopa.shelled
        ? spriteWidth * (
            static_cast<float>(texture.height) /
            static_cast<float>(texture.width)
        )
        : spriteWidth * (
            static_cast<float>(texture.height) /
            static_cast<float>(texture.width)
        );

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
    std::vector<SuperStar>& stars,
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
    player.star = false;
    player.starTimer = 0.0f;
    player.damageTransformTimer = 0.0f;
    player.growthTransformTimer = 0.0f;
    player.fireTransformTimer = 0.0f;

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
    stars.clear();
    fireBalls.clear();
    blockBumps.clear();
    brokenBrickAnimations.clear();
    multiCoinBlocks.clear();
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
    constexpr int bonusEntrancePipeColumn = 65;
    constexpr int bonusEntrancePipeRow = 8;
    constexpr int bonusExitPipeColumn = 17;
    constexpr int bonusExitGroundRow = 12;
    constexpr int overworldReturnPipeColumn = 171;
    constexpr int overworldReturnPipeRow = 10;
    constexpr float pipeTravelSpeed = 3.0f * nesNtscFrameRate;
    constexpr int flagMarkerColumn = 206;
    constexpr int flagMarkerRow = 10;
    constexpr int finishGroundRow = 12;
    constexpr float flagScale = 3.0f;
    constexpr float flagSlideSpeed =
        2.0f * flagScale * nesNtscFrameRate;
    constexpr float flagSlideFrameDuration =
        8.0f / nesNtscFrameRate;
    constexpr float castleDoorX = (210.0f + 2.5f) * tileSize;

    Level level{
        "............................................................................................................................................................................................................................",
        "............................................................................................................................................................................................................................",
        "............................................................................................................................................................................................................................",
        "..........................................................................................G.................................................................................................................................",
        "..............................Q.........................................................BBBBBBBB...BBBQ..............A...........BBB....BQQB........................................................SS......................",
        "...................................................................................................................................................................................................SSS......................",
        ".......................................................................................G..........................................................................................................SSSS......................",
        "........................................................................I........................................................................................................................SSSSS......................",
        "........................Q...BABQB.....................Hh.........Hh..................BAB..............N.....BT....Q..Q..Q.....B..........BB......S..S..........SS..S............BBQB............SSSSSS......................",
        "..............................................Hh......Vv.........Vv.............................................................................SS..SS........SSS..SS..........................SSSSSSS......................",
        "....................................Hh........Vv......Vv.........Vv............................................................................SSS..SSS......SSSS..SSS.....Hh..............Hh.SSSSSSSS........F.............",
        "..P..........................G......Vv........Vv.G....Vv.....G.G.Vv....................................G.G........K.................GG.G.G....SSSS..SSSS....SSSSS..SSSS....Vv.........GG...VvSSSSSSSSS........S.............",
        "#############################################################################..###############...################################################################..#########################################################",
        "#############################################################################..###############...################################################################..#########################################################",
        "#############################################################################..###############...################################################################..#########################################################",
        "#############################################################################..###############...################################################################..#########################################################"
    };

    Level bonusLevel{
        "......P.................",
        "bbbbb...bbbbbbb....Vv...",
        "bbbbb..............Vv...",
        "bbbbb..............Vv...",
        "bbbbb....CCCCC.....Vv...",
        "bbbbb..............Vv...",
        "bbbbb...CCCCCCC....Vv...",
        "bbbbb..............Vv...",
        "bbbbb...CCCCCCC....Vv...",
        "bbbbb...bbbbbbb....Vv...",
        "bbbbb...bbbbbbb..RrVv...",
        "bbbbb...bbbbbbb..rrVv...",
        "XXXXXXXXXXXXXXXXXXXXXXXX",
        "XXXXXXXXXXXXXXXXXXXXXXXX",
        "XXXXXXXXXXXXXXXXXXXXXXXX",
        "XXXXXXXXXXXXXXXXXXXXXXXX"
    };

    NormaliseLevel(level);
    NormaliseLevel(bonusLevel);

    const Level initialLevel = level;
    const Level initialBonusLevel = bonusLevel;

    if (
        level.empty() ||
        GetLevelColumns(level) == 0 ||
        bonusLevel.empty() ||
        GetLevelColumns(bonusLevel) == 0
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

    InitAudioDevice();
    if (!IsAudioDeviceReady())
    {
        TraceLog(LOG_ERROR, "Failed to initialise the audio device.");
        CloseWindow();
        return 1;
    }

    Music backgroundMusic =
        LoadMusicStream("resources/Sounds/music.mp3");
    Music undergroundMusic =
        LoadMusicStream("resources/Sounds/underground_music.mp3");
    Music starMusic =
        LoadMusicStream("resources/Sounds/star.mp3");

    if (
        !IsMusicValid(backgroundMusic) ||
        !IsMusicValid(undergroundMusic) ||
        !IsMusicValid(starMusic)
    )
    {
        TraceLog(LOG_ERROR, "Failed to load one or more music tracks.");
        if (IsMusicValid(backgroundMusic))
        {
            UnloadMusicStream(backgroundMusic);
        }
        if (IsMusicValid(undergroundMusic))
        {
            UnloadMusicStream(undergroundMusic);
        }
        if (IsMusicValid(starMusic))
        {
            UnloadMusicStream(starMusic);
        }
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    backgroundMusic.looping = true;
    undergroundMusic.looping = true;
    starMusic.looping = true;
    PlayMusicStream(backgroundMusic);

    auto stopAllMusic =
        [&backgroundMusic, &undergroundMusic, &starMusic]()
    {
        StopMusicStream(backgroundMusic);
        StopMusicStream(undergroundMusic);
        StopMusicStream(starMusic);
    };

    auto restartBackgroundMusic =
        [&backgroundMusic, &undergroundMusic, &starMusic]()
    {
        StopMusicStream(undergroundMusic);
        StopMusicStream(starMusic);
        StopMusicStream(backgroundMusic);
        SeekMusicStream(backgroundMusic, 0.0f);
        PlayMusicStream(backgroundMusic);
    };

    auto restartUndergroundMusic =
        [&backgroundMusic, &undergroundMusic, &starMusic]()
    {
        StopMusicStream(backgroundMusic);
        StopMusicStream(starMusic);
        StopMusicStream(undergroundMusic);
        SeekMusicStream(undergroundMusic, 0.0f);
        PlayMusicStream(undergroundMusic);
    };

    auto restartStarMusic =
        [&backgroundMusic, &undergroundMusic, &starMusic]()
    {
        StopMusicStream(backgroundMusic);
        StopMusicStream(undergroundMusic);
        StopMusicStream(starMusic);
        SeekMusicStream(starMusic, 0.0f);
        PlayMusicStream(starMusic);
    };

    gameSounds = {
        LoadSound("resources/Sounds/smb_1-up.wav"),
        LoadSound("resources/Sounds/smb_breakblock.wav"),
        LoadSound("resources/Sounds/smb_bump.wav"),
        LoadSound("resources/Sounds/smb_coin.wav"),
        LoadSound("resources/Sounds/smb_fireball.wav"),
        LoadSound("resources/Sounds/smb_flagpole.wav"),
        LoadSound("resources/Sounds/smb_jump-small.wav"),
        LoadSound("resources/Sounds/smb_jump-super.wav"),
        LoadSound("resources/Sounds/smb_kick.wav"),
        LoadSound("resources/Sounds/smb_mariodie.wav"),
        LoadSound("resources/Sounds/smb_pipe.wav"),
        LoadSound("resources/Sounds/smb_powerup.wav"),
        LoadSound("resources/Sounds/smb_powerup_appears.wav"),
        LoadSound("resources/Sounds/smb_stage_clear.wav"),
        LoadSound("resources/Sounds/smb_stomp.wav")
    };

    if (!AreGameSoundsLoaded(gameSounds))
    {
        TraceLog(LOG_ERROR, "Failed to load one or more game sounds.");
        UnloadGameSounds(gameSounds);
        StopMusicStream(backgroundMusic);
        StopMusicStream(undergroundMusic);
        StopMusicStream(starMusic);
        UnloadMusicStream(backgroundMusic);
        UnloadMusicStream(undergroundMusic);
        UnloadMusicStream(starMusic);
        CloseAudioDevice();
        CloseWindow();
        return 1;
    }

    GameTextures textures{
        LoadTexture("resources/Blocks/Ground.png"),
        LoadTexture("resources/Blocks/Brick.png"),
        LoadTexture("resources/Blocks/Broken_Brick.png"),
        LoadTexture("resources/Blocks/Brick_Piece_1.png"),
        LoadTexture("resources/Blocks/Brick_Piece_2.png"),
        LoadTexture("resources/Blocks/Blue_Ground.png"),
        LoadTexture("resources/Blocks/Blue_Brick.png"),
        LoadTexture("resources/Blocks/Blue_Broken_Brick.png"),
        LoadTexture("resources/Blocks/Q_Block_1.png"),
        LoadTexture("resources/Blocks/Q_Block_2.png"),
        LoadTexture("resources/Blocks/Q_Block_3.png"),
        LoadTexture("resources/Blocks/Empty_Block.png"),
        LoadTexture("resources/Blocks/Stair_Block.png"),
        LoadTexture("resources/Pipe.png"),
        LoadTexture("resources/Pipe_Base.png"),
        LoadTexture("resources/Pipe_Connector_Underground.png"),
        LoadTexture("resources/Pipe_Base_Undergorund.png"),
        LoadTexture("resources/Coin.png"),
        LoadTexture("resources/Coin_Flash_1.png"),
        LoadTexture("resources/Coin_Flash_2.png"),
        LoadTexture("resources/Coin_Flash_3.png"),
        LoadTexture("resources/Coin_Flash_4.png"),
        LoadTexture("resources/Blue_Coin_1.png"),
        LoadTexture("resources/Blue_Coin_2.png"),
        LoadTexture("resources/Blue_Coin_3.png"),
        LoadTexture("resources/Background/Big_Hill.png"),
        LoadTexture("resources/Background/Small_Hill.png"),
        LoadTexture("resources/Background/Single_Bush.png"),
        LoadTexture("resources/Background/Double_Bush.png"),
        LoadTexture("resources/Background/Triple_Bush.png"),
        LoadTexture("resources/Background/Single_Cloud.png"),
        LoadTexture("resources/Background/Double_Cloud.png"),
        LoadTexture("resources/Background/Triple_Cloud.png"),
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
        LoadTexture("resources/Enemies/Goomba_Dead.png"),
        LoadTexture("resources/Enemies/Green_Koopa.png"),
        LoadTexture("resources/Enemies/Green_Koopa_Walk.png"),
        LoadTexture("resources/Green_Shell.png"),
        LoadTexture("resources/Green_Shell_Wake.png"),
        LoadTexture("resources/Items/Super_Mushroom.png"),
        LoadTexture("resources/Items/1_Up_Mushroom.png"),
        LoadTexture("resources/Items/Fire_Flower_1.png"),
        LoadTexture("resources/Items/Fire_Flower_2.png"),
        LoadTexture("resources/Items/Fire_Flower_3.png"),
        LoadTexture("resources/Items/Fire_Flower_4.png"),
        LoadTexture("resources/Items/Star_1.png"),
        LoadTexture("resources/Items/Star_2.png"),
        LoadTexture("resources/Items/Star_3.png"),
        LoadTexture("resources/Items/Star_4.png"),
        {
            LoadTexture("resources/Mario/Star_Mario/Brown_Small_Mario.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Small_Mario_Jump.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Small_Mario_Switch.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Small_Mario_Walk_1.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Small_Mario_Walk_2.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Small_Mario_Walk_3.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Super_Mario.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Super_Mario_Duck.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Super_Mario_Fire.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Super_Mario_Jump.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Super_Mario_Switch.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Super_Mario_Walk_1.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Super_Mario_Walk_2.png"),
            LoadTexture("resources/Mario/Star_Mario/Brown_Super_Mario_Walk_3.png")
        },
        {
            LoadTexture("resources/Mario/Star_Mario/Green_Small_Mario.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Small_Mario_Jump.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Small_Mario_Switch.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Small_Mario_Walk_1.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Small_Mario_Walk_2.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Small_Mario_Walk_3.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Super_Mario.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Super_Mario_Duck.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Super_Mario_Fire.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Super_Mario_Jump.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Super_Mario_Switch.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Super_Mario_Walk_1.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Super_Mario_Walk_2.png"),
            LoadTexture("resources/Mario/Star_Mario/Green_Super_Mario_Walk_3.png")
        },
        {
            LoadTexture("resources/Mario/Star_Mario/Red_Small_Mario.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Small_Mario_Jump.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Small_Mario_Switch.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Small_Mario_Walk_1.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Small_Mario_Walk_2.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Small_Mario_Walk_3.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Super_Mario.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Super_Mario_Duck.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Super_Mario_Fire.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Super_Mario_Jump.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Super_Mario_Switch.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Super_Mario_Walk_1.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Super_Mario_Walk_2.png"),
            LoadTexture("resources/Mario/Star_Mario/Red_Super_Mario_Walk_3.png")
        },
        LoadTexture("resources/Fire_Ball.png"),
        LoadTexture("resources/Fire_Ball_Hit_1.png"),
        LoadTexture("resources/Fire_Ball_Hit_2.png"),
        LoadTexture("resources/Fire_Ball_Hit_3.png"),
        LoadTexture("resources/Flag.png"),
        LoadTexture("resources/Flag_Pole.png"),
        LoadTexture("resources/Mario/Small_Mario_Flag_1.png"),
        LoadTexture("resources/Mario/Small_Mario_Flag_2.png"),
        LoadTexture("resources/Mario/Super_Mario_Flag_1.png"),
        LoadTexture("resources/Mario/Super_Mario_Flag_2.png"),
        LoadTexture("resources/Mario/Fire_Mario_Flag_1.png"),
        LoadTexture("resources/Mario/Fire_Mario_Flag_2.png"),
        LoadTexture("resources/castle.png"),
        LoadTexture("resources/title.png")
    };

    SetTextureFilter(
        textures.ground,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.brick,
        TEXTURE_FILTER_POINT
    );
    SetTextureFilter(textures.brokenBrick, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.brickPiece1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.brickPiece2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.blueGround, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.blueBrick, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.blueBrokenBrick, TEXTURE_FILTER_POINT);

    SetTextureFilter(textures.questionBlock1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.questionBlock2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.questionBlock3, TEXTURE_FILTER_POINT);

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
        textures.undergroundPipeConnector,
        TEXTURE_FILTER_POINT
    );
    SetTextureFilter(
        textures.undergroundPipeBase,
        TEXTURE_FILTER_POINT
    );

    SetTextureFilter(
        textures.coin,
        TEXTURE_FILTER_POINT
    );
    SetTextureFilter(textures.coinFlash1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.coinFlash2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.coinFlash3, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.coinFlash4, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.blueCoin1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.blueCoin2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.blueCoin3, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.bigHill, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallHill, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.singleBush, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.doubleBush, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.tripleBush, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.singleCloud, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.doubleCloud, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.tripleCloud, TEXTURE_FILTER_POINT);

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

    SetTextureFilter(textures.goombaDead, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.koopa, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.koopaWalk, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.greenShell, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.greenShellWake, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMushroom, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.oneUpMushroom, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireFlower1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireFlower2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireFlower3, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireFlower4, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.star1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.star2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.star3, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.star4, TEXTURE_FILTER_POINT);
    SetStarMarioTextureFilter(textures.brownStarMario);
    SetStarMarioTextureFilter(textures.greenStarMario);
    SetStarMarioTextureFilter(textures.redStarMario);
    SetTextureFilter(textures.fireBall, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireBallHit1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireBallHit2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireBallHit3, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.flag, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.flagPole, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallMarioFlag1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.smallMarioFlag2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMarioFlag1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.superMarioFlag2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMarioFlag1, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.fireMarioFlag2, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.castle, TEXTURE_FILTER_POINT);
    SetTextureFilter(textures.title, TEXTURE_FILTER_POINT);

    if (
        textures.ground.id == 0 ||
        textures.brokenBrick.id == 0 ||
        textures.brickPiece1.id == 0 ||
        textures.brickPiece2.id == 0 ||
        textures.blueGround.id == 0 ||
        textures.blueBrick.id == 0 ||
        textures.blueBrokenBrick.id == 0 ||
        textures.questionBlock1.id == 0 ||
        textures.questionBlock2.id == 0 ||
        textures.questionBlock3.id == 0 ||
        textures.pipeBase.id == 0 ||
        textures.undergroundPipeConnector.id == 0 ||
        textures.undergroundPipeBase.id == 0 ||
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
        textures.goombaDead.id == 0 ||
        textures.koopa.id == 0 ||
        textures.koopaWalk.id == 0 ||
        textures.greenShell.id == 0 ||
        textures.greenShellWake.id == 0 ||
        textures.superMushroom.id == 0 ||
        textures.oneUpMushroom.id == 0 ||
        textures.coinFlash1.id == 0 ||
        textures.coinFlash2.id == 0 ||
        textures.coinFlash3.id == 0 ||
        textures.coinFlash4.id == 0 ||
        textures.blueCoin1.id == 0 ||
        textures.blueCoin2.id == 0 ||
        textures.blueCoin3.id == 0 ||
        textures.bigHill.id == 0 ||
        textures.smallHill.id == 0 ||
        textures.singleBush.id == 0 ||
        textures.doubleBush.id == 0 ||
        textures.tripleBush.id == 0 ||
        textures.singleCloud.id == 0 ||
        textures.doubleCloud.id == 0 ||
        textures.tripleCloud.id == 0 ||
        textures.fireFlower1.id == 0 ||
        textures.fireFlower2.id == 0 ||
        textures.fireFlower3.id == 0 ||
        textures.fireFlower4.id == 0 ||
        textures.star1.id == 0 ||
        textures.star2.id == 0 ||
        textures.star3.id == 0 ||
        textures.star4.id == 0 ||
        !AreStarMarioTexturesLoaded(textures.brownStarMario) ||
        !AreStarMarioTexturesLoaded(textures.greenStarMario) ||
        !AreStarMarioTexturesLoaded(textures.redStarMario) ||
        textures.fireBall.id == 0 ||
        textures.fireBallHit1.id == 0 ||
        textures.fireBallHit2.id == 0 ||
        textures.fireBallHit3.id == 0 ||
        textures.flag.id == 0 ||
        textures.flagPole.id == 0 ||
        textures.smallMarioFlag1.id == 0 ||
        textures.smallMarioFlag2.id == 0 ||
        textures.superMarioFlag1.id == 0 ||
        textures.superMarioFlag2.id == 0 ||
        textures.fireMarioFlag1.id == 0 ||
        textures.fireMarioFlag2.id == 0 ||
        textures.castle.id == 0 ||
        textures.title.id == 0
    )
    {
        TraceLog(
            LOG_ERROR,
            "Failed to load one or more game textures."
        );

        UnloadTexture(textures.ground);
        UnloadTexture(textures.brokenBrick);
        UnloadTexture(textures.brickPiece1);
        UnloadTexture(textures.brickPiece2);
        UnloadTexture(textures.blueGround);
        UnloadTexture(textures.blueBrick);
        UnloadTexture(textures.blueBrokenBrick);
        UnloadTexture(textures.questionBlock1);
        UnloadTexture(textures.questionBlock2);
        UnloadTexture(textures.questionBlock3);
        UnloadTexture(textures.pipeBase);
        UnloadTexture(textures.undergroundPipeConnector);
        UnloadTexture(textures.undergroundPipeBase);
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
        UnloadTexture(textures.goombaDead);
        UnloadTexture(textures.koopa);
        UnloadTexture(textures.koopaWalk);
        UnloadTexture(textures.greenShell);
        UnloadTexture(textures.greenShellWake);
        UnloadTexture(textures.superMushroom);
        UnloadTexture(textures.oneUpMushroom);
        UnloadTexture(textures.coinFlash1);
        UnloadTexture(textures.coinFlash2);
        UnloadTexture(textures.coinFlash3);
        UnloadTexture(textures.coinFlash4);
        UnloadTexture(textures.blueCoin1);
        UnloadTexture(textures.blueCoin2);
        UnloadTexture(textures.blueCoin3);
        UnloadTexture(textures.bigHill);
        UnloadTexture(textures.smallHill);
        UnloadTexture(textures.singleBush);
        UnloadTexture(textures.doubleBush);
        UnloadTexture(textures.tripleBush);
        UnloadTexture(textures.singleCloud);
        UnloadTexture(textures.doubleCloud);
        UnloadTexture(textures.tripleCloud);
        UnloadTexture(textures.fireFlower1);
        UnloadTexture(textures.fireFlower2);
        UnloadTexture(textures.fireFlower3);
        UnloadTexture(textures.fireFlower4);
        UnloadTexture(textures.star1);
        UnloadTexture(textures.star2);
        UnloadTexture(textures.star3);
        UnloadTexture(textures.star4);
        UnloadStarMarioTextures(textures.brownStarMario);
        UnloadStarMarioTextures(textures.greenStarMario);
        UnloadStarMarioTextures(textures.redStarMario);
        UnloadTexture(textures.fireBall);
        UnloadTexture(textures.fireBallHit1);
        UnloadTexture(textures.fireBallHit2);
        UnloadTexture(textures.fireBallHit3);
        UnloadTexture(textures.flag);
        UnloadTexture(textures.flagPole);
        UnloadTexture(textures.smallMarioFlag1);
        UnloadTexture(textures.smallMarioFlag2);
        UnloadTexture(textures.superMarioFlag1);
        UnloadTexture(textures.superMarioFlag2);
        UnloadTexture(textures.fireMarioFlag1);
        UnloadTexture(textures.fireMarioFlag2);
        UnloadTexture(textures.castle);
        UnloadTexture(textures.title);

        UnloadGameSounds(gameSounds);
        StopMusicStream(backgroundMusic);
        StopMusicStream(undergroundMusic);
        StopMusicStream(starMusic);
        UnloadMusicStream(backgroundMusic);
        UnloadMusicStream(undergroundMusic);
        UnloadMusicStream(starMusic);
        CloseAudioDevice();
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
        false,
        false,
        0.0f,
        0.0f,
        0.0f,
        0.0f
    };

    std::vector<SuperMushroom> mushrooms;
    std::vector<FireFlower> fireFlowers;
    std::vector<SuperStar> stars;
    std::vector<FireBall> fireBalls;

    std::vector<Coin> initialCoins =
        FindCoinSpawns(level);

    std::vector<Coin> coins =
        initialCoins;

    const std::vector<Coin> initialBonusCoins =
        FindCoinSpawns(bonusLevel, true);

    std::vector<Coin> bonusCoins =
        initialBonusCoins;

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

    std::vector<Goomba> otherAreaGoombas;
    std::vector<Koopa> otherAreaKoopas;
    std::vector<SuperMushroom> otherAreaMushrooms;
    std::vector<FireFlower> otherAreaFireFlowers;
    std::vector<SuperStar> otherAreaStars;
    std::vector<FireBall> otherAreaFireBalls;

    bool inBonusArea = false;

    enum class PipeTransition
    {
        None,
        EnteringBonusDown,
        EnteringOverworldRight,
        EmergingOverworldUp
    };

    PipeTransition pipeTransition = PipeTransition::None;

    enum class FinishSequence
    {
        None,
        Sliding,
        Turning,
        Dismounting,
        WalkingToCastle,
        InsideCastle,
        Blackout
    };

    FinishSequence finishSequence = FinishSequence::None;
    float finishSequenceTimer = 0.0f;
    const float flagPoleTop =
        (flagMarkerRow + 2) * tileSize -
        textures.flagPole.height * flagScale;
    const float flagTopY = flagPoleTop + tileSize - 22.0f;
    const float flagBottomY =
        (finishGroundRow - 1) * tileSize -
        textures.flag.height * flagScale;
    const float flagPoleX =
        flagMarkerColumn * tileSize - 8.0f * flagScale +
        15.5f * flagScale;
    float flagY = flagTopY;

    auto swapAreaState = [&]()
    {
        level.swap(bonusLevel);
        coins.swap(bonusCoins);
        goombas.swap(otherAreaGoombas);
        koopas.swap(otherAreaKoopas);
        mushrooms.swap(otherAreaMushrooms);
        fireFlowers.swap(otherAreaFireFlowers);
        stars.swap(otherAreaStars);
        fireBalls.swap(otherAreaFireBalls);
        blockCoins.clear();
        blockBumps.clear();
        brokenBrickAnimations.clear();
        multiCoinBlocks.clear();
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
    float pitRespawnTimer = 0.0f;
    float deathSoundTimer = 0.0f;
    float stageClearSoundTimer = 0.0f;
    bool pitDeathPending = false;
    int lastEnemyCollisionFrame = -1;

    while (!WindowShouldClose())
    {
        if (IsMusicStreamPlaying(backgroundMusic))
        {
            UpdateMusicStream(backgroundMusic);
        }
        if (IsMusicStreamPlaying(undergroundMusic))
        {
            UpdateMusicStream(undergroundMusic);
        }
        if (IsMusicStreamPlaying(starMusic))
        {
            UpdateMusicStream(starMusic);
        }

        float deltaTime =
            std::min(
                GetFrameTime(),
                0.05f
            );

        deathSoundTimer = std::max(
            0.0f,
            deathSoundTimer - deltaTime
        );
        stageClearSoundTimer = std::max(
            0.0f,
            stageClearSoundTimer - deltaTime
        );

        player.invulnerabilityTimer = std::max(
            0.0f,
            player.invulnerabilityTimer - deltaTime
        );

        float previousDamageTransformTimer =
            player.damageTransformTimer;
        player.damageTransformTimer = std::max(
            0.0f,
            player.damageTransformTimer - deltaTime
        );

        if (
            previousDamageTransformTimer > 0.0f &&
            player.damageTransformTimer <= 0.0f &&
            player.big
        )
        {
            float bottom = player.body.y + player.body.height;
            float centre = player.body.x + player.body.width / 2.0f;

            player.body.width = 32.0f;
            player.body.height = 44.0f;
            player.body.x = centre - player.body.width / 2.0f;
            player.body.y = bottom - player.body.height;
            player.big = false;
            player.fire = false;
            player.ducking = false;
        }

        float previousGrowthTransformTimer =
            player.growthTransformTimer;
        player.growthTransformTimer = std::max(
            0.0f,
            player.growthTransformTimer - deltaTime
        );

        if (
            previousGrowthTransformTimer > 0.0f &&
            player.growthTransformTimer <= 0.0f &&
            !player.big
        )
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

        float previousFireTransformTimer =
            player.fireTransformTimer;
        player.fireTransformTimer = std::max(
            0.0f,
            player.fireTransformTimer - deltaTime
        );

        if (
            previousFireTransformTimer > 0.0f &&
            player.fireTransformTimer <= 0.0f
        )
        {
            player.fire = true;
        }

        player.fireThrowTimer = std::max(
            0.0f,
            player.fireThrowTimer - deltaTime
        );

        float previousStarTimer = player.starTimer;
        player.starTimer = std::max(
            0.0f,
            player.starTimer - deltaTime
        );
        player.star = player.starTimer > 0.0f;

        if (
            previousStarTimer > 0.0f &&
            player.starTimer <= 0.0f
        )
        {
            if (inBonusArea)
            {
                restartUndergroundMusic();
            }
            else
            {
                restartBackgroundMusic();
            }
        }

        UpdateBlockBumps(
            deltaTime,
            mushrooms,
            fireFlowers,
            stars,
            goombas,
            koopas
        );
        UpdateBrokenBrickAnimations(deltaTime);

        if (
            finishSequence == FinishSequence::None &&
            pipeTransition == PipeTransition::None &&
            !inBonusArea &&
            !player.dying &&
            !IsPlayerTransforming(player) &&
            player.body.x <= flagPoleX + tileSize &&
            player.body.x + player.body.width >= flagPoleX &&
            player.body.y + player.body.height >= flagPoleTop
        )
        {
            finishSequence = FinishSequence::Sliding;
            stopAllMusic();
            PlayGameSound(gameSounds.flagPole);
            SetPlayerDucking(player, false, level);
            player.body.x = flagPoleX - player.body.width - 2.0f;
            player.velocity = {0.0f, 0.0f};
            player.onGround = false;
            player.facingLeft = false;
            player.sprinting = false;
            player.skidding = false;
            player.star = false;
            player.starTimer = 0.0f;
            player.fireThrowTimer = 0.0f;
            player.walkAnimationTimer = 0.0f;
            player.walkAnimationFrame = 0;
            jumpBufferTimer = 0.0f;
            coyoteTimer = 0.0f;
        }

        if (
            finishSequence == FinishSequence::None &&
            pipeTransition == PipeTransition::None &&
            !IsPlayerTransforming(player) &&
            !player.dying &&
            player.onGround
        )
        {
            if (!inBonusArea && (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)))
            {
                float pipeLeft = bonusEntrancePipeColumn * tileSize;
                float pipeRight =
                    (bonusEntrancePipeColumn + 2) * tileSize;
                float pipeMiddle = pipeLeft + tileSize;
                float pipeTop = bonusEntrancePipeRow * tileSize;
                float playerRight = player.body.x + player.body.width;
                float playerBottom =
                    player.body.y + player.body.height;
                float leftTileOverlap =
                    std::min(playerRight, pipeMiddle) -
                    std::max(player.body.x, pipeLeft);
                float rightTileOverlap =
                    std::min(playerRight, pipeRight) -
                    std::max(player.body.x, pipeMiddle);

                if (
                    leftTileOverlap >= 1.0f &&
                    rightTileOverlap >= 1.0f &&
                    std::abs(playerBottom - pipeTop) <= 4.0f
                )
                {
                    SetPlayerDucking(player, false, level);
                    player.body.x = pipeLeft +
                        (tileSize * 2.0f - player.body.width) / 2.0f;
                    player.body.y = pipeTop - player.body.height;
                    player.velocity = {0.0f, 0.0f};
                    player.onGround = false;
                    player.skidding = false;
                    player.sprinting = false;
                    PlayGameSound(gameSounds.pipe);
                    pipeTransition = PipeTransition::EnteringBonusDown;
                }
            }
            else if (
                inBonusArea &&
                (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
            )
            {
                float pipeLeft = bonusExitPipeColumn * tileSize;
                float groundTop = bonusExitGroundRow * tileSize;
                float playerRight = player.body.x + player.body.width;
                float playerBottom = player.body.y + player.body.height;

                if (
                    std::abs(playerRight - pipeLeft) <= 2.0f &&
                    std::abs(playerBottom - groundTop) <= 4.0f
                )
                {
                    SetPlayerDucking(player, false, level);
                    player.body.x = pipeLeft - player.body.width;
                    player.body.y = groundTop - player.body.height;
                    player.velocity = {0.0f, 0.0f};
                    player.facingLeft = false;
                    player.skidding = false;
                    player.sprinting = false;
                    PlayGameSound(gameSounds.pipe);
                    pipeTransition =
                        PipeTransition::EnteringOverworldRight;
                }
            }
        }

        if (finishSequence != FinishSequence::None)
        {
            player.velocity.x = 0.0f;
            if (finishSequence != FinishSequence::Dismounting)
            {
                player.velocity.y = 0.0f;
            }
            player.ducking = false;
            player.skidding = false;
            player.sprinting = false;
            jumpBufferTimer = 0.0f;
            coyoteTimer = 0.0f;

            const float groundTop = finishGroundRow * tileSize;
            const float playerGroundY = groundTop - player.body.height;
            const float playerSlideEndY = playerGroundY - tileSize;

            if (finishSequence == FinishSequence::Sliding)
            {
                flagY = std::min(
                    flagBottomY,
                    flagY + flagSlideSpeed * deltaTime
                );
                player.body.y = std::min(
                    playerSlideEndY,
                    player.body.y + flagSlideSpeed * deltaTime
                );
                player.walkAnimationTimer += deltaTime;

                while (
                    player.walkAnimationTimer >= flagSlideFrameDuration
                )
                {
                    player.walkAnimationTimer -= flagSlideFrameDuration;
                    player.walkAnimationFrame =
                        (player.walkAnimationFrame + 1) % 2;
                }

                if (
                    flagY >= flagBottomY &&
                    player.body.y >= playerSlideEndY
                )
                {
                    finishSequence = FinishSequence::Turning;
                    finishSequenceTimer = 0.35f;
                    player.body.x = flagPoleX + 2.0f;
                    player.walkAnimationFrame = 0;
                }
            }
            else if (finishSequence == FinishSequence::Turning)
            {
                finishSequenceTimer = std::max(
                    0.0f,
                    finishSequenceTimer - deltaTime
                );

                if (finishSequenceTimer <= 0.0f)
                {
                    finishSequence = FinishSequence::Dismounting;
                    player.velocity.y = 0.0f;
                    player.onGround = false;
                }
            }
            else if (finishSequence == FinishSequence::Dismounting)
            {
                player.velocity.y = std::min(
                    flagSlideSpeed,
                    player.velocity.y + fallingGravity * deltaTime
                );
                player.body.y = std::min(
                    playerGroundY,
                    player.body.y + player.velocity.y * deltaTime
                );

                if (player.body.y >= playerGroundY)
                {
                    finishSequence = FinishSequence::WalkingToCastle;
                    PlayGameSound(gameSounds.stageClear);
                    stageClearSoundTimer = stageClearSoundDuration;
                    player.facingLeft = false;
                    player.onGround = true;
                    player.velocity.y = 0.0f;
                    player.walkAnimationTimer = 0.0f;
                    player.walkAnimationFrame = 0;
                }
            }
            else if (finishSequence == FinishSequence::WalkingToCastle)
            {
                constexpr float castleWalkSpeed = 150.0f;
                constexpr float castleWalkFrameDuration = 0.11f;
                float targetX = castleDoorX - player.body.width / 2.0f;

                player.body.x = std::min(
                    targetX,
                    player.body.x + castleWalkSpeed * deltaTime
                );
                player.body.y = playerGroundY;
                player.onGround = true;
                player.velocity.x = castleWalkSpeed;
                player.walkAnimationTimer += deltaTime;

                while (
                    player.walkAnimationTimer >= castleWalkFrameDuration
                )
                {
                    player.walkAnimationTimer -= castleWalkFrameDuration;
                    player.walkAnimationFrame =
                        (player.walkAnimationFrame + 1) % 3;
                }

                if (player.body.x >= targetX)
                {
                    finishSequence = FinishSequence::InsideCastle;
                    finishSequenceTimer = 1.0f;
                    player.velocity = {0.0f, 0.0f};
                }
            }
            else if (finishSequence == FinishSequence::InsideCastle)
            {
                finishSequenceTimer = std::max(
                    0.0f,
                    finishSequenceTimer - deltaTime
                );

                if (finishSequenceTimer <= 0.0f)
                {
                    if (stageClearSoundTimer <= 0.0f)
                    {
                        finishSequence = FinishSequence::Blackout;
                        finishSequenceTimer = 1.25f;
                    }
                }
            }
            else if (finishSequence == FinishSequence::Blackout)
            {
                finishSequenceTimer = std::max(
                    0.0f,
                    finishSequenceTimer - deltaTime
                );

                if (finishSequenceTimer <= 0.0f)
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
                        stars,
                        fireBalls,
                        jumpBufferTimer,
                        coyoteTimer
                    );

                    bonusLevel = initialBonusLevel;
                    bonusCoins = initialBonusCoins;
                    otherAreaGoombas.clear();
                    otherAreaKoopas.clear();
                    otherAreaMushrooms.clear();
                    otherAreaFireFlowers.clear();
                    otherAreaStars.clear();
                    otherAreaFireBalls.clear();
                    pipeTransition = PipeTransition::None;
                    pitRespawnTimer = 0.0f;
                    stageClearSoundTimer = 0.0f;
                    pitDeathPending = false;
                    flagY = flagTopY;
                    finishSequence = FinishSequence::None;
                    restartBackgroundMusic();
                }
            }
        }
        else if (pipeTransition != PipeTransition::None)
        {
            player.velocity = {0.0f, 0.0f};
            player.ducking = false;
            player.skidding = false;
            player.sprinting = false;
            player.walkAnimationTimer = 0.0f;
            player.walkAnimationFrame = 0;
            jumpBufferTimer = 0.0f;
            coyoteTimer = 0.0f;

            if (pipeTransition == PipeTransition::EnteringBonusDown)
            {
                float pipeTop = bonusEntrancePipeRow * tileSize;
                player.body.y = std::min(
                    pipeTop,
                    player.body.y + pipeTravelSpeed * deltaTime
                );

                if (player.body.y >= pipeTop)
                {
                    swapAreaState();
                    inBonusArea = true;
                    if (!player.star)
                    {
                        restartUndergroundMusic();
                    }

                    Vector2 bonusSpawn = FindPlayerSpawn(level);
                    player.body.x = bonusSpawn.x +
                        (tileSize - player.body.width) / 2.0f;
                    player.body.y = -player.body.height;
                    player.onGround = false;
                    camera.target = {
                        virtualWidth / 2.0f,
                        virtualHeight / 2.0f
                    };
                    pipeTransition = PipeTransition::None;
                }
            }
            else if (
                pipeTransition == PipeTransition::EnteringOverworldRight
            )
            {
                float pipeLeft = bonusExitPipeColumn * tileSize;
                player.body.x = std::min(
                    pipeLeft,
                    player.body.x + pipeTravelSpeed * deltaTime
                );

                if (player.body.x >= pipeLeft)
                {
                    swapAreaState();
                    inBonusArea = false;
                    if (!player.star)
                    {
                        restartBackgroundMusic();
                    }

                    float returnPipeLeft =
                        overworldReturnPipeColumn * tileSize;
                    float returnPipeTop =
                        overworldReturnPipeRow * tileSize;

                    player.body.x = returnPipeLeft +
                        (tileSize * 2.0f - player.body.width) / 2.0f;
                    player.body.y = returnPipeTop;
                    player.onGround = false;

                    float halfVisibleWidth =
                        virtualWidth / (2.0f * camera.zoom);
                    camera.target = {
                        std::clamp(
                            player.body.x + player.body.width / 2.0f,
                            halfVisibleWidth,
                            GetLevelWidth(level) - halfVisibleWidth
                        ),
                        virtualHeight / 2.0f
                    };

                    // The overworld becomes visible immediately while Mario
                    // rises from the pipe. Activate enemies against the new
                    // camera now so none pop in after the transition ends.
                    SpawnGoombasNearCamera(goombas, camera);
                    SpawnKoopasNearCamera(koopas, camera);
                    pipeTransition = PipeTransition::EmergingOverworldUp;
                }
            }
            else if (pipeTransition == PipeTransition::EmergingOverworldUp)
            {
                float pipeTop = overworldReturnPipeRow * tileSize;
                float destinationY = pipeTop - player.body.height;
                player.body.y = std::max(
                    destinationY,
                    player.body.y - pipeTravelSpeed * deltaTime
                );

                if (player.body.y <= destinationY)
                {
                    player.body.y = destinationY;
                    player.onGround = true;
                    coyoteTimer = coyoteTimeDuration;
                    pipeTransition = PipeTransition::None;
                }
            }
        }
        else if (IsPlayerTransforming(player))
        {
            player.sprinting = false;
            player.skidding = false;
            player.ducking = false;
            player.walkAnimationTimer = 0.0f;
            player.walkAnimationFrame = 0;
            jumpBufferTimer = 0.0f;
            coyoteTimer = 0.0f;
        }
        else
        {

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

        if (pitDeathPending)
        {
            direction = 0.0f;
            player.velocity.x = 0.0f;
            player.sprinting = false;
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
        }

        if (direction < 0.0f)
        {
            player.facingLeft = true;
        }
        else if (direction > 0.0f)
        {
            player.facingLeft = false;
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
                0.0f,
                false,
                0.0f
            });

            PlayGameSound(gameSounds.fireBall);
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

        if (!sprinting && !player.ducking)
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
            PlayGameSound(
                player.big
                    ? gameSounds.jumpSuper
                    : gameSounds.jumpSmall
            );
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
            stars,
            goombas,
            koopas,
            blockCoins,
            coinCount,
            deltaTime
        );

        UpdateBlockCoins(blockCoins, deltaTime);
        UpdateMultiCoinBlocks(deltaTime);

        if (
            player.onGround &&
            jumpBufferTimer > 0.0f
        )
        {
            PlayGameSound(
                player.big
                    ? gameSounds.jumpSuper
                    : gameSounds.jumpSmall
            );
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
            deltaTime
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

        UpdateSuperStars(stars, level, deltaTime);
        if (!player.dying)
        {
            if (CollectSuperStars(player, stars))
            {
                restartStarMusic();
            }
        }

        UpdateFireBalls(
            fireBalls,
            level,
            goombas,
            koopas,
            camera,
            deltaTime
        );

        // The original checks enemy pairs only on odd NES frames.
        int enemyCollisionFrame =
            static_cast<int>(GetTime() * nesNtscFrameRate);
        if (
            (enemyCollisionFrame & 0x01) != 0 &&
            enemyCollisionFrame != lastEnemyCollisionFrame
        )
        {
            HandleOrdinaryEnemyCollisions(goombas, koopas, camera);
            lastEnemyCollisionFrame = enemyCollisionFrame;
        }

        HandleStationaryShellEnemyCollisions(
            koopas,
            goombas
        );

        HandleMovingShellEnemyCollisions(
            koopas,
            goombas
        );

        if (
            !player.dying &&
            !IsPlayerTransforming(player)
        )
        {
            HitEnemiesAsStarMario(player, goombas, koopas);
        }

        bool playerHitGoomba =
            !player.dying &&
            !IsPlayerTransforming(player) &&
            HandlePlayerGoombaCollisions(
                player,
                goombas,
                previousPlayerBottom
            );

        bool playerHitKoopa =
            !player.dying &&
            !IsPlayerTransforming(player) &&
            HandlePlayerKoopaCollisions(
                player,
                koopas,
                previousPlayerBottom
            );

        bool playerHitEnemy =
            playerHitGoomba || playerHitKoopa;

        if (
            playerHitEnemy &&
            player.big &&
            player.damageTransformTimer <= 0.0f
        )
        {
            constexpr float damageInvulnerability = 1.5f;
            constexpr float damageTransformDelay = 0.45f;

            player.ducking = false;
            player.invulnerabilityTimer =
                damageInvulnerability;
            player.damageTransformTimer =
                damageTransformDelay;
            PlayGameSound(gameSounds.pipe);

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

        if (playerFell && !pitDeathPending)
        {
            constexpr float pitRespawnDelay = 0.75f;

            pitDeathPending = true;
            pitRespawnTimer = pitRespawnDelay;
            deathSoundTimer = marioDeathSoundDuration;
            player.velocity.x = 0.0f;
            player.sprinting = false;
            stopAllMusic();
            PlayGameSound(gameSounds.marioDie);
        }

        if (pitDeathPending)
        {
            pitRespawnTimer = std::max(
                0.0f,
                pitRespawnTimer - deltaTime
            );
        }

        bool pitRespawnReady =
            pitDeathPending && pitRespawnTimer <= 0.0f;

        if (playerHitEnemy)
        {
            stopAllMusic();
            deathSoundTimer = marioDeathSoundDuration;
            KillPlayer(player);
        }

        bool deathAnimationFinished =
            player.dying &&
            player.velocity.y > 0.0f &&
            player.body.y > cameraBottom + 100.0f;

        if (
            (pitRespawnReady || deathAnimationFinished) &&
            deathSoundTimer <= 0.0f
        )
        {
            if (inBonusArea)
            {
                swapAreaState();
                inBonusArea = false;
            }

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
                stars,
                fireBalls,
                jumpBufferTimer,
                coyoteTimer
            );

            bonusLevel = initialBonusLevel;
            bonusCoins = initialBonusCoins;
            otherAreaGoombas.clear();
            otherAreaKoopas.clear();
            otherAreaMushrooms.clear();
            otherAreaFireFlowers.clear();
            otherAreaStars.clear();
            otherAreaFireBalls.clear();

            pitRespawnTimer = 0.0f;
            deathSoundTimer = 0.0f;
            pitDeathPending = false;
            restartBackgroundMusic();
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

            for (SuperStar& star : stars)
            {
                if (
                    !star.collected &&
                    star.body.x + star.body.width < viewportLeft
                )
                {
                    star.collected = true;
                }
            }
        }

        }

        // --------------------------------
        // Draw virtual game screen
        // --------------------------------

        BeginTextureMode(gameTexture);

        ClearBackground(
            inBonusArea
                ? BLACK
                : Color{146, 144, 255, 255}
        );

        BeginMode2D(camera);

        if (!inBonusArea)
        {
            DrawBackgroundScenery(level, textures);

            // Keep the logo in the opening area's world space so it begins
            // centred on screen, then naturally scrolls away with the level.
            DrawOpeningTitle(textures.title);
        }

        // Draw items behind tiles so mushrooms rise out of blocks.
        DrawSuperMushrooms(
            mushrooms,
            textures.superMushroom,
            textures.oneUpMushroom
        );

        DrawFireFlowers(
            fireFlowers,
            textures.fireFlower1,
            textures.fireFlower2,
            textures.fireFlower3,
            textures.fireFlower4
        );

        DrawSuperStars(
            stars,
            textures.star1,
            textures.star2,
            textures.star3,
            textures.star4
        );

        bool drawPlayerBehindTiles =
            pipeTransition != PipeTransition::None;

        if (drawPlayerBehindTiles)
        {
            DrawPlayer(player, textures, true);
        }

        DrawLevel(
            level,
            textures,
            inBonusArea,
            flagY
        );

        if (!inBonusArea)
        {
            DrawCastle(textures.castle);
        }

        DrawBrokenBrickAnimations(
            brokenBrickAnimations,
            textures.brokenBrick,
            textures.blueBrokenBrick,
            textures.brickPiece1,
            textures.brickPiece2
        );

        if (inBonusArea)
        {
            DrawBlueCoins(
                coins,
                textures.blueCoin1,
                textures.blueCoin2,
                textures.blueCoin3
            );
        }
        else
        {
            DrawCoins(
                coins,
                textures.coin
            );
        }

        DrawBlockCoins(
            blockCoins,
            textures.coinFlash1,
            textures.coinFlash2,
            textures.coinFlash3,
            textures.coinFlash4
        );

        // The original game selects the second enemy frame using bit 3
        // of its NTSC frame counter.
        int nesFrame = static_cast<int>(GetTime() * nesNtscFrameRate);
        bool alternateWalkFrame = (nesFrame & 0x08) != 0;

        for (const Goomba& goomba : goombas)
        {
            DrawGoomba(
                goomba,
                textures.goomba,
                textures.goombaDead,
                alternateWalkFrame
            );
        }

        for (const Koopa& koopa : koopas)
        {
            DrawKoopa(
                koopa,
                textures.koopa,
                textures.koopaWalk,
                alternateWalkFrame,
                textures.greenShell,
                textures.greenShellWake
            );
        }

        DrawFireBalls(
            fireBalls,
            textures.fireBall,
            textures.fireBallHit1,
            textures.fireBallHit2,
            textures.fireBallHit3
        );

        if (!drawPlayerBehindTiles)
        {
            if (
                finishSequence == FinishSequence::Sliding ||
                finishSequence == FinishSequence::Turning
            )
            {
                DrawPlayerOnFlagPole(
                    player,
                    textures,
                    finishSequence == FinishSequence::Sliding &&
                        (player.walkAnimationFrame & 1) != 0,
                    finishSequence == FinishSequence::Turning
                );
            }
            else if (
                finishSequence == FinishSequence::InsideCastle ||
                finishSequence == FinishSequence::Blackout
            )
            {
                // Mario has entered the castle and is no longer visible.
            }
            else if (IsPlayerTransforming(player))
            {
                Player transformingPlayer = player;
                transformingPlayer.onGround = true;
                transformingPlayer.velocity = {0.0f, 0.0f};
                int palette = -1;

                if (player.growthTransformTimer > 0.0f)
                {
                    constexpr bool growthSequence[] = {
                        false, true, false, true, false,
                        true, true, false, true, true
                    };

                    float animationTime =
                        growthTransformDuration -
                        player.growthTransformTimer -
                        growthAnimationDelay;
                    bool drawBig = false;
                    if (animationTime >= 0.0f)
                    {
                        int animationFrame = std::clamp(
                            static_cast<int>(
                                animationTime *
                                nesNtscFrameRate / 4.0f
                            ),
                            0,
                            9
                        );
                        drawBig = growthSequence[animationFrame];
                    }

                    transformingPlayer.big = drawBig;
                    transformingPlayer.fire =
                        player.fire && drawBig;
                }
                else
                {
                    float animationTime =
                        growthTransformDuration -
                        player.fireTransformTimer;
                    int animationFrame = static_cast<int>(
                        animationTime * nesNtscFrameRate
                    );
                    palette = (animationFrame / 4) & 0x03;
                    transformingPlayer.fire = false;
                }

                DrawPlayer(
                    transformingPlayer,
                    textures,
                    true,
                    palette
                );
            }
            else
            {
                DrawPlayer(player, textures);
            }
        }

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

        if (finishSequence == FinishSequence::Blackout)
        {
            DrawRectangle(0, 0, virtualWidth, virtualHeight, BLACK);
        }

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
    UnloadTexture(textures.brokenBrick);
    UnloadTexture(textures.brickPiece1);
    UnloadTexture(textures.brickPiece2);
    UnloadTexture(textures.blueGround);
    UnloadTexture(textures.blueBrick);
    UnloadTexture(textures.blueBrokenBrick);
    UnloadTexture(textures.questionBlock1);
    UnloadTexture(textures.questionBlock2);
    UnloadTexture(textures.questionBlock3);
    UnloadTexture(textures.emptyBlock);
    UnloadTexture(textures.stairBlock);
    UnloadTexture(textures.pipe);
    UnloadTexture(textures.pipeBase);
    UnloadTexture(textures.undergroundPipeConnector);
    UnloadTexture(textures.undergroundPipeBase);
    UnloadTexture(textures.coin);
    UnloadTexture(textures.coinFlash1);
    UnloadTexture(textures.coinFlash2);
    UnloadTexture(textures.coinFlash3);
    UnloadTexture(textures.coinFlash4);
    UnloadTexture(textures.blueCoin1);
    UnloadTexture(textures.blueCoin2);
    UnloadTexture(textures.blueCoin3);
    UnloadTexture(textures.bigHill);
    UnloadTexture(textures.smallHill);
    UnloadTexture(textures.singleBush);
    UnloadTexture(textures.doubleBush);
    UnloadTexture(textures.tripleBush);
    UnloadTexture(textures.singleCloud);
    UnloadTexture(textures.doubleCloud);
    UnloadTexture(textures.tripleCloud);
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
    UnloadTexture(textures.goombaDead);
    UnloadTexture(textures.koopa);
    UnloadTexture(textures.koopaWalk);
    UnloadTexture(textures.greenShell);
    UnloadTexture(textures.greenShellWake);
    UnloadTexture(textures.superMushroom);
    UnloadTexture(textures.oneUpMushroom);
    UnloadTexture(textures.fireFlower1);
    UnloadTexture(textures.fireFlower2);
    UnloadTexture(textures.fireFlower3);
    UnloadTexture(textures.fireFlower4);
    UnloadTexture(textures.star1);
    UnloadTexture(textures.star2);
    UnloadTexture(textures.star3);
    UnloadTexture(textures.star4);
    UnloadStarMarioTextures(textures.brownStarMario);
    UnloadStarMarioTextures(textures.greenStarMario);
    UnloadStarMarioTextures(textures.redStarMario);
    UnloadTexture(textures.fireBall);
    UnloadTexture(textures.fireBallHit1);
    UnloadTexture(textures.fireBallHit2);
    UnloadTexture(textures.fireBallHit3);
    UnloadTexture(textures.flag);
    UnloadTexture(textures.flagPole);
    UnloadTexture(textures.smallMarioFlag1);
    UnloadTexture(textures.smallMarioFlag2);
    UnloadTexture(textures.superMarioFlag1);
    UnloadTexture(textures.superMarioFlag2);
    UnloadTexture(textures.fireMarioFlag1);
    UnloadTexture(textures.fireMarioFlag2);
    UnloadTexture(textures.castle);
    UnloadTexture(textures.title);

    UnloadRenderTexture(gameTexture);
    UnloadGameSounds(gameSounds);
    StopMusicStream(backgroundMusic);
    StopMusicStream(undergroundMusic);
    StopMusicStream(starMusic);
    UnloadMusicStream(backgroundMusic);
    UnloadMusicStream(undergroundMusic);
    UnloadMusicStream(starMusic);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
