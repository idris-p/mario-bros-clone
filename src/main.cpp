#include "raylib.h"

struct Player
{
    Rectangle body;
    Vector2 velocity;
    bool onGround;
};

bool LandOnPlatform(
    Player& player,
    const Rectangle& platform,
    float previousBottom,
    float currentBottom
)
{
    bool horizontallyOverlapping =
        player.body.x + player.body.width > platform.x &&
        player.body.x < platform.x + platform.width;

    bool crossedPlatformTop =
        previousBottom <= platform.y &&
        currentBottom >= platform.y;

    if (
        horizontallyOverlapping &&
        crossedPlatformTop &&
        player.velocity.y >= 0.0f
    )
    {
        player.body.y = platform.y - player.body.height;
        player.velocity.y = 0.0f;
        player.onGround = true;

        return true;
    }

    return false;
}

int main()
{
    constexpr int screenWidth = 960;
    constexpr int screenHeight = 540;

    // Approximate Mario-style movement values.
    constexpr float walkSpeed = 280.0f;
    constexpr float sprintSpeed = 450.0f;

    constexpr float walkingAcceleration = 1400.0f;
    constexpr float sprintAcceleration = 1900.0f;
    constexpr float friction = 1800.0f;

    // Gives a maximum jump height of roughly 200 pixels:
    // jumpHeight = jumpSpeed² / (2 × gravity)
    constexpr float jumpSpeed = 850.0f;
    constexpr float gravity = 1800.0f;

    constexpr float jumpBufferDuration = 0.12f;

    InitWindow(
        screenWidth,
        screenHeight,
        "Super Mario Bros. Clone"
    );

    SetTargetFPS(60);

    Player player{
        {100.0f, 300.0f, 40.0f, 50.0f},
        {0.0f, 0.0f},
        false
    };

    Rectangle ground{
        0.0f,
        450.0f,
        1500.0f,
        90.0f
    };

    // The top is 180 pixels above the ground.
    // The new jump can comfortably reach this.
    Rectangle skyPlatform{
        340.0f,
        270.0f,
        240.0f,
        30.0f
    };

    float jumpBufferTimer = 0.0f;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // Prevent unusually long frames from causing physics problems.
        if (deltaTime > 0.05f)
        {
            deltaTime = 0.05f;
        }

        // -----------------------------
        // Input
        // -----------------------------

        float direction = 0.0f;

        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))
        {
            direction -= 1.0f;
        }

        if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT))
        {
            direction += 1.0f;
        }

        bool sprinting =
            IsKeyDown(KEY_LEFT_SHIFT) ||
            IsKeyDown(KEY_RIGHT_SHIFT);

        float currentMaxSpeed =
            sprinting ? sprintSpeed : walkSpeed;

        float currentAcceleration =
            sprinting
                ? sprintAcceleration
                : walkingAcceleration;

        // -----------------------------
        // Jump buffering
        // -----------------------------

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W))
        {
            jumpBufferTimer = jumpBufferDuration;
        }
        else if (jumpBufferTimer > 0.0f)
        {
            jumpBufferTimer -= deltaTime;
        }

        // -----------------------------
        // Horizontal movement
        // -----------------------------

        if (direction != 0.0f)
        {
            player.velocity.x +=
                direction *
                currentAcceleration *
                deltaTime;

            if (player.velocity.x > currentMaxSpeed)
            {
                player.velocity.x = currentMaxSpeed;
            }

            if (player.velocity.x < -currentMaxSpeed)
            {
                player.velocity.x = -currentMaxSpeed;
            }
        }
        else
        {
            if (player.velocity.x > 0.0f)
            {
                player.velocity.x -= friction * deltaTime;

                if (player.velocity.x < 0.0f)
                {
                    player.velocity.x = 0.0f;
                }
            }
            else if (player.velocity.x < 0.0f)
            {
                player.velocity.x += friction * deltaTime;

                if (player.velocity.x > 0.0f)
                {
                    player.velocity.x = 0.0f;
                }
            }
        }

        // If sprint is released, gradually bring the player
        // back down to walking speed rather than snapping.
        if (
            !sprinting &&
            direction != 0.0f &&
            player.velocity.x > walkSpeed
        )
        {
            player.velocity.x -= friction * deltaTime;

            if (player.velocity.x < walkSpeed)
            {
                player.velocity.x = walkSpeed;
            }
        }

        if (
            !sprinting &&
            direction != 0.0f &&
            player.velocity.x < -walkSpeed
        )
        {
            player.velocity.x += friction * deltaTime;

            if (player.velocity.x > -walkSpeed)
            {
                player.velocity.x = -walkSpeed;
            }
        }

        // Jump immediately if already grounded.
        if (player.onGround && jumpBufferTimer > 0.0f)
        {
            player.velocity.y = -jumpSpeed;
            player.onGround = false;
            jumpBufferTimer = 0.0f;
        }

        // -----------------------------
        // Physics
        // -----------------------------

        player.velocity.y += gravity * deltaTime;

        player.body.x += player.velocity.x * deltaTime;

        float previousBottom =
            player.body.y + player.body.height;

        player.body.y += player.velocity.y * deltaTime;

        float currentBottom =
            player.body.y + player.body.height;

        player.onGround = false;

        // -----------------------------
        // Platform collisions
        // -----------------------------

        LandOnPlatform(
            player,
            ground,
            previousBottom,
            currentBottom
        );

        LandOnPlatform(
            player,
            skyPlatform,
            previousBottom,
            currentBottom
        );

        // A buffered jump executes immediately upon landing.
        if (player.onGround && jumpBufferTimer > 0.0f)
        {
            player.velocity.y = -jumpSpeed;
            player.onGround = false;
            jumpBufferTimer = 0.0f;
        }

        // -----------------------------
        // Reset
        // -----------------------------

        if (player.body.y > screenHeight)
        {
            player.body.x = 100.0f;
            player.body.y = 300.0f;
            player.velocity = {0.0f, 0.0f};
            player.onGround = false;
            jumpBufferTimer = 0.0f;
        }

        // -----------------------------
        // Drawing
        // -----------------------------

        BeginDrawing();

        ClearBackground(SKYBLUE);

        DrawRectangleRec(ground, BROWN);
        DrawRectangleRec(skyPlatform, DARKBROWN);
        DrawRectangleRec(player.body, RED);

        DrawText(
            "A/D or arrows: move",
            20,
            20,
            22,
            BLACK
        );

        DrawText(
            "Shift: sprint",
            20,
            48,
            22,
            BLACK
        );

        DrawText(
            "Space or W: jump",
            20,
            76,
            22,
            BLACK
        );

        DrawText(
            TextFormat(
                "Speed: %.0f",
                player.velocity.x
            ),
            20,
            110,
            20,
            BLACK
        );

        EndDrawing();
    }

    CloseWindow();

    return 0;
}