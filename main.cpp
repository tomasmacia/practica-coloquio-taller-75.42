#include <iostream>
#include <fstream>
#include <vector>
#include <deque>
#include <map>
#include <cmath>

#define STEP 0.05

bool areEqualsDouble(double a, double b) {
    return fabs(a - b) < STEP/2;
}

class Position {
public:
    Position(double x, int lane) {
        this->x = x;
        this->lane = lane;
    }

    void setX(double nextX) {
        this->x = nextX;
    }

    double getX() {
        return this->x;
    }

    void incrementX(double increment) {
        this->x += increment;
    }

    void setLane(int nextLane) {
        this->lane = nextLane;
    }

    int getLane() {
        return this->lane;
    }

    bool equals(Position *otherPosition) {
        return areEqualsDouble(this->getX(), otherPosition->getX()) && this->getLane() == otherPosition->getLane();
    }

private:
    double x;
    int lane;
};

class Input {
public:
    Input(double pos, std::string name, int lane) {
        this->positionTriggered = pos;
        this->name = name;
        this->newLane = lane;
    }

    ~Input() {

    }

    double positionTriggered;
    std::string name;
    int newLane;
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
    CollisionManager(EntityManager *entityManager);

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
        delete(this->position);
        position = nullptr;
    };

    void kill() {
        this->alive = false;
    }

    Position *position;
    std::string name;
    std::string type;
    bool alive = true;

    virtual void update(CollisionManager*) = 0;
};

class Movable : public Entity {
public:
    Movable(std::string name, double x, int lane) : Entity(name, x, lane) {
        this->type = "p";
        this->nextAction.action = MOVE;
        this->nextAction.nextLane = 0;
    }

    ~Movable() {
        delete(position);
        position = nullptr;
    }

    void setNextAction(NextAction na) {
        this->nextAction = na;
    }

    void move() {
        if (alive) {
            if (nextAction.action == MOVE) {
                this->position->incrementX(MOVE_STEP);
                traveled += MOVE_STEP;
            } else if (nextAction.action == SWITCH_LANE) {
                switchLane(nextAction.nextLane);
            }
        }
    }

    void switchLane(int nextLane) {
        this->position->setLane(nextLane);
    }

    void update(CollisionManager *collisionManager) override {
        if (alive) {
            move();
            collisionManager->checkCollisions(this);
        }
    }

    double getTraveledDistance() {
        return traveled;
    }

private:
    NextAction nextAction;
    double MOVE_STEP = STEP;
    double traveled = 0.0;
//    std::vector<Input*> inputs;
};

class FixedObject : public Entity {
public:
    FixedObject(std::string name, double x, int lane) : Entity(name, x, lane) {
        this->type = "o";
    }

    ~FixedObject() {
        delete(position);
        position = nullptr;
    }

    void update(CollisionManager *collisionManager) override {
        if (alive) {
            collisionManager->checkCollisions(this);
        }
    }
};



class EntityManager {
public:
    EntityManager() {
        this->collisionManager = new CollisionManager(this);
    }

    ~EntityManager() {
        for (auto *object : objects) {
            delete(object);
            object = nullptr;
        }

        for (auto *movable : movables) {
            delete(movable);
            movable = nullptr;
        }
    }

    void update() {
        for (auto *movable : movables) {
            movable->update(collisionManager);
        }
        for (auto *object : objects) {
            object->update(collisionManager);
        }
    }

    void addObject(FixedObject* fo) {
        objects.push_back(fo);
    }

    void addMovable(Movable* movable) {
        movables.push_back(movable);
    }

    int getCurrentAliveMovables() {
        int alives = 0;
        for (auto *movable : movables) {
            if (movable->alive) {
                alives +=1;
            }
        }

        return alives;
    }

    int getCurrentObjects() {
        int alives = 0;
        for (auto *object : objects) {
            if (object->alive) {
                alives +=1;
            }
        }

        return alives;
    }

    std::vector<FixedObject*> getObjects() {
        return objects;
    }

    std::vector<Movable*> getMovables() {
        return movables;
    }

    void showLeaderboard() {
        std::vector<Movable*> leaderboard = movables;
        sort(leaderboard.begin(), leaderboard.end(), [](Movable *leftMovable, Movable* rightMovable) {
            return leftMovable->getTraveledDistance() < rightMovable->getTraveledDistance();
        });

        for (auto *movable : leaderboard) {
            if (movable->alive) {
                std::cout << movable->name << " traveled for " << movable->getTraveledDistance() << " without crashing!" << std::endl;
            } else {
                std::cout << movable->name << " crashed after traveling for " << movable->getTraveledDistance() << std::endl;
            }
        }
    }

private:
    std::vector<FixedObject*> objects;
    std::vector<Movable*> movables;
    CollisionManager *collisionManager;
};

