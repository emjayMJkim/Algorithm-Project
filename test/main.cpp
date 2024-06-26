#include <SFML/Graphics.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <limits>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <ctime>
#include <chrono>

// Node 클래스 정의
class Node {
public:
    std::string code;  // 노드 코드
    std::string centralNode;  // 중앙 노드 여부 (중앙 노드일 경우 "O")
    double latitude;  // 위도
    double longitude;  // 경도
    std::vector<std::pair<std::string, double>> nearNodes;  // 인접 노드 목록 및 가중치

    Node(std::string c, std::string cn, double lat, double lon,
        std::vector<std::pair<std::string, double>> nn)
        : code(c), centralNode(cn), latitude(lat), longitude(lon), nearNodes(nn) {}

    //csv 파일을 제대로 읽는지 테스트
    void display() const {
        std::cout << "Code: " << code << "\n";
        std::cout << "Central Node: " << centralNode << "\n";
        std::cout << "Latitude: " << latitude << "\n";
        std::cout << "Longitude: " << longitude << "\n";
        std::cout << "Nearby Nodes:\n";
        for (const auto& node : nearNodes) {
            std::cout << "  Node: " << node.first << ", Weight: " << node.second << "\n";
        }
        std::cout << "--------------------\n";
    }
};

// 문자열을 구분자(delimiter)로 분할하는 함수
std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// CSV 파일을 읽어 Node 객체들의 벡터를 반환하는 함수
std::vector<Node> readCSV(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;

    std::vector<Node> nodes;

    // 헤더 스킵
    std::getline(file, line);

    // 데이터 읽기
    while (std::getline(file, line)) {
        auto tokens = split(line, ',');

        std::string code = tokens[0];
        std::string centralNode = tokens[1];
        double latitude = std::stod(tokens[2]);
        double longitude = std::stod(tokens[3]);

        std::vector<std::pair<std::string, double>> nearNodes;
        for (size_t i = 4; i < tokens.size(); i += 2) {
            if (tokens[i].empty()) break;
            std::string nearNode = tokens[i];
            double weight = std::stod(tokens[i + 1]);
            nearNodes.push_back({ nearNode, weight });
        }

        nodes.emplace_back(code, centralNode, latitude, longitude, nearNodes);
    }

    return nodes;
}

// 노드의 좌표를 정규화하는 함수
void normalizeNodes(std::vector<Node>& nodes) {
    double minLat = std::numeric_limits<double>::max();
    double maxLat = std::numeric_limits<double>::lowest();
    double minLon = std::numeric_limits<double>::max();
    double maxLon = std::numeric_limits<double>::lowest();

    for (const auto& node : nodes) {
        if (node.latitude < minLat) minLat = node.latitude;
        if (node.latitude > maxLat) maxLat = node.latitude;
        if (node.longitude < minLon) minLon = node.longitude;
        if (node.longitude > maxLon) maxLon = node.longitude;
    }

    for (auto& node : nodes) {
        node.latitude = (node.latitude - minLat) / (maxLat - minLat);
        node.longitude = (node.longitude - minLon) / (maxLon - minLon);
    }
}

