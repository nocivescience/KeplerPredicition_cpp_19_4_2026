#include <SFML/Graphics.hpp> // Para sf::RenderWindow, sf::CircleShape, etc.
#include <SFML/Window.hpp> // Para mapPixelToCoords
#include <vector>
#include <cmath>
#include <iostream>
#include <string>

// --- Constantes Globales ---
const float dT = 0.01f; // Paso de tiempo para la simulación
const float G = 0.9f; // Constante gravitacional (ajustada para
// la escala de la simulación)
const float PI = 3.14159265f; // Valor de pi

float mag(sf::Vector2f v) { return std::sqrt(v.x * v.x + v.y * v.y); } // Magnitud de un vector
sf::Vector2f normalize(sf::Vector2f v) // Normalización de un vector