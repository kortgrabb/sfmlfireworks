#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <SFML/Graphics/Shader.hpp>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;
const float PLAYER_SPEED = 300.f;
const float JUMP_VELOCITY = -400.f;
const float GRAVITY = 981.f;
const float BULLET_SPEED = 800.f;
const int VOXEL_SIZE = 4;                    // Size of each "2D voxel"
const float DRAW_RADIUS = 3.0f * VOXEL_SIZE; // Radius for voxel spawning
const float STEP_HEIGHT = VOXEL_SIZE * 1.0f; // Maximum height player can automatically step up
const float GLOW_STRENGTH = 1000.0f;
const float EXPLOSION_RADIUS = VOXEL_SIZE * 4.0f;
const float PARTICLE_LIFETIME = 1.2f;
const float SCREEN_SHAKE_DURATION = 0.2f;
const float SCREEN_SHAKE_INTENSITY = 8.0f;

// Helper functions for vector operations
float vectorLength(const sf::Vector2f &vec)
{
    return std::sqrt(vec.x * vec.x + vec.y * vec.y);
}

sf::Vector2f normalize(const sf::Vector2f &vec)
{
    float length = vectorLength(vec);
    return length > 0 ? vec / length : vec;
}

class Player
{
public:
    sf::RectangleShape shape;
    sf::Vector2f velocity;
    bool isJumping;

    Player() : isJumping(false)
    {
        shape.setSize(sf::Vector2f(30.f, 30.f));
        shape.setFillColor(sf::Color::Green);
        shape.setPosition(100.f, WINDOW_HEIGHT - 100.f);
        velocity = sf::Vector2f(0.f, 0.f);
    }
};

class Bullet
{
public:
    sf::RectangleShape shape;
    sf::Vector2f velocity;

    Bullet(const sf::Vector2f &pos, const sf::Vector2f &vel) : velocity(vel)
    {
        shape.setSize(sf::Vector2f(5.f, 5.f));
        shape.setFillColor(sf::Color::Yellow);
        shape.setPosition(pos);
    }
};

class Voxel
{
public:
    sf::RectangleShape shape;

    Voxel(float x, float y)
    {
        shape.setSize(sf::Vector2f(VOXEL_SIZE, VOXEL_SIZE));
        shape.setFillColor(sf::Color::White);
        shape.setPosition(x, y);
    }
};

class Particle
{
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float lifetime;

    Particle(const sf::Vector2f &pos, const sf::Vector2f &vel, const sf::Color &color)
        : velocity(vel), lifetime(PARTICLE_LIFETIME)
    {
        shape.setRadius(2.f);
        shape.setFillColor(color);
        shape.setPosition(pos);
    }

    bool update(float deltaTime)
    {
        lifetime -= deltaTime;
        shape.move(velocity * deltaTime);
        shape.setFillColor(sf::Color(
            shape.getFillColor().r,
            shape.getFillColor().g,
            shape.getFillColor().b,
            static_cast<sf::Uint8>(255 * (lifetime / PARTICLE_LIFETIME))));
        return lifetime > 0;
    }
};