// 다익스트라 알고리즘을 사용하여 최단 경로를 찾는 함수
std::vector<std::string> dijkstra(const std::vector<Node>& nodes, const std::string& startCode, const std::string& exitCode, const std::unordered_set<std::string>& fireNodes) {
    std::unordered_map<std::string, double> distances;
    std::unordered_map<std::string, std::string> previous;
    auto cmp = [&distances](const std::string& left, const std::string& right) { return distances[left] > distances[right]; };
    //우선순위 큐 선언
    std::priority_queue<std::string, std::vector<std::string>, decltype(cmp)> queue(cmp);

    //모든 노드와의 거리를 무한으로 초기화
    for (const auto& node : nodes) {
        distances[node.code] = std::numeric_limits<double>::infinity();
        previous[node.code] = "";
    }
    //시작노드의 거리를 0으로 설정 후 우선순위 큐에 삽입
    distances[startCode] = 0;
    queue.push(startCode);

    while (!queue.empty()) {
        std::string current = queue.top();
        queue.pop();

        if (current == exitCode) break;

        const Node* currentNode = nullptr;
        for (const auto& node : nodes) {
            if (node.code == current) {
                currentNode = &node;
                break;
            }
        }
        if (!currentNode) continue;

        for (const auto& neighbor : currentNode->nearNodes) {
            if (fireNodes.find(neighbor.first) != fireNodes.end()) {
                continue; // 화재가 발생한 노드로 이동하지 않음
            }
            double alt = distances[current] + neighbor.second;
            if (alt < distances[neighbor.first]) {
                distances[neighbor.first] = alt;
                previous[neighbor.first] = current;
                queue.push(neighbor.first);
            }
        }
    }

    //목표 노드부터 시작 노드까지 previous 맵을 사용하여 경로를 추적한다. 이후, 추적한 경로를 뒤집어서 최단 경로로 반환한다.
    std::vector<std::string> path;
    if (distances[exitCode] == std::numeric_limits<double>::infinity()) {
        return {}; // 경로가 없는 경우 빈 벡터 반환
    }
    for (std::string at = exitCode; !at.empty(); at = previous[at]) {
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());
    return path;
}


// bellman-ford 알고리즘을 사용하여 최단 경로를 찾는 함수
std::vector<std::string> bellmanFord(const std::vector<Node>& nodes, const std::string& startCode, const std::string& exitCode, const std::unordered_set<std::string>& fireNodes) {
    std::unordered_map<std::string, double> distances;
    std::unordered_map<std::string, std::string> previous;

    for (const auto& node : nodes) {
        distances[node.code] = std::numeric_limits<double>::infinity();
        previous[node.code] = "";
    }
    distances[startCode] = 0.0;

    for (size_t i = 0; i < nodes.size() - 1; ++i) {     // (정점 개수 - 1)번 반복  
        for (const auto& node : nodes) {
            if (distances[node.code] == std::numeric_limits<double>::infinity()) continue;
            for (const auto& neighbor : node.nearNodes) {
                if (fireNodes.find(neighbor.first) != fireNodes.end()) continue; // 화재가 발생한 노드로는 이동하지 않음
                // 지금까지의 neighbor까지의 거리보다 현재 node를 거친 neighbor까지의 거리가 더 작을 경우 업데이트 
                if (distances[neighbor.first] > distances[node.code] + neighbor.second) {
                    distances[neighbor.first] = distances[node.code] + neighbor.second;
                    previous[neighbor.first] = node.code;
                }
            }
        }
    }

    std::vector<std::string> path;
    for (std::string at = exitCode; !at.empty(); at = previous[at]) {
        path.push_back(at);
    }
    std::reverse(path.begin(), path.end());

    // 출구로 가는 경로가 없는 경우
    if (path.size() == 1 && path[0] == startCode) {
        path.clear();
    }

    return path;
}

// 플로이드워셜 알고리즘을 사용하여 최단 경로를 찾는 함수
std::vector<std::string> floydWarshall(const std::vector<Node>& nodes, const std::string& startCode, const std::string& exitCode, const std::unordered_set<std::string>& fireNodes) {
    // 노드 코드와 인덱스를 매핑하는 맵 생성
    std::unordered_map<std::string, int> nodeIndex;
    int n = nodes.size();
    for (int i = 0; i < n; ++i) {
        nodeIndex[nodes[i].code] = i;
    }

    // 거리 행렬 초기화
    std::vector<std::vector<double>> dist(n, std::vector<double>(n, std::numeric_limits<double>::infinity()));
    std::vector<std::vector<int>> next(n, std::vector<int>(n, -1));

    for (int i = 0; i < n; ++i) {
        dist[i][i] = 0;
        next[i][i] = i;
    }

    for (const auto& node : nodes) {
        int u = nodeIndex[node.code];
        for (const auto& neighbor : node.nearNodes) {
            if (fireNodes.find(neighbor.first) == fireNodes.end()) { // 화재가 발생한 노드로의 간선 제외
                int v = nodeIndex[neighbor.first];
                dist[u][v] = neighbor.second;
                next[u][v] = v;
            }
        }
    }

    // 플로이드-워셜 알고리즘 수행
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    next[i][j] = next[i][k];
                }
            }
        }
    }

    // 경로 재구성
    std::vector<std::string> path;
    int u = nodeIndex[startCode];
    int v = nodeIndex[exitCode];

    if (next[u][v] == -1) {
        // 경로가 존재하지 않음
        return path;
    }

    while (u != v) {
        path.push_back(nodes[u].code);
        u = next[u][v];
    }
    path.push_back(nodes[v].code);

    return path;
}

