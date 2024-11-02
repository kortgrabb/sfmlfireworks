#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <iostream>

// Rope configuration
struct RopeConfig
{
    static constexpr int NUM_POINTS = 40;            // Number of points in rope
    static constexpr float POINT_SPACING = 15.0f;    // Space between points
    static constexpr float GRAVITY = 1000.0f;        // Gravity strength
    static constexpr float POINT_SIZE = 0.0f;        // Visual size of points
    static constexpr int CONSTRAINT_ITERATIONS = 50; // Physics iteration count
    static constexpr float DAMPING = 1.0f;           // Velocity damping (1.0 = no damping)
    static constexpr float START_X = 400.0f;         // Starting X position
    static constexpr float START_Y = 100.0f;         // Starting Y position
    static constexpr float ROPE_THICKNESS = 2.0f;    // Thickness of the rope
};

struct ShaderConfig
{
    static constexpr float DEFAULT_PIXEL_SIZE = 5.0f;
    static constexpr float DEFAULT_CRT_CURVE = 0.0f;
    static constexpr float DEFAULT_SCANLINE = 0.2f;
    static constexpr float DEFAULT_CHROMATIC = 0.3f;
};

class Point
{
public:
    sf::Vector2f position;
    sf::Vector2f oldPosition;
    bool isLocked;

    Point(float x, float y, bool locked = false)
        : position(x, y), oldPosition(x, y), isLocked(locked) {}

    void update(float dt)
    {
        if (!isLocked)
        {
            sf::Vector2f velocity = (position - oldPosition) * RopeConfig::DAMPING;
            oldPosition = position;
            position += velocity + sf::Vector2f(0.0f, RopeConfig::GRAVITY) * (dt * dt);
        }
    }
};

class Stick
{
public:
    Point *pointA;
    Point *pointB;
    float length;

    Stick(Point *a, Point *b) : pointA(a), pointB(b)
    {
        length = std::sqrt(pow(a->position.x - b->position.x, 2) +
                           pow(a->position.y - b->position.y, 2));
    }

    void solve()
    {
        sf::Vector2f delta = pointB->position - pointA->position;
        float currentLength = std::sqrt(delta.x * delta.x + delta.y * delta.y);
        float diff = (currentLength - length) / currentLength;

        if (!pointA->isLocked)
            pointA->position += delta * 0.5f * diff;
        if (!pointB->isLocked)
            pointB->position -= delta * 0.5f * diff;
    }
};

class Obstacle
{
public:
    sf::CircleShape shape;
    float radius;

    Obstacle(float x, float y, float r) : radius(r)
    {
        shape.setRadius(r);
        shape.setPosition(x, y);
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin(r, r);
    }

    bool checkCollision(Point &point)
    {
        sf::Vector2f center = shape.getPosition();
        sf::Vector2f diff = point.position - center;
        float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);

