#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <cmath>
#include <iostream>
#include <string>

// --- Constantes Globales ---
const float dT = 0.01f;
const float G = 0.9f;
const float PI = 3.14159265f;

// --- Funciones de Utilidad Vectorial ---
float mag(sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); }
sf::Vector2f normalize(sf::Vector2f v) {
    float m = mag(v);
    if (m != 0) return v / m;
    return v;
}
float dot(sf::Vector2f v1, sf::Vector2f v2) { return v1.x * v2.x + v1.y * v2.y; }

struct OrbitData {
    float sM, sm, ecc, orient;
};

class GravityBody {
public:
    sf::Vector2f pos, vel, acc;
    float size, mass, soi = 0;

    GravityBody(float r, float m) : size(r), mass(m) {}

    void update() {
        vel += acc * dT;
        pos += vel * dT;
        acc = {0.f, 0.f};
    }

    void setParent(const GravityBody& target) {
        sf::Vector2f relPos = pos - target.pos;
        sf::Vector2f relVel = vel - target.vel;
        float _pos_ = mag(relPos);
        float _vel_ = mag(relVel);
        
        float u = mass + target.mass;
        float a = -(u * _pos_) / (_pos_ * _vel_ * _vel_ - (u + u));
        soi = a * std::pow(mass / target.mass, 0.4f);
    }

    void draw(sf::RenderWindow& window, const sf::Font& font) {
        // Dibujar SOI (Círculo tenue)
        sf::CircleShape soiCircle(soi);
        soiCircle.setOrigin({soi, soi});
        soiCircle.setPosition(pos);
        soiCircle.setFillColor(sf::Color(255, 255, 255, 30));
        window.draw(soiCircle);

        // Dibujar Cuerpo
        sf::CircleShape bodyCircle(size);
        bodyCircle.setOrigin({size, size});
        bodyCircle.setPosition(pos);
        bodyCircle.setFillColor(sf::Color::White);
        window.draw(bodyCircle);

        // Texto de Masa
        sf::Text text(font, std::to_string((int)mass), 12);
        text.setPosition(pos + sf::Vector2f(size, size));
        text.setFillColor(sf::Color(80, 240, 150));
        window.draw(text);
    }
};

// --- Cálculos Orbitales ---
sf::Vector2f getAcceleration(const GravityBody& a, const GravityBody& b) {
    sf::Vector2f disp = b.pos - a.pos;
    float dist = mag(disp);
    if (dist < 1.0f) return {0, 0}; 
    return normalize(disp) * (G * b.mass / (dist * dist));
}

OrbitData getOrbitPrediction(const GravityBody& gb1, const GravityBody& gb2) {
    sf::Vector2f r = gb1.pos - gb2.pos;
    sf::Vector2f v = gb1.vel - gb2.vel;
    float _r = mag(r);
    float _v = mag(v);

    float u = G * gb2.mass;
    float a = -(u * _r) / (_r * _v * _v - (u + u));
    float angP = r.x * v.y - r.y * v.x;
    float E = (_v * _v / 2.0f) - (u / _r);
    float e = std::sqrt(std::max(0.0f, 1.0f + (2.0f * E * angP * angP) / (u * u)));
    
    sf::Vector2f eV = (r * ((_v * _v / u) - (1.0f / _r))) - (v * (dot(r, v) / u));
    float o = std::atan2(eV.y, eV.x);
    float b = a * std::sqrt(std::abs(1.0f - e * e));

    return {a, b, e, o};
}

void drawOrbit(sf::RenderWindow& window, const GravityBody& parent, const OrbitData& data) {
    sf::RectangleShape ellipse; // Usamos CircleShape con escala para elipses en SFML
    float radius = data.sM;
    sf::CircleShape orbitVisual(radius);
    orbitVisual.setPointCount(100);
    orbitVisual.setOrigin({radius, radius});
    
    // Posicionar en el foco de la elipse
    float focusDist = data.sM * data.ecc;
    orbitVisual.setPosition(parent.pos);
    orbitVisual.setRotation(sf::radians(data.orient));
    orbitVisual.move(sf::Vector2f(std::cos(data.orient), std::sin(data.orient)) * -focusDist);
    
    orbitVisual.setScale({1.0f, data.sm / data.sM});
    orbitVisual.setFillColor(sf::Color::Transparent);
    orbitVisual.setOutlineThickness(1.0f);
    orbitVisual.setOutlineColor(sf::Color(255, 255, 255, 100));
    
    window.draw(orbitVisual);
}

int main() {
    sf::RenderWindow window(sf::VideoMode({1200, 800}), "Gravity Simulation SFML 3");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.openFromFile("arial.ttf")) return -1; // Asegúrate de tener una fuente

    std::vector<GravityBody> bodies;
    bodies.emplace_back(25.0f, 1e8f); // Sol
    bodies.emplace_back(3.0f, 1000.0f); // Planeta

    bodies[0].pos = {600, 400};
    bodies[1].pos = {880, 400};
    bodies[1].vel = {0, 500};

    bool recording = false;
    sf::Vector2f startPos, mousePos;
    float startM = 20.0f;

    while (window.isOpen()) {
        while (const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Up) startM += 5.0f;
                if (keyPressed->code == sf::Keyboard::Key::Down) startM = std::max(5.0f, startM - 5.0f);
            }

            if (const auto* mouseB = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mouseB->button == sf::Mouse::Button::Left) {
                    recording = true;
                    startPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                }
            }

            if (const auto* mouseB = event->getIf<sf::Event::MouseButtonReleased>()) {
                if (mouseB->button == sf::Mouse::Button::Left && recording) {
                    sf::Vector2f endPos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    GravityBody newBody(std::log(startM), startM);
                    newBody.pos = startPos;
                    newBody.vel = (endPos - startPos) * 0.8f;
                    bodies.push_back(newBody);
                    recording = false;
                }
            }
        }

        // --- Lógica de Física ---
        for (size_t i = 0; i < bodies.size(); i++) {
            size_t parentI = 0;
            float maxAccMag = -1.0f;

            for (size_t j = 0; j < bodies.size(); j++) {
                if (i == j) continue;
                sf::Vector2f grav = getAcceleration(bodies[i], bodies[j]);
                bodies[i].acc += grav;

                float currMag = mag(grav);
                if (currMag > maxAccMag && bodies[j].mass > bodies[i].mass) {
                    maxAccMag = currMag;
                    parentI = j;
                }
            }
            bodies[i].update();
            if (i != parentI) bodies[i].setParent(bodies[parentI]);
        }

        // --- Renderizado ---
        window.clear(sf::Color(10, 10, 10));

        for (size_t i = 0; i < bodies.size(); i++) {
            // Encontrar padre para dibujar órbita
            size_t parentI = 0;
            float maxM = -1.0f;
            for(size_t j=0; j<bodies.size(); j++) {
                if(i != j && bodies[j].mass > maxM) { 
                    maxM = bodies[j].mass; parentI = j; 
                }
            }
            
            if (i != parentI) {
                OrbitData data = getOrbitPrediction(bodies[i], bodies[parentI]);
                drawOrbit(window, bodies[parentI], data);
            }
            bodies[i].draw(window, font);
        }

        if (recording) {
            mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
            sf::Vertex line[] = { sf::Vertex{startPos, sf::Color::Yellow}, sf::Vertex{mousePos, sf::Color::Yellow} };
            window.draw(line, 2, sf::PrimitiveType::Lines);
        }

        window.display();
    }
    return 0;
}