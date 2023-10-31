/**
 * @file main.cpp
 * @author Siddharth Saha (sahasiddharth611@gmail.com)
 * @brief Main function to test the SFML environment
 * @version 1.0
 * @date 2023-10-25
 */

#include <SFML/Graphics.hpp>
#include "utils.h"
#include "rrt.h"

const int WIDTH = 800;
const int HEIGHT = 600;
const int RADIUS = 1;

Point start, stop;
std::vector<Point> nodes;

std::vector<Polygon> obstacles;
std::vector<sf::ConvexShape> polygons;
std::vector<Node> node_list;

sf::CircleShape startingPoint;
sf::CircleShape endingPoint;

std::random_device rd;                                                 // obtain a random number from hardware
std::mt19937 gen(rd());                                                // seed the generator
std::uniform_int_distribution<> distr_w(0, WIDTH), distr_h(0, HEIGHT); // define the range

const int EPS = 20;
const double GOAL_BIAS = 0.1;

enum status
{
    REACHED,
    ADVANCED,
    TRAPPED
};

void init()
{
    start = Point(100, 20);
    stop = Point(500, 20);

    startingPoint.setPosition(start.x, start.y);
    endingPoint.setPosition(stop.x, stop.y);
    startingPoint.setRadius(5*RADIUS);
    endingPoint.setRadius(5*RADIUS);
    startingPoint.setFillColor(sf::Color(255, 0, 255));
    endingPoint.setFillColor(sf::Color(0, 255, 0));

    std::vector<Point> obstacle = {Point(200, 0), Point(250, 0), Point(250, 400), Point(200, 400), Point(200, 0)};
    obstacles.push_back(Polygon(obstacle, obstacle.size()));

    polygons.resize(obstacles.size());
    for (int i = 0; i < obstacles.size(); i++)
    {
        polygons[i].setPointCount(obstacles[i].num_points);
        polygons[i].setFillColor(
            sf::Color(0, 0, 125));
        for (int j = 0; j < obstacles[i].num_points; j++)
        {
            polygons[i].setPoint(j, sf::Vector2f(obstacles[i].points[j].x, obstacles[i].points[j].y));
        }
    }
}

void NEW_CONFIG(const Node &q_rand, const Node &q_near, Node &q_new)
{
    q_new = q_near + (q_rand - q_near) * (EPS / dist(q_rand, q_near));
}

status EXTEND(std::vector<Node> &node_list, Node &q_rand, const Node &q_goal)
{
    Node q_near;
    double nearest_dist = INT_MAX;
    // NEAREST_NEIGHBOR
    for (auto &q : node_list)
    {
        if (nearest_dist > dist(q_rand, q))
        {
            nearest_dist = dist(q_rand, q);
            q_near = q;
        }
    }
    Node q_new;
    if (nearest_dist < EPS)
    {
        q_new = q_near;
    }
    else
    {
        NEW_CONFIG(q_rand, q_near, q_new);
    }

    if (isPointInsideObstacle(obstacles, q_new.pt))
    {
        std::cout << "Inside obstacle" << std::endl;
        return TRAPPED;
    }

    q_new.parent = new Node(q_near.pt.x, q_near.pt.y);
    node_list.push_back(q_new);
    if (dist(q_new, q_goal) < EPS)
    {
        Node q_final = q_goal;
        q_final.parent = new Node(q_new.pt.x, q_new.pt.y);
        node_list.push_back(q_final);
        std::cout << "Reached" << std::endl;
        return REACHED;
    }
    else
    {
        std::cout << node_list.size() << std::endl;
        std::cout << "Advanced" << std::endl;
        return ADVANCED;
    }

    std::cout << "Trapped" << std::endl;
    return TRAPPED;
}

void draw(sf::RenderWindow &window, std::vector<Node> &node_list)
{
    sf::CircleShape nodeCircle;

    // Draw obstacles
    for (auto &poly : polygons)
        window.draw(poly);

    for (int i = 0; i < node_list.size(); i++)
    {
        Node *parent_node = node_list[i].parent;
        nodeCircle.setPosition(node_list[i].pt.x, node_list[i].pt.y);
        nodeCircle.setRadius(RADIUS);
        nodeCircle.setFillColor(sf::Color(220, 220, 0));
        window.draw(nodeCircle);

        if (parent_node != nullptr)
        {
            const std::array<sf::Vertex, 2> line =
                {sf::Vertex(sf::Vector2f(node_list[i].pt.x, node_list[i].pt.y), sf::Color::Red),
                 sf::Vertex(sf::Vector2f(parent_node->pt.x, parent_node->pt.y), sf::Color::Red)};

            window.draw(line.data(), line.size(), sf::Lines, sf::RenderStates::Default);
        }
    }

    window.draw(startingPoint);
    window.draw(endingPoint);
}

status runRRT(sf::RenderWindow &window, std::vector<Node> &node_list)
{
    Node q_init{start}, q_goal{stop};
    Node q_rand;
    q_init.parent = nullptr;
    int K = 1000;

    node_list.push_back(q_init);
    for (int k = 0; k < K; k++)
    {
        // RANDOM_CONFIG
        if (rand() * 1.0 / RAND_MAX < GOAL_BIAS)
            q_rand = q_goal;
        else
            q_rand = Node(distr_w(gen), distr_h(gen));
        status s = EXTEND(node_list, q_rand, q_goal);
        window.clear();
        draw(window, node_list);
        window.display();
        if (s == REACHED)
            return s;
    }
    return TRAPPED;
}

status runRRTIteration(sf::RenderWindow &window, std::vector<Node> &node_list, const Node &q_init, const Node& q_goal)
{
    Node q_rand;
    if (rand() * 1.0 / RAND_MAX < GOAL_BIAS)
        q_rand = q_goal;
    else
        q_rand = Node(distr_w(gen), distr_h(gen));
    status s = EXTEND(node_list, q_rand, q_goal);
    window.clear();
    draw(window, node_list);
    window.display();
    if (s == REACHED)
        return s;
    return TRAPPED;
}

int main()
{
    sf::RenderWindow window{{WIDTH, HEIGHT}, "Sampling Planner"};
    window.setFramerateLimit(1000);

    init();

    Node q_init{start}, q_goal{stop};
    
    q_init.parent = nullptr;
    int K = 1000;

    node_list.push_back(q_init);
    status s = ADVANCED;

    while (window.isOpen())
    {
        if (s != REACHED)
        {
            s = runRRTIteration(window, node_list, q_init, q_goal);
        }
        window.clear();
        draw(window, node_list);
        window.display();
        for (auto event = sf::Event{}; window.pollEvent(event);)
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
                return 0;
                exit(0);
            }
        }
    }
}