CollisionManager::CollisionManager(EntityManager *entityManager) {
    this->entityManager = entityManager;
}

void CollisionManager::checkCollisions(Entity *entity){
    if (entity->type == "p") { // collides with objects
        for (auto *object: entityManager->getObjects()) {
            if (object->alive && object->position->equals(entity->position)) {
                object->kill();
                entity->kill();
            }
        }
    } else if (entity->type == "o") { // collides with movables
        for (auto *movable: entityManager->getMovables()) {
            if (movable->alive && movable->position->equals(entity->position)) {
                movable->kill();
                entity->kill();
            }
        }
    }

}

class Controller {
public:
    Controller(std::vector<Input*> inputs) {
        this->inputsPerPlayer = getInputsPerPlayer(inputs);
    }

    void dispatchNextAction(Movable *movable) {
        if (!movable->alive) {
            movable->setNextAction(getMove());
            return;
        }
        auto element = this->inputsPerPlayer.find(movable->name);
        if (element != this->inputsPerPlayer.end()) { // has inputs
            if (!element->second.empty()) {
                Input* possibleNextInput = element->second.front();
                if (areEqualsDouble(possibleNextInput->positionTriggered, movable->getTraveledDistance())) {
                    movable->setNextAction(getSwitchLane(possibleNextInput));
                    element->second.pop_front();
                } else {
                    movable->setNextAction(getMove());
                }
            } else {
                movable->setNextAction(getMove());
            }
        } else {
            movable->setNextAction(getMove());
        }
    }

private:
    std::map<std::string, std::deque<Input*>> inputsPerPlayer;

    std::map<std::string, std::deque<Input*>> getInputsPerPlayer(std::vector<Input*> inputs) {
        std::map<std::string, std::deque<Input*>> inputsMap;
        for (auto *input : inputs) {
            auto element = inputsMap.find(input->name); // check for inputs for this player input
            if ( element == inputsMap.end()) { // no inputs loaded for this player -> put a deque with this input
                std::deque<Input*> inputsForPlayer = {input};
                inputsMap.emplace(std::make_pair(input->name, inputsForPlayer));
            } else {
                element->second.push_back(input); // put input in existent deque
                inputsMap.emplace(std::make_pair(input->name, element->second));
            }
        }

        return inputsMap;
    }

    NextAction getMove() {
        NextAction na{};
        na.action = MOVE;
        na.nextLane = 0;

        return na;
    }

    NextAction getSwitchLane(Input *input) {
        NextAction na{};
        na.action = SWITCH_LANE;
        na.nextLane = input->newLane;

        return na;
    }

};


bool compareInputs(Input *leftInput, Input *rightInput) {
    return leftInput->positionTriggered < rightInput->positionTriggered;
}


int main() {

    std::ifstream file;
    file.open("jugadores_objetos.txt");

    EntityManager entityManager;

    while (!file.eof()) {
        std::string name, position, type, lane;
        getline(file, name, ',');
        if (name.empty()) break;
        getline(file, position, ',');
        getline(file, type, ',');
        getline(file, lane, '\n');

        if (type == "p") {
            entityManager.addMovable(new Movable(name, stod(position), stoi(lane)));
        } else if (type == "o") {
            entityManager.addObject(new FixedObject(name, stod(position), stoi(lane)));
        } else {
            std::cerr << "Type " << type << " not supported." << std::endl;
            std::cerr << "Error initializing " << name << " at position " << position << " in lane " << lane << std::endl;
        }

        std::cout << "Initialized " << name << " at position " << position << " in lane " << lane << std::endl;
    }

    std::ifstream commandsFile;
    commandsFile.open("comandos_inputs.txt");

    std::vector<Input*> inputs;

    while (!commandsFile.eof()) {
        std::string position, name, newLane;
        getline(commandsFile, position, ',');
        if (position.empty()) break;
        getline(commandsFile, name, ',');
        getline(commandsFile, newLane, '\n');

        inputs.push_back(new Input(stod(position), name, stoi(newLane)));
    }

    sort(inputs.begin(), inputs.end(), &compareInputs);

    for (auto *input : inputs) {
        std::cout << "At position " << input->positionTriggered << " " << input->name << " moved to lane " << input->newLane << std::endl;
    }

    Controller controller(inputs);

    while (entityManager.getCurrentAliveMovables() > 0 && entityManager.getCurrentObjects() > 0) {
        for (auto *movable : entityManager.getMovables()) {
            controller.dispatchNextAction(movable);
        }
        entityManager.update();
    }

    entityManager.showLeaderboard();

    return 0;
}