// A* 알고리즘에 사용할 휴리스틱 함
// 두 노드 간의 유클리드 거리를 계산하여 반환
double heuristic(const Node& a, const Node& b) {
    return std::sqrt((a.latitude - b.latitude) * (a.latitude - b.latitude) +
        (a.longitude - b.longitude) * (a.longitude - b.longitude));
}

//A* 알고리즘을 이용하여 최단 경로를 찾는 함수
std::vector<std::string> astar(const std::vector<Node>& nodes, const std::string& startCode, const std::string& exitCode, const std::unordered_set<std::string>& fireNodes) {
    std::unordered_map<std::string, double> gScore, /*시작 노드에서 특정 노드까지의 실제 비용*/ fScore; // 시작 노드에서 목표 노드까지의 예상 비용 (gScore + 휴리스틱)
    std::unordered_map<std::string, std::string> cameFrom; // 각 노드의 이전 노드를 저장하여 경로를 재구성
    // 우선순위 큐를 사용하여 fScore가 낮은 노드를 우선 탐색
    auto cmp = [&fScore](const std::string& left, const std::string& right) { return fScore[left] > fScore[right]; };
    std::priority_queue<std::string, std::vector<std::string>, decltype(cmp)> openSet(cmp);

    //초기화: 모든 노드의 gScore와 fScore를 무한대로 설정
    for (const auto& node : nodes) {
        gScore[node.code] = std::numeric_limits<double>::infinity();
        fScore[node.code] = std::numeric_limits<double>::infinity();
    }
    gScore[startCode] = 0.0; // 시작 노드의 gScore는 0
    fScore[startCode] = heuristic(nodes[0], nodes[0]); // 초기 휴리스틱 값 설정 (자기 자신과의 거리이므로 0)

    openSet.push(startCode); // 시작 노드를 우선순위 큐에 추가

    while (!openSet.empty()) {
        std::string current = openSet.top(); // fScore가 가장 낮은 노드를 선택
        openSet.pop();

        if (current == exitCode) { // 목표 노드에 도달한 경우 경로를 재구성하여 반환
            std::vector<std::string> path;
            for (std::string at = exitCode; !at.empty(); at = cameFrom[at]) {
                path.push_back(at);
            }
            std::reverse(path.begin(), path.end()); // 경로를 역순으로 저장했으므로 순서를 반대로 변경
            return path;
        }

        const Node* currentNode = nullptr; // 현재 노드를 찾음
        for (const auto& node : nodes) {
            if (node.code == current) {
                currentNode = &node;
                break;
            }
        }
        if (!currentNode) continue;

        //현재 노드의 모든 인접 노드를 탐색
        for (const auto& neighbor : currentNode->nearNodes) {
            if (fireNodes.find(neighbor.first) != fireNodes.end()) {
                continue; //화재가 발생한 노드는 탐색하지 않음
            }
            double tentative_gScore = gScore[current] + neighbor.second; // 새로운 gScore 계산
            if (tentative_gScore < gScore[neighbor.first]) { // 더 낮은 gScore를 발견한 경우 갱신
                cameFrom[neighbor.first] = current; // 경로를 재구성하기 위해 이전 노드 저장
                gScore[neighbor.first] = tentative_gScore;

                const Node* neighborNode = nullptr;
                //인접 노드를 찾음
                for (const auto& node : nodes) {
                    if (node.code == neighbor.first) {
                        neighborNode = &node;
                        break;
                    }
                }
                if (neighborNode) { // fScore 갱신: gScore + 휴리스틱
                    fScore[neighbor.first] = gScore[neighbor.first] + heuristic(*neighborNode, *currentNode);
                    openSet.push(neighbor.first); // 인접 노드를 우선순위 큐에 추가ㅏ
                }
            }
        }
    }

    return {}; //목표 노드에 도달할 수 없는 경우 빈 벡터 반환
}