        if (dist < radius)
        {
            float angle = std::atan2(diff.y, diff.x);
            point.position.x = center.x + std::cos(angle) * radius;
            point.position.y = center.y + std::sin(angle) * radius;
            return true;
        }
        return false;
    }
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "Verlet Rope Simulation");
    window.setFramerateLimit(100);

    // Initialize shader and render texture
    sf::Shader shader;
    if (!shader.loadFromFile("combined.frag", sf::Shader::Fragment))
    {
        std::cout << "Error loading shader\n";
        return -1;
    }

    sf::RenderTexture renderTexture;
    if (!renderTexture.create(800, 600))
    {
        std::cout << "Error creating render texture\n";
        return -1;
    }

    float pixelSize = ShaderConfig::DEFAULT_PIXEL_SIZE;
    float crtCurve = ShaderConfig::DEFAULT_CRT_CURVE;
    float scanlineIntensity = ShaderConfig::DEFAULT_SCANLINE;
    float chromaticIntensity = ShaderConfig::DEFAULT_CHROMATIC;
    sf::Clock clock;

    shader.setUniform("resolution", sf::Vector2f(800, 600));
    shader.setUniform("pixel_size", pixelSize);
    shader.setUniform("crt_curve", crtCurve);
    shader.setUniform("scanline_intensity", scanlineIntensity);
    shader.setUniform("chromatic_intensity", chromaticIntensity);

    // Create rope
    std::vector<Point> points;
    std::vector<Stick> sticks;

    // Initialize points using constants
    for (int i = 0; i < RopeConfig::NUM_POINTS; i++)
    {
        points.emplace_back(RopeConfig::START_X,
                            RopeConfig::START_Y + i * RopeConfig::POINT_SPACING,
                            i == 0);
    }

    // Create sticks between points
    for (int i = 0; i < RopeConfig::NUM_POINTS - 1; i++)
    {
        sticks.emplace_back(&points[i], &points[i + 1]);
    }

    // Create obstacles
    std::vector<Obstacle> obstacles;
    obstacles.emplace_back(400, 300, 50);
    obstacles.emplace_back(300, 400, 40);

    // Simulation variables
    const float dt = 1.0f / 100.0f;
    const int iterations = RopeConfig::CONSTRAINT_ITERATIONS;
    bool isDragging = false;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
            else if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)
            {
                isDragging = true;
            }
            else if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left)
            {
                isDragging = false;
            }
            else if (event.type == sf::Event::KeyPressed)
            {
                switch (event.key.code)
                {
                case sf::Keyboard::Up:
                    pixelSize = std::min(pixelSize + 1.0f, 32.0f);
                    shader.setUniform("pixel_size", pixelSize);
                    break;
                case sf::Keyboard::Down:
                    pixelSize = std::max(pixelSize - 1.0f, 1.0f);
                    shader.setUniform("pixel_size", pixelSize);
                    break;
                case sf::Keyboard::Q:
                    crtCurve = std::min(crtCurve + 0.1f, 1.0f);
                    shader.setUniform("crt_curve", crtCurve);
                    break;
                case sf::Keyboard::A:
                    crtCurve = std::max(crtCurve - 0.1f, 0.0f);
                    shader.setUniform("crt_curve", crtCurve);
                    break;
                case sf::Keyboard::W:
                    scanlineIntensity = std::min(scanlineIntensity + 0.1f, 1.0f);
                    shader.setUniform("scanline_intensity", scanlineIntensity);
                    break;
                case sf::Keyboard::S:
                    scanlineIntensity = std::max(scanlineIntensity - 0.1f, 0.0f);
                    shader.setUniform("scanline_intensity", scanlineIntensity);
                    break;
                case sf::Keyboard::E:
                    chromaticIntensity = std::min(chromaticIntensity + 0.1f, 1.0f);
                    shader.setUniform("chromatic_intensity", chromaticIntensity);
                    break;
                case sf::Keyboard::D:
                    chromaticIntensity = std::max(chromaticIntensity - 0.1f, 0.0f);
                    shader.setUniform("chromatic_intensity", chromaticIntensity);
                    break;
                }
            }
        }

        // Update anchor point position to mouse position when dragging
        if (isDragging)
        {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            points[0].position = window.mapPixelToCoords(mousePos);
            points[0].oldPosition = points[0].position;
        }

        // Update points
        for (auto &point : points)
        {
            point.update(dt);
        }

        // Solve constraints
        for (int i = 0; i < iterations; i++)
        {
            for (auto &stick : sticks)
            {
                stick.solve();
            }

            // Check obstacle collisions
            for (auto &point : points)
            {
                for (auto &obstacle : obstacles)
                {
                    obstacle.checkCollision(point);
                }
            }
        }

        // Render to texture
        renderTexture.clear(sf::Color::Black);

        // Draw obstacles to texture
        for (const auto &obstacle : obstacles)
        {
            renderTexture.draw(obstacle.shape);
        }

        // Draw sticks to texture (thick lines)
        for (const auto &stick : sticks)
        {
            sf::Vector2f direction = stick.pointB->position - stick.pointA->position;
            float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            if (length > 0)
            {
                direction /= length;
                sf::Vector2f normal(-direction.y, direction.x);
                normal *= RopeConfig::ROPE_THICKNESS;

                sf::ConvexShape line;
                line.setPointCount(4);
                line.setPoint(0, stick.pointA->position + normal);
                line.setPoint(1, stick.pointB->position + normal);
                line.setPoint(2, stick.pointB->position - normal);
                line.setPoint(3, stick.pointA->position - normal);
                line.setFillColor(sf::Color::White);
                renderTexture.draw(line);
            }
        }

        // Draw points to texture
        for (const auto &point : points)
        {
            sf::CircleShape circle(RopeConfig::POINT_SIZE);
            circle.setFillColor(sf::Color::White);
            circle.setPosition(point.position);
            circle.setOrigin(RopeConfig::POINT_SIZE, RopeConfig::POINT_SIZE);
            renderTexture.draw(circle);
        }

        renderTexture.display();

        // Final render with shader
        window.clear();
        sf::Sprite sprite(renderTexture.getTexture());
        shader.setUniform("texture", sf::Shader::CurrentTexture);
        shader.setUniform("time", clock.getElapsedTime().asSeconds());
        window.draw(sprite, &shader);
        window.display();
    }

    return 0;
}
