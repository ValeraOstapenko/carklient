#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <map>
#include <sstream>
#include <iostream>
#include <winsock.h>

using namespace sf;
using namespace std;

string HOST = "127.0.0.1";
int PORT = 30780;

int WIDTH = 400;
int HEIGHT = 800;
int ROWS = 3;
float ROW_WIDTH = WIDTH / ROWS;
int CAR_HEIGHT = 200;

bool gameOver = false;
int score = 0;
bool didSendScore = false;

int randomRange(int min, int max) {
    return min + (rand() % static_cast<int>(max - min + 1));
}

string int_to_string(int i) {
    stringstream ss;
    ss << i;
    return ss.str();
}

void centerTextAt(Text* text, float x, float y) {
    FloatRect textRect = text->getLocalBounds();
    text->setOrigin(textRect.left + textRect.width / 2.0f, textRect.top + textRect.height / 2.0f);
    text->setPosition(Vector2f(x, y));
}

void centerShapeAt(Shape* shape, float x, float y) {
    FloatRect rect = shape->getLocalBounds();
    shape->setOrigin(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    shape->setPosition(Vector2f(x, y));
}

class Obstacle {
    Texture brownTexture;
    RectangleShape body;

    public:
    float x, y;

    Obstacle(int x, int y) {
        this->x = x;
        this->y = y;
        brownTexture.loadFromFile("brown.png");
        body = RectangleShape(Vector2f(ROW_WIDTH, CAR_HEIGHT));
        body.setTexture(&brownTexture);
        body.setPosition(Vector2f(x * ROW_WIDTH, y));
    }

    void setPosition(float x, float y) {
        this->x = x;
        this->y = y;
        body.setPosition(Vector2f(x * ROW_WIDTH, y));
    }

    void render(RenderWindow* window) {
        window->draw(body);
    }
};

class Player {
    Texture greenTexture;
    RectangleShape body;
public:
    int x = 1;

    Player() {
        greenTexture.loadFromFile("green.png");
        body = RectangleShape(Vector2f(ROW_WIDTH, CAR_HEIGHT));
        body.setTexture(&greenTexture);
        body.setPosition(Vector2f(x * ROW_WIDTH, HEIGHT - CAR_HEIGHT));
    }

    void setPosition(int x) {
        this->x = x;
        body.setPosition(Vector2f(x * ROW_WIDTH, HEIGHT - CAR_HEIGHT));
    }

    void render(RenderWindow* window) {
        window->draw(body);
    }
};

class ObstacleSpawner {
    float speed = 100;
    float maxSpeed = 1000;
    float spawnRate = 5;
    float minSpawnRate = 0.5f;
    float spawnTimer = spawnRate;

    public:
    vector<Obstacle*> obstacles;

    ObstacleSpawner() {
        obstacles.push_back(new Obstacle(randomRange(0, 2), -CAR_HEIGHT));
    }

    void tick(Time deltaTime, Player* player) {
        spawnTimer -= deltaTime.asSeconds();

        if (spawnTimer <= 0) {
            spawnTimer = spawnRate;
            if (spawnRate - spawnRate / 10 > minSpawnRate) spawnRate -= spawnRate / 10;
            if (speed + speed / 10 < maxSpeed) speed += speed / 10;
            obstacles.push_back(new Obstacle(randomRange(0, 2), -CAR_HEIGHT));
        }

        for (int i = 0; i < obstacles.size(); i++) {
            Obstacle* currentObstacle = obstacles.at(i);
            currentObstacle->setPosition(currentObstacle->x, currentObstacle->y + speed * deltaTime.asSeconds());

            if (currentObstacle->y > HEIGHT) {
                obstacles.erase(obstacles.begin() + i);
                score++;
            }

            if (player->x == currentObstacle->x && (
                currentObstacle->y > HEIGHT - CAR_HEIGHT && currentObstacle->y < HEIGHT ||
                currentObstacle->y + CAR_HEIGHT > HEIGHT - CAR_HEIGHT && currentObstacle->y + CAR_HEIGHT < HEIGHT)
                ) {
                gameOver = true;
            }
        }
    }

    void render(RenderWindow* window) {
        for (int i = 0; i < obstacles.size(); i++) {
            obstacles.at(i)->render(window);
        }
    }

    void reset() {
        speed = 100;
        spawnRate = 5;
        obstacles.clear();
        obstacles.push_back(new Obstacle(randomRange(0, 2), -CAR_HEIGHT));
    }
};

int main() {
    srand(time(0));

    RenderWindow window(VideoMode(WIDTH, HEIGHT), "Cars");

    Font font;
    font.loadFromFile("fff.ttf");
    

    Text gameOverText;
    gameOverText.setFont(font);
    gameOverText.setString("GameOver");
    gameOverText.setFillColor(Color::Blue);
    centerTextAt(&gameOverText, WIDTH / 2, HEIGHT / 2 - 200);

    Text scoreText;
    scoreText.setFont(font);
    scoreText.setString("0");
    scoreText.setFillColor(Color::Blue);
    centerTextAt(&scoreText, 50, 50);

    Text scoreBoardText;
    scoreBoardText.setFont(font);
    scoreBoardText.setFillColor(Color::Blue);
    scoreBoardText.setString("Scoreboard");
    centerTextAt(&scoreBoardText, WIDTH / 2, HEIGHT / 2 - 150);
    

    Text top1, top2, top3;
    top1.setFont(font);
    top2.setFont(font);
    top3.setFont(font);
    top1.setFillColor(Color::Blue);
    top2.setFillColor(Color::Blue);
    top3.setFillColor(Color::Blue);


    bool failedToConnect = false;

    TcpSocket serverSocket;

    if (serverSocket.connect(HOST, PORT) != Socket::Done) {
        failedToConnect = true;

        scoreBoardText.setString("Scoreboard not available");
        scoreBoardText.setCharacterSize(16);
        centerTextAt(&scoreBoardText, WIDTH / 2, HEIGHT / 2 - 150);
    }

    if (!failedToConnect) {
        Packet packet;
        string recievedTop;

        serverSocket.receive(packet);
        packet >> recievedTop;
        top1.setString(recievedTop);
        centerTextAt(&top1, WIDTH / 2, HEIGHT / 2 - 100);
        packet >> recievedTop;
        top2.setString(recievedTop);
        centerTextAt(&top2, WIDTH / 2, HEIGHT / 2 - 50);
        packet >> recievedTop;
        top3.setString(recievedTop);
        centerTextAt(&top3, WIDTH / 2, HEIGHT / 2);
    }

    Player player;

    Clock deltaClock;

    ObstacleSpawner os;


    while (window.isOpen()) {
        Event event;

        while (window.pollEvent(event)) {
            if (event.type == Event::Closed)
                window.close();

            if (event.type == Event::KeyPressed) {
                if (!gameOver) {
                    if (player.x < 2 && event.key.code == Keyboard::D) {
                        player.setPosition(player.x + 1);
                    } else if (player.x > 0 && event.key.code == Keyboard::A) {
                        player.setPosition(player.x - 1);
                    }
                } else {
                    if (event.key.code == Keyboard::R) {
                        gameOver = false;
                        score = 0;
                        os.reset();
                        didSendScore = false;
                    }
                }
            }
        }

        Time deltaTime = deltaClock.restart();

        if (!gameOver) os.tick(deltaTime, &player);

        scoreText.setString(int_to_string(score));
        window.clear(Color::White);
        
        player.render(&window);
        os.render(&window);
        window.draw(scoreText);

        if (gameOver) {
            if (!failedToConnect && !didSendScore) {
                char name[100];
                gethostname(name, 100);

                Packet packet;

                packet << "game over" << (string) name << int_to_string(score);

                serverSocket.send(packet);

                didSendScore = true;

                packet.clear();

                packet << "get scores";

                serverSocket.send(packet);
                string recievedTop;

                serverSocket.receive(packet);
                packet >> recievedTop;
                top1.setString(recievedTop);
                centerTextAt(&top1, WIDTH / 2, HEIGHT / 2 - 100);
                packet >> recievedTop;
                top2.setString(recievedTop);
                centerTextAt(&top2, WIDTH / 2, HEIGHT / 2 - 50);
                packet >> recievedTop;
                top3.setString(recievedTop);
                centerTextAt(&top3, WIDTH / 2, HEIGHT / 2);
            }

            window.draw(gameOverText);
            window.draw(scoreBoardText);
            window.draw(top1);
            window.draw(top2);
            window.draw(top3);
        }

        window.display();
    }

    return 0;
}