// 가중치를 정규화하는 함수(최소 minTravelTime, 최대 maxTravelTime)
double normalizeWeight(double weight, double minWeight, double maxWeight, const double minTravelTime, const double maxTravelTime) {
    return minTravelTime + (maxTravelTime - minTravelTime) * (weight - minWeight) / (maxWeight - minWeight);
}

// 화재 애니메이션을 위한 구조체
struct Fire {
    sf::CircleShape shape;
    std::string startNode;
    std::string endNode;
    double interpolation;
    Fire(const sf::CircleShape& shape, const std::string& startNode, const std::string& endNode)
        : shape(shape), startNode(startNode), endNode(endNode), interpolation(0.0) {}
};

// 경로 간선 색상을 리셋하는 함수
void resetPathEdgesColors(const std::vector<std::string>& path, std::unordered_map<std::string, sf::CircleShape>& nodeMap, std::vector<sf::VertexArray>& pathEdgesShapes, const sf::Color& color) {
    for (size_t i = 1; i < path.size(); ++i) {
        const auto& startNode = nodeMap[path[i - 1]];
        const auto& endNode = nodeMap[path[i]];
        for (auto& edge : pathEdgesShapes) {
            if ((edge[0].position == startNode.getPosition() && edge[1].position == endNode.getPosition()) ||
                (edge[1].position == startNode.getPosition() && edge[0].position == endNode.getPosition())) {
                edge[0].color = color;
                edge[1].color = color;
            }
        }
    }
}

// 두 노드 사이의 가중치를 가져오는 함수
double getWeight(const std::string& from, const std::string& to, const std::vector<Node>& nodes) {
    for (const auto& node : nodes) {
        if (node.code == from) {
            for (const auto& neighbor : node.nearNodes) {
                if (neighbor.first == to) {
                    return neighbor.second;
                }
            }
        }
    }
    return 1.0; // 기본 가중치
}

