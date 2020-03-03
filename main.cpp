#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <map>
#include <cmath>
#include <algorithm>
#include <zconf.h>


double ONE_UNIT = 0.01;

bool areEqualsDouble(double a, double b) {
    return fabs(a - b) < (ONE_UNIT*ONE_UNIT); // avoid doubleing point miscalculations
}



class Position {
public:
    Position(double x, int lane) {
        this->x = x;
        this->lane = lane;
    }

    double getX() {
        return this->x;
    }

    int getLane() {
        return this->lane;
    }

    void setX(double newX) {
        this->x = newX;
    }

    void setLane(int newLane) {
        this->lane = newLane;
    }

    void incrementX(double amount) {
        this->x += amount;
    }

    bool areInSameLane(Position *otherPosition) {
        return this->lane == otherPosition->lane;
    }

private:
    double x;
    int lane;
};

class Input {
public:
    Input(double time, std::string name, int lane) {
        this->time = time;
        this->name = name;
        this->lane = lane;
    }

    double time;
    std::string name;
    int lane;
};

enum Action {
    MOVE,
    SWITCH_LANE
};

struct NextAction {
    Action action;
    int nextLane;
};

class Entity;
class EntityManager;

class CollisionManager {
public:
    CollisionManager(EntityManager *entityManager) {
        this->entityManager = entityManager;
    }

    void checkCollisions(Entity *entity);

private:
    EntityManager *entityManager;
};

class Entity {
public:
    Entity(std::string name, double x, int lane) {
        this->position = new Position(x, lane);
        this->name = name;
    }

    ~Entity() {
        delete(position);
        position = nullptr;
    }

    void kill() {
        this->alive = false;
    }

    virtual void update(CollisionManager *collisionManager) = 0;

    Position *position = nullptr;
    std::string name;
    std::string type;
    bool alive = true;
};

class Player : public Entity {
public:
    Player(std::string name, double x, int lane) : Entity(name, x, lane) {
        this->type = "p";
        this->nextAction.action = MOVE;
        this->nextAction.nextLane = 0;
    }

    void setNextAction(NextAction na) {
        this->nextAction = na;
    }

    void move() {
        if (alive) {
            if (nextAction.action == MOVE) {
                this->position->incrementX(STEP);
                traveled += STEP;
            } else if (nextAction.action == SWITCH_LANE) {
                this->position->incrementX(STEP);
                traveled += STEP;
                switchLane(nextAction.nextLane);
            }
        }
    }

    void switchLane(int nextLane) {
        std::cout << "Cambio de carril " << this->name << " a " << nextLane << std::endl;
        this->position->setLane(nextLane);
    }

    void update(CollisionManager *collisionManager) override {
        if (alive) {
            move();
            collisionManager->checkCollisions(this);
        }
    }

    double getDistanceTraveled() {
        return traveled;
    }

private:
    NextAction nextAction;
    double STEP = ONE_UNIT;
    double traveled;
};

class FixedObject : public Entity {
public:
    FixedObject(std::string name, double x, int lane) : Entity(name, x, lane) {
        this->type = "o";
    }

    void update(CollisionManager *collisionManager) override {
        if (alive) {
            collisionManager->checkCollisions(this);
        }
    }

    bool collidesWithEntity(Entity *entity) {
        bool collides = false;

        if (this->position->areInSameLane(entity->position)) { // if they are in the same lane, check for collisions
            // TODO: divide unit time to avoid doubleing point miscalculations
            collides = entity->position->getX() >= this->position->getX() && entity->position->getX() <= (this->position->getX() + DEPTH);
        }

        return collides;
    }

private:
    double DEPTH = 1;
};


class EntityManager {
public:
    EntityManager() {
        this->collisionManager = new CollisionManager(this);
    }

    ~EntityManager() {
        delete(collisionManager);
        collisionManager = nullptr;
    }

    void addPlayer(Player *player) {
        players.push_back(player);
    }

    void addObject(FixedObject *object) {
        objects.push_back(object);
    }

    void update() {
        for (auto *player : players) {
            player->update(collisionManager);
        }

        for (auto *fixedObject : objects) {
            fixedObject->update(collisionManager);
        }
    }

    std::vector<Player*> getPlayers() {
        return players;
    }

    std::vector<FixedObject*> getObjects() {
        return objects;
    };

    bool objectsAheadOfPlayers() {
        FixedObject* rightmostObject = nullptr;

        for (auto *object : objects) {
            if (rightmostObject == nullptr && object->alive) {
                rightmostObject = object;
                break;
            }
            if (object->alive && object->position->getX() > rightmostObject->position->getX()) {
                rightmostObject = object;
            }
        }

        Player* leftmostPlayer = nullptr;

        for (auto *player : players) {
            if (leftmostPlayer == nullptr && player->alive) {
                leftmostPlayer = player;
                break;
            }
            if (player->alive && player->position->getX() < leftmostPlayer->position->getX()) {
                leftmostPlayer = player;
            }
        }

        bool cmp = false;

        if (leftmostPlayer != nullptr && rightmostObject != nullptr) {
            cmp = leftmostPlayer->position->getX() < rightmostObject->position->getX();
        }

        return cmp;
    }

    int getAlivePlayers() {
        int alive = 0;
        for (auto *player : players) {
            if (player->alive) {
                alive += 1;
            }
        }

        return alive;
    }