class Game
{
private:
    sf::RenderWindow window;
    Player player;
    std::vector<Bullet> bullets;
    std::vector<Voxel> voxels;
    std::vector<Particle> particles;
    bool isDrawing;
    sf::Shader backgroundShader;
    sf::Shader glowShader;
    sf::RenderTexture bulletLayer;
    sf::Clock shaderClock;
    float screenShakeTime = 0.0f;
    sf::Vector2f screenShakeOffset;

public:
    Game() : window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "2D Shooter"), isDrawing(false)
    {
        window.setFramerateLimit(60);

        // Initialize shaders
        if (!sf::Shader::isAvailable())
        {
            throw std::runtime_error("Shaders are not available!");
        }

        if (!backgroundShader.loadFromFile("shaders/background.frag", sf::Shader::Fragment))
        {
            throw std::runtime_error("Could not load background shader!");
        }

        if (!glowShader.loadFromFile("shaders/glow.frag", sf::Shader::Fragment))
        {
            throw std::runtime_error("Could not load glow shader!");
        }

        // Initialize render texture for bullet layer
        bulletLayer.create(WINDOW_WIDTH, WINDOW_HEIGHT);

        // Set shader parameters
        backgroundShader.setUniform("resolution", sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
        glowShader.setUniform("glowStrength", GLOW_STRENGTH);
    }

    void run()
    {
        sf::Clock clock;

        while (window.isOpen())
        {
            sf::Time deltaTime = clock.restart();
            handleEvents();
            update(deltaTime.asSeconds());
            render();
        }
    }

private:
    bool checkVoxelCollision(const sf::FloatRect &bounds)
    {
        for (const auto &voxel : voxels)
        {
            if (bounds.intersects(voxel.shape.getGlobalBounds()))
            {
                return true;
            }
        }
        return false;
    }

    bool canStepUp(const sf::Vector2f &pos)
    {
        sf::FloatRect playerBounds = player.shape.getGlobalBounds();

        // Check if there's a block in front of us
        playerBounds.left = pos.x;
        playerBounds.top = pos.y;
        if (!checkVoxelCollision(playerBounds))
        {
            return false; // No need to step if no collision
        }

        // Try stepping up
        playerBounds.top = pos.y - STEP_HEIGHT;
        return !checkVoxelCollision(playerBounds);
    }

    void spawnVoxelsInRadius(float centerX, float centerY)
    {
        for (float x = -DRAW_RADIUS; x <= DRAW_RADIUS; x += VOXEL_SIZE)
        {
            for (float y = -DRAW_RADIUS; y <= DRAW_RADIUS; y += VOXEL_SIZE)
            {
                float distance = std::sqrt(x * x + y * y);
                if (distance <= DRAW_RADIUS)
                {
                    float snapX = round((centerX + x) / VOXEL_SIZE) * VOXEL_SIZE;
                    float snapY = round((centerY + y) / VOXEL_SIZE) * VOXEL_SIZE;

                    // Check if voxel already exists
                    bool voxelExists = false;
                    for (const auto &voxel : voxels)
                    {
                        if (voxel.shape.getPosition().x == snapX &&
                            voxel.shape.getPosition().y == snapY)
                        {
                            voxelExists = true;
                            break;
                        }
                    }

                    if (!voxelExists)
                    {
                        voxels.emplace_back(snapX, snapY);
                    }
                }
            }
        }
    }

    void handleCollisions(sf::Vector2f &newPos, float deltaTime)
    {
        sf::FloatRect playerBounds = player.shape.getGlobalBounds();
        sf::Vector2f oldPos = player.shape.getPosition();

        // First, try horizontal movement
        playerBounds.left = newPos.x;
        playerBounds.top = oldPos.y;
        if (checkVoxelCollision(playerBounds))
        {
            // Try stepping up before blocking horizontal movement
            if (canStepUp(sf::Vector2f(newPos.x, oldPos.y)))
            {
                // Move up by step height
                newPos.y = oldPos.y - STEP_HEIGHT;
            }
            else
            {
                // Can't step up, block horizontal movement
                newPos.x = oldPos.x;
                player.velocity.x = 0;
            }
        }

        // Then, try vertical movement
        playerBounds.left = newPos.x;
        playerBounds.top = newPos.y;
        if (checkVoxelCollision(playerBounds))
        {
            if (player.velocity.y > 0)
            {
                newPos.y = oldPos.y;
                player.velocity.y = 0;
                player.isJumping = false;
            }
            else if (player.velocity.y < 0)
            {
                newPos.y = oldPos.y;
                player.velocity.y = 0;
            }
        }

        // Apply gravity after stepping
        if (!checkVoxelCollision(playerBounds))
        {
            sf::FloatRect groundCheck = playerBounds;
            groundCheck.top += 1.0f;
            if (!checkVoxelCollision(groundCheck))
            {
                player.isJumping = true;
            }
        }
    }

    void handleEvents()
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    isDrawing = true;
                }
            }
            if (event.type == sf::Event::MouseButtonReleased)
            {
                if (event.mouseButton.button == sf::Mouse::Left)
                {
                    isDrawing = false;
                }
            }
            if (event.type == sf::Event::MouseButtonPressed)
            {
                if (event.mouseButton.button == sf::Mouse::Right)
                {
                    // Shoot bullet
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    sf::Vector2f playerCenter = player.shape.getPosition() +
                                                sf::Vector2f(player.shape.getSize().x / 2, player.shape.getSize().y / 2);

                    sf::Vector2f direction = sf::Vector2f(mousePos.x - playerCenter.x,
                                                          mousePos.y - playerCenter.y);
                    float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
                    direction /= length;

                    bullets.emplace_back(playerCenter, direction * BULLET_SPEED);
                }
            }
        }

        // Handle continuous drawing while mouse is held
        if (isDrawing)
        {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            spawnVoxelsInRadius(mousePos.x, mousePos.y);
        }
    }

    void createExplosion(const sf::Vector2f &position)
    {
        // Screen shake
        screenShakeTime = SCREEN_SHAKE_DURATION;

        // Create explosion particles
        for (int i = 0; i < 20; ++i)
        {
            float angle = (rand() % 360) * 3.14159f / 180.f;
            float speed = 100.f + (rand() % 100);
            sf::Vector2f velocity(cos(angle) * speed, sin(angle) * speed);
            particles.emplace_back(position, velocity, sf::Color(255, 200, 0));
        }

        // Destroy nearby voxels
        for (auto it = voxels.begin(); it != voxels.end();)
        {
            sf::Vector2f voxelPos = it->shape.getPosition();
            float dx = voxelPos.x - position.x;
            float dy = voxelPos.y - position.y;
            float distance = sqrt(dx * dx + dy * dy);

            if (distance < EXPLOSION_RADIUS)
            {
                // Create debris particles
                for (int i = 0; i < 3; ++i)
                {
                    float angle = (rand() % 360) * 3.14159f / 180.f;
                    float speed = 50.f + (rand() % 50);
                    sf::Vector2f velocity(cos(angle) * speed, sin(angle) * speed);
                    particles.emplace_back(voxelPos, velocity, sf::Color::White);
                }
                it = voxels.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void updateScreenShake(float deltaTime)
    {
        if (screenShakeTime > 0)
        {
            screenShakeTime -= deltaTime;
            float intensity = (screenShakeTime / SCREEN_SHAKE_DURATION) * SCREEN_SHAKE_INTENSITY;
            screenShakeOffset = sf::Vector2f(
                (rand() % 100 - 50) * 0.01f * intensity,
                (rand() % 100 - 50) * 0.01f * intensity);
        }
        else
        {
            screenShakeOffset = sf::Vector2f(0, 0);
        }
    }

    void update(float deltaTime)
    {
        // Player movement
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A))
            player.velocity.x = -PLAYER_SPEED;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D))
            player.velocity.x = PLAYER_SPEED;
        else
            player.velocity.x = 0;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && !player.isJumping)
        {
            player.velocity.y = JUMP_VELOCITY;
            player.isJumping = true;
        }

        // Apply gravity
        player.velocity.y += GRAVITY * deltaTime;

        // Update player position
        sf::Vector2f newPos = player.shape.getPosition();
        newPos += player.velocity * deltaTime;

        handleCollisions(newPos, deltaTime);

        // Window bounds checking
        if (newPos.x < 0)
            newPos.x = 0;
        if (newPos.x > WINDOW_WIDTH - player.shape.getSize().x)
            newPos.x = WINDOW_WIDTH - player.shape.getSize().x;
        if (newPos.y > WINDOW_HEIGHT - player.shape.getSize().y)
        {
            newPos.y = WINDOW_HEIGHT - player.shape.getSize().y;
            player.velocity.y = 0;
            player.isJumping = false;
        }

        player.shape.setPosition(newPos);

        // Update bullets
        for (auto it = bullets.begin(); it != bullets.end();)
        {
            it->shape.move(it->velocity * deltaTime);

            // Create bullet trail particles
            if (rand() % 2 == 0)
            {
                particles.emplace_back(
                    it->shape.getPosition(),
                    sf::Vector2f(0, 0),
                    sf::Color(255, 255, 0, 128));
            }

            // Check bullet collision with voxels
            bool bulletHit = false;
            for (const auto &voxel : voxels)
            {
                if (it->shape.getGlobalBounds().intersects(voxel.shape.getGlobalBounds()))
                {
                    createExplosion(it->shape.getPosition());
                    bulletHit = true;
                    break;
                }
            }

            // Remove bullets that are out of bounds or hit voxels
            if (bulletHit ||
                it->shape.getPosition().x < 0 ||
                it->shape.getPosition().x > WINDOW_WIDTH ||
                it->shape.getPosition().y < 0 ||
                it->shape.getPosition().y > WINDOW_HEIGHT)
            {
                it = bullets.erase(it);
            }
            else
            {
                ++it;
            }
        }

        // Update screen shake
        updateScreenShake(deltaTime);

        // Update particles
        for (auto it = particles.begin(); it != particles.end();)
        {
            if (!it->update(deltaTime))
            {
                it = particles.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void render()
    {
        // Clear all layers
        window.clear();
        bulletLayer.clear(sf::Color::Transparent);

        // Update background shader time
        backgroundShader.setUniform("time", shaderClock.getElapsedTime().asSeconds());

        // Draw background with shader
        sf::RectangleShape background(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
        window.draw(background, &backgroundShader);

        // Apply screen shake to view
        sf::View view = window.getDefaultView();
        view.move(screenShakeOffset);
        window.setView(view);

        // Draw voxels
        for (const auto &voxel : voxels)
        {
            window.draw(voxel.shape);
        }

        // Draw bullets to separate layer with glow shader
        for (const auto &bullet : bullets)
        {
            bulletLayer.draw(bullet.shape);
        }
        bulletLayer.display();

        // Draw bullet layer with glow effect
        sf::Sprite bulletSprite(bulletLayer.getTexture());
        window.draw(bulletSprite, &glowShader);

        // Draw particles
        for (const auto &particle : particles)
        {
            window.draw(particle.shape);
        }

        // Draw player
        window.draw(player.shape);

        window.display();
    }
};

int main()
{
    Game game;
    game.run();
    return 0;
}