int main() {
    sf::RenderWindow window(sf::VideoMode(780, 580), "SFML Nodes Visualization");

    // 게임 시작 시간 기록
    auto gameStartTime = std::chrono::high_resolution_clock::now();

    // CSV 파일 경로 설정
    std::string csvFilePath = "nodes.csv";

    // CSV 파일 읽기 및 정규화
    std::vector<Node> nodes = readCSV(csvFilePath);
    if (nodes.empty()) {
        std::cerr << "Error: No nodes were loaded from the CSV file." << std::endl;
        return 1; // 오류 코드 반환
    }

    normalizeNodes(nodes);

    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // 플레이어와 출구의 임의 위치 설정
    /*std::string playerNodeCode = nodes[std::rand() % nodes.size()].code;
    std::string exitNodeCode = nodes[std::rand() % nodes.size()].code;*/

    // 플레이어와 출구의 위치를 고정
    auto playerNodeIt = std::min_element(nodes.begin(), nodes.end(), [](const Node& a, const Node& b) {
        return a.longitude < b.longitude;
    });
    auto exitNodeIt = std::max_element(nodes.begin(), nodes.end(), [](const Node& a, const Node& b) {
        return a.longitude < b.longitude;
    });
    std::string playerNodeCode = playerNodeIt->code;
    std::string exitNodeCode = exitNodeIt->code;

    // 화재 발생 초기화
    std::unordered_set<std::string> fireNodes;
    fireNodes.insert(nodes[std::rand() % nodes.size()].code);

    std::vector<std::string> path;
    switch (4) {
    case 1:
        // 다이익스트라 알고리즘으로 경로 찾기
        path = dijkstra(nodes, playerNodeCode, exitNodeCode, fireNodes);
        break;
    case 2:
        // bellman-Ford 알고리즘으로 경로 찾기
        path = bellmanFord(nodes, playerNodeCode, exitNodeCode, fireNodes);
        break;
    case 3:
        // 플로이드 워셜 알고리즘으로 경로 찾기
        path = floydWarshall(nodes, playerNodeCode, exitNodeCode, fireNodes);
        break;
    case 4:
        // A* 알고리즘으로 경로 찾기
        path = astar(nodes, playerNodeCode, exitNodeCode, fireNodes);
        break;
    }

    // 경로가 없는 경우 처리
    if (path.empty()) {
        std::cout << "Initial path is blocked by fire. Exiting game." << std::endl;
        return 1;
    }

    // 최소 및 최대 가중치 찾기
    double minWeight = std::numeric_limits<double>::max();
    double maxWeight = std::numeric_limits<double>::lowest();
    for (const auto& node : nodes) {
        for (const auto& neighbor : node.nearNodes) {
            if (neighbor.second < minWeight) minWeight = neighbor.second;
            if (neighbor.second > maxWeight) maxWeight = neighbor.second;
        }
    }

    // 노드와 간선 시각화 준비
    std::vector<sf::CircleShape> nodeShapes;
    std::vector<sf::VertexArray> edgesShapes;
    std::unordered_map<std::string, sf::CircleShape> nodeMap;
    std::vector<sf::VertexArray> pathEdgesShapes;

    for (const auto& node : nodes) {
        sf::CircleShape shape(5);
        if (node.centralNode == "O") {
            shape.setFillColor(sf::Color::Green); // centralNode 값이 "O"일 때 초록색으로 설정
        }
        else {
            shape.setFillColor(sf::Color::Yellow); // 그 외에는 노란색으로 설정
        }
        shape.setPosition(node.longitude * 760 + 10, (1.0 - node.latitude) * 560 + 10); // Y축 반전 및 여백 추가
        if (node.code == playerNodeCode) {
            shape.setFillColor(sf::Color::Red); // 플레이어 위치는 빨간색으로 설정
        }
        if (node.code == exitNodeCode) {
            shape.setFillColor(sf::Color::Blue); // 출구 위치는 파란색으로 설정
        }
        if (fireNodes.find(node.code) != fireNodes.end()) {
            shape.setFillColor(sf::Color::Magenta); // 화재 발생 노드는 자홍색으로 설정
        }
        nodeShapes.push_back(shape);
        nodeMap[node.code] = shape;

        for (const auto& edge : node.nearNodes) { 
            auto it = std::find_if(nodes.begin(), nodes.end(), [&edge](const Node& n) {
                return n.code == edge.first;
            });
            if (it == nodes.end()) {
                std::cerr << "Error: Edge points to a non-existing node with ID " << edge.first << std::endl;
                continue;
            }

            sf::VertexArray line(sf::Lines, 2);
            line[0].position = sf::Vector2f(node.longitude * 760 + 10, (1.0 - node.latitude) * 560 + 10);
            line[0].color = sf::Color::White;

            const auto& neighbor = *it;
            line[1].position = sf::Vector2f(neighbor.longitude * 760 + 10, (1.0 - neighbor.latitude) * 560 + 10);
            line[1].color = sf::Color::White;

            edgesShapes.push_back(line);
        }
    }

    //최적 경로 간선 시각화 준비
    for (size_t i = 1; i < path.size(); ++i) {
        sf::VertexArray line(sf::Lines, 2);
        const auto& startNode = nodeMap[path[i - 1]];
        const auto& endNode = nodeMap[path[i]];

        line[0].position = startNode.getPosition();
        line[0].color = sf::Color::Red;
        line[1].position = endNode.getPosition();
        line[1].color = sf::Color::Red;

        pathEdgesShapes.push_back(line);
    }

    // 경로를 따라 이동하는 플레이어의 초기 위치 설정
    sf::CircleShape playerShape(5);
    playerShape.setFillColor(sf::Color::Red);
    playerShape.setPosition(nodeMap[path[0]].getPosition());

    sf::Clock clock;
    sf::Clock fireClock;
    size_t currentPathIndex = 0;
    double interpolation = 0.0;
    const double minTravelTime = 0.3; // 간선 이동의 최소 시간
    const double maxTravelTime = 3.0; // 간선 이동의 최대 시간

    double totalWeight = 0.0; // 지나온 가중치의 합 계산

    std::vector<Fire> fireAnimations;
    std::unordered_set<std::string> passedNodes;

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // 화재가 퍼지는 과정 처리(현재, 사용자가 최대 가중치 경로를 이동할 때의 시간마다 확산된다)
        if (fireClock.getElapsedTime().asSeconds() > 1.0 * maxTravelTime) {
            fireClock.restart();
            std::unordered_set<std::string> newFireNodes;

            //모든 화재 노드에 대해서,
            for (const auto& fireNode : fireNodes) {
                //화재 노드 주위의 모든 노드에 대해 확산
                for (const auto& node : nodes) {
                    if (node.code == fireNode) {
                        for (const auto& neighbor : node.nearNodes) {
                            if (fireNodes.find(neighbor.first) == fireNodes.end()) {
                                newFireNodes.insert(neighbor.first);

                                sf::CircleShape fireShape(5);
                                fireShape.setFillColor(sf::Color::Magenta);
                                fireShape.setPosition(nodeMap[fireNode].getPosition());
                                fireAnimations.emplace_back(fireShape, fireNode, neighbor.first);
                            }
                        }
                        break;
                    }
                }
            }
            //확산된 화재 노드를 저장
            for (const auto& newFireNode : newFireNodes) {
                fireNodes.insert(newFireNode);
            }

            // 화재가 현재 경로를 차단하는지 확인
            bool pathBlocked = false;
            for (const auto& node : path) {
                if (fireNodes.find(node) != fireNodes.end() && passedNodes.find(node) == passedNodes.end()) {
                    pathBlocked = true;
                    break;
                }
            }

            // 경로 재계산
            if (pathBlocked) {
                std::vector<std::string> newPath = dijkstra(nodes, path[currentPathIndex], exitNodeCode, fireNodes);
                if (newPath.empty()) {
                    std::cout << "Game Over: All paths to the exit are blocked by fire." << std::endl;
                    window.close();
                }
                //경로 색상 초기화 후, 현재 경로에서부터 목적지까지 최적 경로를 다시 red 색깔로 칠한다
                else {
                    resetPathEdgesColors(path, nodeMap, pathEdgesShapes, sf::Color::White);
                    path = newPath;
                    pathEdgesShapes.clear();
                    for (size_t i = 1; i < path.size(); ++i) {
                        sf::VertexArray line(sf::Lines, 2);
                        const auto& startNode = nodeMap[path[i - 1]];
                        const auto& endNode = nodeMap[path[i]];

                        line[0].position = startNode.getPosition();
                        line[0].color = sf::Color::Red;
                        line[1].position = endNode.getPosition();
                        line[1].color = sf::Color::Red;

                        pathEdgesShapes.push_back(line);
                    }
                    currentPathIndex = 0;
                    interpolation = 0.0;
                }
            }
        }

        // 이동 시간 계산
        sf::Time elapsed = clock.restart(); // 이전 프레임의 시간 간격을 측정하고, clock을 다시 시작
        double travelTime = getWeight(path[currentPathIndex], path[currentPathIndex + 1], nodes); // 현재 노드에서 다음 노드까지의 가중치를 계산
        double normalizedTime = normalizeWeight(travelTime, minWeight, maxWeight, minTravelTime, maxTravelTime); // 가중치를 정규화하여 이동 시간을 계산
        interpolation += elapsed.asSeconds() / normalizedTime; // 경과된 시간을 정규화된 이동 시간으로 나누어 보간 비율을 계산

        if (interpolation >= 1.0) { // 보간 비율이 1.0 이상이면 다음 노드로 이동 완료된 것
            interpolation = 0.0; // 보간 비율을 초기화
            totalWeight += travelTime; // 이동 완료된 경로의 가중치를 합산
            passedNodes.insert(path[currentPathIndex]); // 지나간 노드를 추가하여 기록
            currentPathIndex++; // 다음 노드로 인덱스를 증가
            if (currentPathIndex + 1 >= path.size()) { // 경로의 끝에 도달한 경우
                playerShape.setPosition(nodeMap[exitNodeCode].getPosition()); // 플레이어의 위치를 출구 노드로 설정
                std::cout << "Player reached the exit!" << std::endl;

                // 경과 시간 계산
                auto gameEndTime = std::chrono::high_resolution_clock::now(); // 게임 종료 시간을 기록
                std::chrono::duration<double> gameDuration = gameEndTime - gameStartTime; // 게임 시작 시간부터 종료 시간까지의 경과 시간을 계산

                std::cout << "Total time taken: " << gameDuration.count() << " seconds" << std::endl;
                std::cout << "Total weight of the path: " << totalWeight << std::endl;

                window.close(); // 게임 창을 닫습니다.
            }
        }
        else { // 아직 다음 노드에 도달하지 않은 경우
            sf::Vector2f startPos = nodeMap[path[currentPathIndex]].getPosition(); // 현재 노드의 위치를 가져온다
            sf::Vector2f endPos = nodeMap[path[currentPathIndex + 1]].getPosition(); // 다음 노드의 위치를 가져온다
            sf::Vector2f delta = endPos - startPos; // 현재 노드와 다음 노드 간의 위치 차이를 계산

            // 각 요소별 곱셈을 수동으로 계산하여 보간된 위치를 설정
            sf::Vector2f interpolatedPos(
                startPos.x + interpolation * delta.x, // X 좌표 보간 계산
                startPos.y + interpolation * delta.y // Y 좌표 보간 계산
            );

            playerShape.setPosition(interpolatedPos); // 플레이어의 위치를 보간된 위치로 설정
        }


        // 화재 애니메이션 업데이트
        for (auto& fire : fireAnimations) {
            double fireTravelTime = getWeight(fire.startNode, fire.endNode, nodes) * 1.0; // 화재가 시작 노드에서 끝 노드로 이동하는 데 걸리는 시간을 가중치로 계산
            double fireNormalizedTime = normalizeWeight(fireTravelTime, minWeight, maxWeight, minTravelTime, maxTravelTime); // 가중치를 정규화하여 이동 시간을 계산
            fire.interpolation += elapsed.asSeconds() / fireNormalizedTime; // 경과된 시간을 정규화된 이동 시간으로 나누어 보간 비율을 계산

            if (fire.interpolation >= 1.0) { // 보간 비율이 1.0 이상이면 화재가 끝 노드에 도달했음을 의미
                fire.interpolation = 1.0; // 보간 비율을 1.0으로 설정하여 끝 노드에 정확히 도달하도록 한다
                nodeMap[fire.endNode].setFillColor(sf::Color::Magenta); // 끝 노드를 자홍색으로 설정하여 화재가 발생했음을 표시
            }

            sf::Vector2f fireStartPos = nodeMap[fire.startNode].getPosition(); // 시작 노드의 위치를 가져온다
            sf::Vector2f fireEndPos = nodeMap[fire.endNode].getPosition(); // 끝 노드의 위치를 가져온다
            sf::Vector2f fireDelta = fireEndPos - fireStartPos; // 시작 노드와 끝 노드 간의 위치 차이를 계산

            // 각 요소별 곱셈을 수동으로 계산하여 보간된 위치를 설정.
            sf::Vector2f fireInterpolatedPos(
                fireStartPos.x + fire.interpolation * fireDelta.x, // X 좌표 보간 계산
                fireStartPos.y + fire.interpolation * fireDelta.y // Y 좌표 보간 계산
            );

            fire.shape.setPosition(fireInterpolatedPos); // 화재 애니메이션의 위치를 보간된 위치로 설정.
        }


        window.clear();

        // 모든 간선 그리기
        for (const auto& edge : edgesShapes) {
            window.draw(edge);
        }

        // 경로 간선 그리기
        for (const auto& edge : pathEdgesShapes) {
            window.draw(edge);
        }

        // 모든 노드 그리기
        for (const auto& shape : nodeShapes) {
            window.draw(shape);
        }

        // 플레이어 그리기
        window.draw(playerShape);

        // 화재 애니메이션 그리기
        for (const auto& fire : fireAnimations) {
            window.draw(fire.shape);
        }

        window.display();
    }

    return 0;
}