    void printDistanceTraveled() {
        std::vector<Player*> playersCpy = players;

        sort(playersCpy.begin(), playersCpy.end(), [](Player* leftPlayer, Player *rightPlayer) {
            return leftPlayer->getDistanceTraveled() < rightPlayer->getDistanceTraveled();
        });

        std::cout << std::endl << "**** DISTANCE TRAVELED ****" << std::endl;

        for (auto *player : playersCpy) {
            if (!player->alive) {
                std::cout << "Player " << player->name << " crashed after traveling for " << player->getDistanceTraveled() << std::endl;
            }
        }
    }

private:
    std::vector<Player*> players;
    std::vector<FixedObject*> objects;

    CollisionManager *collisionManager = nullptr;
};

void CollisionManager::checkCollisions(Entity *entity) {
    if (entity->alive) {
        if (entity->type == "p") {
            for (auto *object : this->entityManager->getObjects()) {
                if (object->collidesWithEntity(entity)) {
                    std::cout << "Player " << entity->name << " crashed into " << object->name << " at " << entity->position->getX() << std::endl;
                    entity->kill();
                 }
            }
        } else if (entity->type == "o") {
            // do nothing
            // we could check for collisions, but this should not happen since players update first...
        }
    }
}

class Controller {
public:
    Controller(std::vector<Input*> inputs) {
        this->inputsPerPlayer = getInputsPerPlayer(inputs);
    }

    void processInput(Player *player, double currentTime) {
        if (player->alive) {
            auto element = this->inputsPerPlayer.find(player->name);

            if (element != this->inputsPerPlayer.end()) { // we have a deque with inputs (or not if empty)
                if (!element->second.empty()) {
                    Input *input = element->second.front();
                    if (input != nullptr && areEqualsDouble(input->time, currentTime)) {
                        //std::cout << "Enviando input al jugador " << input->name << " al carril " << input->lane << ". Tiempo input: " << input->time << " | Tiempo loop: " << currentTime << std::endl;
                        element->second.pop_front();
                        player->setNextAction(getSwitchLaneAction(input));
                        return;
                    }
                }
            }
        }

        player->setNextAction(getMoveAction());
    }

    int getInputsLeft() {
        int inputsLeft = 0;
        for (auto element : this->inputsPerPlayer) {
            inputsLeft += element.second.size();
        }

        return inputsLeft;
    }


private:
    std::map<std::string, std::deque<Input*>> inputsPerPlayer;

    std::map<std::string, std::deque<Input*>> getInputsPerPlayer(std::vector<Input*> inputs) {
        std::map<std::string, std::deque<Input*>> inputsMap;

        for (auto *input : inputs) {
            auto element = inputsMap.find(input->name);

            if (element == inputsMap.end()) { // no inputs present for this player name :: we create a new deque and put that in the map
                std::deque<Input*> dequeForPlayer = {input};
                inputsMap.emplace(std::make_pair(input->name, dequeForPlayer));
            } else { // there are inputs preset :: we add the new input to the existing deque and put it back in the map (just in case)
                element->second.push_back(input);
                inputsMap.emplace(std::make_pair(input->name, element->second));
            }
        }


        return inputsMap;
    }

    NextAction getMoveAction() {
        NextAction na{};
        na.action = MOVE;
        na.nextLane = 0; // we dont use this value when moving

        return na;
    }

    NextAction getSwitchLaneAction(Input *input) {
        NextAction na{};
        na.action = SWITCH_LANE;
        na.nextLane = input->lane;

        return na;
    }
};

int main() {
    std::cout << "**** INITIALIZING PLAYERS AND OBJECTS ****" << std::endl;

    std::ifstream players("players.txt");

    EntityManager entityManager;

    while(!players.eof()) {
        std::string name, position, type, lane;
        getline(players, name, ',');
        if (name.empty()) break;
        getline(players, position, ',');
        getline(players, type, ',');
        getline(players, lane);

        if (type == "p") {
            entityManager.addPlayer(new Player(name, stod(position), stoi(lane)));
            std::cout << "Initialized player " << name << " in position " << position << " and lane " << lane << std::endl;
        } else if (type == "o") {
            entityManager.addObject(new FixedObject(name, stod(position), stoi(lane)));
            std::cout << "Initialized object " << name << " in position " << position << " and lane " << lane << std::endl;
        }
    }

    players.close();

    std::cout << std::endl << "Initialized " << entityManager.getAlivePlayers() << " players and " << entityManager.getObjects().size() << " objects" << std::endl;


    std::cout << std::endl << "**** INITIALIZING INPUTS ****" << std::endl;

    std::ifstream inputsFile("inputs.txt");

    std::vector<Input*> inputs;

    while(!inputsFile.eof()) {
        std::string time, name, lane;
        getline(inputsFile, time, ',');
        if (time.empty()) break;
        getline(inputsFile, name, ',');
        getline(inputsFile, lane);

        inputs.push_back(new Input(stod(time), name, stoi(lane)));
    }

    inputsFile.close();

    sort(inputs.begin(), inputs.end(), [](Input *leftInput, Input* rightInput) {
        return leftInput->time < rightInput->time;
    });

    for (auto *input : inputs) {
        std::cout << "Player " << input->name << " moved to lane " << input->lane << " at " << input->time << std::endl;
    }

    std::cout << std::endl << std::endl << std::endl << "Beginning of the game" << std::endl;


    Controller controller(inputs);

    double frames = 0;

    while(entityManager.getAlivePlayers() > 0 && entityManager.objectsAheadOfPlayers()) {
        frames += ONE_UNIT;
        for (auto *player : entityManager.getPlayers()) {
            controller.processInput(player, frames);
        }

        entityManager.update();
    }

    entityManager.printDistanceTraveled();


    return 0;
}