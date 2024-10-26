#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>

// Particle structure representing individual firework particles
struct Particle {
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float lifespan; // Lifespan in seconds

    Particle(sf::Vector2f position, sf::Vector2f velocity, sf::Color color, float lifespanDuration) {
        shape.setPosition(position);
        shape.setRadius(2.f);
        shape.setFillColor(color);
        shape.setOrigin(2.f, 2.f); // Center the circle
        this->velocity = velocity;
        this->lifespan = lifespanDuration;
    }

    void update(float dt) {
        // Update position based on velocity
        shape.move(velocity * dt);
        // Apply gravity (pixels/s²)
        velocity.y += 98.1f * dt;
        // Decrease lifespan
        lifespan -= dt;
    }
};

// Firework structure managing a collection of particles
struct Firework {
    std::vector<Particle> particles;
    bool exploded;
    sf::Vector2f explosionPosition;

    Firework(sf::Vector2f position) : exploded(false), explosionPosition(position) {
        // Initial upward velocity for the rocket
        sf::Vector2f velocity(0.f, -300.f);
        // Create the initial rocket particle
        particles.emplace_back(position, velocity, sf::Color::White, 2.f);
    }

    void update(float dt) {
        if (!exploded) {
            // Update the rocket
            particles[0].update(dt);
            // Check if the rocket should explode (when velocity.y >= 0)
            if (particles[0].velocity.y >= 0.f) {
                explode();
            }
        } else {
            // Update all explosion particles
            for (auto it = particles.begin(); it != particles.end(); ) {
                it->update(dt);
                if (it->lifespan <= 0.f) {
                    it = particles.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }

    void explode() {
        exploded = true;
        // Store the explosion position before removing the rocket
        explosionPosition = particles[0].shape.getPosition();
        // Remove the rocket particle
        particles.erase(particles.begin());

        // Generate explosion particles
        int numParticles = 100;
        for (int i = 0; i < numParticles; ++i) {
            float angle = static_cast<float>(std::rand()) / RAND_MAX * 2 * static_cast<float>(M_PI);
            float speed = static_cast<float>(std::rand()) / RAND_MAX * 200.f + 50.f;
            sf::Vector2f velocity(std::cos(angle) * speed, std::sin(angle) * speed);

            sf::Color color = sf::Color(std::rand() % 256, std::rand() % 256, std::rand() % 256);

            particles.emplace_back(explosionPosition, velocity, color, 2.f);
        }
    }

    bool isFinished() const {
        return exploded && particles.empty();
    }
};

int main() {
    // Initialize random seed
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // Create the main SFML window
    sf::RenderWindow window(sf::VideoMode(800, 600), "Fireworks Simulation");
    window.setFramerateLimit(60);

    // Vector to hold all active fireworks
    std::vector<Firework> fireworks;

    // Clock to track delta time
    sf::Clock clock;

    while (window.isOpen()) {
        // Calculate delta time
        float dt = clock.restart().asSeconds();

        // Handle events
        sf::Event event;
        while (window.pollEvent(event)) {
            // Close window event
            if (event.type == sf::Event::Closed)
                window.close();

            // Launch a firework on left mouse button click
            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f position(static_cast<float>(event.mouseButton.x), static_cast<float>(event.mouseButton.y));
                    fireworks.emplace_back(position);
                }
            }
        }

        // Update all fireworks
        for (auto it = fireworks.begin(); it != fireworks.end(); ) {
            it->update(dt);
            if (it->isFinished()) {
                it = fireworks.erase(it);
            } else {
                ++it;
            }
        }

        // Optionally, launch fireworks automatically at random intervals
        if (std::rand() % 100 < 2) { // Adjust probability as needed
            sf::Vector2f position(static_cast<float>(std::rand() % window.getSize().x), static_cast<float>(window.getSize().y));
            fireworks.emplace_back(position);
        }

        // Clear the window with black color (night sky)
        window.clear(sf::Color::Black);

        // Draw all particles of all fireworks
        for (const auto& firework : fireworks) {
            for (const auto& particle : firework.particles) {
                window.draw(particle.shape);
            }
        }

        // Display the rendered frame on screen
        window.display();
    }

    return 0;
}
