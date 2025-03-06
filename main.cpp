#include <GL/freeglut.h>
#include <vector>
#include <queue>
#include <limits>
#include <cmath>

struct Node {
    float x, y;
    bool obstacle = false;
    bool visited = false;
    float globalDistance;
    Node* parent = nullptr;
    std::vector<Node*> neighbors;

    Node(float x_, float y_) : 
        x(x_), y(y_),
        globalDistance(std::numeric_limits<float>::max()) {}
};

std::vector<Node> nodes;
Node* startNode = nullptr;
Node* endNode = nullptr;
Node* draggedNode = nullptr;
float rotationAngle = 0.0f;
bool autoRotate = true;
bool showDebug = false;

void initializeNodes() {
    const int NUM_NODES = 12;
    const float RADIUS = 0.6f;
    nodes.clear();
    
    // Create nodes in a starlike pattern
    for(int i = 0; i < NUM_NODES; ++i) {
        float angle = i * (2 * 3.14159f / NUM_NODES);
        float varRadius = RADIUS * (0.7f + 0.3f * sin(angle * 3));
        nodes.emplace_back(
            varRadius * cos(angle),
            varRadius * sin(angle)
        );
    }

    // Connect neighbors dynamically
    for(int i = 0; i < NUM_NODES; ++i) {
        for(int j = 1; j <= 3; ++j) {
            if(i % j == 0) {
                nodes[i].neighbors.push_back(&nodes[(i + j) % NUM_NODES]);
            }
        }
    }
}

void solveDijkstra() {
    for(auto& node : nodes) {
        node.globalDistance = std::numeric_limits<float>::max();
        node.visited = false;
        node.parent = nullptr;
    }

    if(!startNode) return;
    
    startNode->globalDistance = 0.0f;
    std::priority_queue<std::pair<float, Node*>> pq;
    pq.push({0.0f, startNode});

    while(!pq.empty()) {
        Node* current = pq.top().second;
        pq.pop();
        
        if(current->visited || current->obstacle) continue;
        current->visited = true;

        for(Node* neighbor : current->neighbors) {
            if(neighbor->obstacle) continue;
            
            float dx = current->x - neighbor->x;
            float dy = current->y - neighbor->y;
            float distance = sqrt(dx*dx + dy*dy);
            
            if(!neighbor->visited && (current->globalDistance + distance < neighbor->globalDistance)) {
                neighbor->parent = current;
                neighbor->globalDistance = current->globalDistance + distance;
                pq.push({-neighbor->globalDistance, neighbor});
            }
        }
    }
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glRotatef(rotationAngle, 0, 0, 1);

    // Draw connections with varying colors
    glBegin(GL_LINES);
    for(const auto& node : nodes) {
        for(const auto& neighbor : node.neighbors) {
            float hue = fmod(rotationAngle/360.0f, 1.0f);
            glColor3f(fabs(sin(hue*3.14159f)), fabs(cos(hue*3.14159f)), 0.5f);
            glVertex2f(node.x, node.y);
            glVertex2f(neighbor->x, neighbor->y);
        }
    }
    glEnd();

    // Draw nodes with dynamic appearance
    for(const auto& node : nodes) {
        if(node.obstacle) {
            glColor3f(0.3f, 0.3f, 0.3f);
            glBegin(GL_POLYGON);
            for(int i = 0; i < 8; ++i) {
                float angle = i * (2 * 3.14159f / 8);
                glVertex2f(node.x + 0.04f * cos(angle), node.y + 0.04f * sin(angle));
            }
            glEnd();
        } else {
            // Pulsing effect for active nodes
            float pulse = 0.02f * sin(glutGet(GLUT_ELAPSED_TIME)*0.005f);
            glColor3f(node.visited ? 0.0f : 1.0f, 0.0f, 0.0f);
            if(&node == startNode) glColor3f(0.0f, 1.0f, 0.0f);
            if(&node == endNode) glColor3f(0.0f, 0.0f, 1.0f);
            
            glBegin(GL_TRIANGLE_FAN);
            for(int i = 0; i < 32; ++i) {
                float angle = i * (2 * 3.14159f / 32);
                glVertex2f(node.x + (0.03f + pulse) * cos(angle), 
                         node.y + (0.03f + pulse) * sin(angle));
            }
            glEnd();
        }
    }

    // Draw rotating path
    if(endNode && endNode->parent) {
        glColor3f(1.0f, 1.0f, 0.0f);
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        Node* current = endNode;
        while(current) {
            glVertex2f(current->x, current->y);
            current = current->parent;
        }
        glEnd();
        glLineWidth(1.0f);
    }

    glutSwapBuffers();
}

void mouse(int button, int state, int x, int y) {
    float mx = (x/400.0f)*2 - 1;
    float my = 1 - (y/400.0f)*2;
    
    // Adjust for rotation
    float cosTheta = cos(-rotationAngle * 3.14159f/180.0f);
    float sinTheta = sin(-rotationAngle * 3.14159f/180.0f);
    float rotX = mx*cosTheta - my*sinTheta;
    float rotY = mx*sinTheta + my*cosTheta;

    if(state == GLUT_DOWN) {
        for(auto& node : nodes) {
            float dx = rotX - node.x;
            float dy = rotY - node.y;
            if(dx*dx + dy*dy < 0.0009f) {
                if(button == GLUT_LEFT_BUTTON) {
                    startNode = &node;
                } else if(button == GLUT_RIGHT_BUTTON) {
                    endNode = &node;
                } else if(button == GLUT_MIDDLE_BUTTON) {
                    node.obstacle = !node.obstacle;
                }
                solveDijkstra();
                glutPostRedisplay();
                break;
            }
        }
    }
}

void mouseMotion(int x, int y) {
    if(draggedNode) {
        float mx = (x/400.0f)*2 - 1;
        float my = 1 - (y/400.0f)*2;
        
        // Adjust for rotation
        float cosTheta = cos(-rotationAngle * 3.14159f/180.0f);
        float sinTheta = sin(-rotationAngle * 3.14159f/180.0f);
        draggedNode->x = mx*cosTheta - my*sinTheta;
        draggedNode->y = mx*sinTheta + my*cosTheta;
        
        solveDijkstra();
        glutPostRedisplay();
    }
}

void keyboard(unsigned char key, int x, int y) {
    switch(key) {
        case ' ':
            autoRotate = !autoRotate;
            break;
        case 'r':
            initializeNodes();
            startNode = endNode = nullptr;
            break;
        case 'd':
            showDebug = !showDebug;
            break;
        case '+':
            rotationAngle += 5.0f;
            break;
        case '-':
            rotationAngle -= 5.0f;
            break;
    }
    glutPostRedisplay();
}

void idle() {
    if(autoRotate) {
        rotationAngle += 0.3f;
        if(rotationAngle >= 360.0f) rotationAngle -= 360.0f;
        glutPostRedisplay();
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(800, 800);
    glutCreateWindow("Interactive Path Visualizer");
    glutSetOption(GLUT_MULTISAMPLE, 8);
    glEnable(GL_MULTISAMPLE);
    
    initializeNodes();
    
    glutDisplayFunc(display);
    glutMouseFunc(mouse);
    glutMotionFunc(mouseMotion);
    glutKeyboardFunc(keyboard);
    glutIdleFunc(idle);
    
    glutMainLoop();
    return 0;
}
