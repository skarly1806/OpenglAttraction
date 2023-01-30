#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <cstddef>
#include <glimac/Cone.hpp>
#include <glimac/Cylindre.hpp>
#include <glimac/FilePath.hpp>
#include <glimac/FreeFlyCamera.hpp>
#include <glimac/Geometry.hpp>
#include <glimac/Image.hpp>
#include <glimac/LoadObject.hpp>
#include <glimac/Program.hpp>
#include <glimac/Sphere.hpp>
#include <glimac/TrackballCamera.hpp>
#include <glimac/common.hpp>
#include <glimac/glm.hpp>
#include <vector>

int window_width  = 1280;
int window_height = 720;
const float r = 0.5f;
const float PI = 3.141593;

#define MAX_TEXTURES 2
#define MAX_LIGHTS 10

static void key_callback(GLFWwindow* /*window*/, int /*key*/, int /*scancode*/, int /*action*/, int /*mods*/)
{
}

static void mouse_button_callback(GLFWwindow* /*window*/, int /*button*/, int /*action*/, int /*mods*/)
{
}

static void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double /*yoffset*/)
{
}

static void cursor_position_callback(GLFWwindow* /*window*/, double /*xpos*/, double /*ypos*/)
{
}

static void size_callback(GLFWwindow* /*window*/, int width, int height)
{
    window_width  = width;
    window_height = height;
}

float randomFloat(float limit)
{
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX / limit);
}


struct Material {
private:
    GLint uMVPMatrix_gl;
    GLint uMVMatrix_gl;
    GLint uNormalMatrix_gl;

    GLint color_gl;
    GLint specularIntensity_gl;
    GLint shininess_gl;
    GLint hasTexture_gl;
    GLint isLamp_gl;

    GLuint* uTextures_gl;

public:
    glm::vec3  color;
    float      specularIntensity;
    float      shininess;
    bool       hasTexture;
    bool       isLamp;

    std::unique_ptr<glimac::Image>* uTextures;

    Material(){}

    Material(GLint prog_GLid)
    {
        color_gl         = glGetUniformLocation(prog_GLid, "uMaterial.color");
        specularIntensity_gl = glGetUniformLocation(prog_GLid, "uMaterial.specularIntensity");
        shininess_gl     = glGetUniformLocation(prog_GLid, "uMaterial.shininess");

        hasTexture_gl = glGetUniformLocation(prog_GLid, "uMaterial.hasTexture");
        isLamp_gl        = glGetUniformLocation(prog_GLid, "uMaterial.isLamp");

        uMVPMatrix_gl   = glGetUniformLocation(prog_GLid, "uMVPMatrix");
        uMVMatrix_gl    = glGetUniformLocation(prog_GLid, "uMVMatrix");
        uNormalMatrix_gl = glGetUniformLocation(prog_GLid, "uNormalMatrix");

        // Textures
        uTextures_gl = (GLuint*)calloc(MAX_TEXTURES, sizeof(GLuint));
        uTextures    = (std::unique_ptr<glimac::Image>*)calloc(MAX_TEXTURES, sizeof(std::unique_ptr<glimac::Image>));

        for (int i = 0; i < MAX_TEXTURES; i++) {
            char varname[50] = "";
            sprintf(varname, "uMaterial.textures[%d]", i);
            uTextures_gl[i] = glGetUniformLocation(prog_GLid, varname);
        }
    }

    void ChargeGLints(){
        glUniform3f(color_gl, color.x, color.y, color.z);
        glUniform1f(specularIntensity_gl, specularIntensity);
        glUniform1f(shininess_gl, shininess);
        glUniform1f(hasTexture_gl, hasTexture);
        glUniform1f(isLamp_gl, isLamp);
        if (hasTexture)
            for (int i = 0; i < MAX_TEXTURES; i++) {
                glGenTextures(1, &(uTextures_gl[i]));
                glBindTexture(GL_TEXTURE_2D, uTextures_gl[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, uTextures[i]->getWidth(), uTextures[i]->getHeight(), 0, GL_RGBA, GL_FLOAT, uTextures[i]->getPixels());
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
    }

    void ChargeMatrices(glm::mat4 materialMVMatrix, glm::mat4 projMatrix)
    {
        glUniformMatrix4fv(uMVMatrix_gl, 1, GL_FALSE, glm::value_ptr(materialMVMatrix));
        glUniformMatrix4fv(uNormalMatrix_gl, 1, GL_FALSE, glm::value_ptr(glm::transpose(glm::inverse(materialMVMatrix))));
        glUniformMatrix4fv(uMVPMatrix_gl, 1, GL_FALSE, glm::value_ptr(projMatrix * materialMVMatrix));
    }
};

struct PointLight {
private:
    GLint position_gl;
    GLint intensity_gl;
    GLint color_gl;

public:
    glm::vec3 position;
    float     intensity;
    glm::vec3 color;

    PointLight() {}

    PointLight(GLint prog_GLid, int i)
    {
        char varname[50] = "";
        sprintf(varname, "uPointLights[%d].position", i);
        position_gl = glGetUniformLocation(prog_GLid, varname);

        sprintf(varname, "uPointLights[%d].intensity", i);
        intensity_gl = glGetUniformLocation(prog_GLid, varname);

        sprintf(varname, "uPointLights[%d].color", i);
        color_gl = glGetUniformLocation(prog_GLid, varname);

    }

    void ChargeGLints(glm::vec3 light_position)
    {
        glUniform3fv(position_gl, 1, glm::value_ptr(light_position));
        glUniform3f(color_gl, color.x, color.y, color.z);
        glUniform1f(intensity_gl, intensity);
    }

    Material* GenerateLampe(GLint prog_GLid)
    {
        Material* my_lampe = new Material(prog_GLid);
        my_lampe->color = color;
        my_lampe->hasTexture = false;
        my_lampe->isLamp = true;
        my_lampe->shininess = 6.f;
        my_lampe->specularIntensity = 1.f;
        return my_lampe;
    }
};

struct DirLight {
private:
    GLint direction_gl;
    GLint intensity_gl;
    GLint color_gl;

public:
    glm::vec3 direction;
    float intensity;
    glm::vec3 color;

    DirLight() {}

    DirLight(GLint prog_GLid, int i)
    {
        std::string basename = "uDirLights[";
        basename.push_back(char(i));
        basename.push_back(']');
        basename.push_back('.');

        std::string directionname = basename + "direction";
        std::string intensityname = basename + "intensity";
        std::string colorname = basename + "color";

        direction_gl = glGetUniformLocation(prog_GLid, directionname.c_str());
        intensity_gl = glGetUniformLocation(prog_GLid, intensityname.c_str());
        color_gl     = glGetUniformLocation(prog_GLid, colorname.c_str());
    }

    void ChargeGLints()
    {
        glUniform3f(direction_gl, direction.x, direction.y, direction.z);
        glUniform3f(color_gl, color.x, color.y, color.z);
        glUniform1f(intensity_gl, intensity);
    }
};

struct Circuit{
public:
    std::vector<glm::vec3> CircuitParts;
    std::vector<glm::vec3> CircuitColors;
    Material*              CircuitMaterial;
    int                    NbCircuitPoints = 0;

    Circuit(GLint prog_GLid)
    {
        CircuitMaterial = new Material(prog_GLid);
    }
};

struct Wagon{
public:
    glimac::Geometry* WagonObject;
    Material*         WagonMaterial;

    bool isActif;
    float timeSinceSwitchingIndex;

    glm::vec3 Position;
    int indexPos;

    float speed;
    float minSpeed;
    float maxSpeed;

    Wagon(GLint prog_GLid)
    {
        WagonObject = new glimac::Geometry();
        bool res    = WagonObject->loadOBJ("./assets/models/Wagon.obj", "./assets/models/Wagon.mtl", false);
        if (!res) {
            printf("ERROR chargement du Wagon! \n");
            exit(-1);
        }
        WagonMaterial = new Material(prog_GLid);
        isActif = false;
        timeSinceSwitchingIndex = 0.f;
        indexPos = 0;

        speed = 0.f;
        minSpeed = 0.5f;
        maxSpeed = 2.f;

        Position = glm::vec3(0, 0, 0);
    }

    void ResetState(){
        //isActif  = false;
        indexPos = 0;
        speed   = 0.f;
    }

};

struct Rectangle {
public:
    std::vector<glimac::ShapeVertex> vertices;
    std::vector<int>        indices;
    unsigned int             numVertices;
    unsigned int              numIndices;

    Material* material;

    Rectangle(GLint prog_GLid, float length, float height, glm::vec3 color = glm::vec3(1, 1, 1))
    {
        glm::vec3 normal = glm::vec3(0, 0, -1);
        vertices.push_back(glimac::ShapeVertex(glm::vec3(-length / 2, -height / 2, 0), normal, glm::vec2(0, 0)));
        vertices.push_back(glimac::ShapeVertex(glm::vec3(length / 2, -height / 2, 0), normal, glm::vec2(1, 0)));
        vertices.push_back(glimac::ShapeVertex(glm::vec3(length / 2, height / 2, 0), normal, glm::vec2(1, 1)));
        vertices.push_back(glimac::ShapeVertex(glm::vec3(-length / 2, height / 2, 0), normal, glm::vec2(0, 1)));

        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(2);
        indices.push_back(0);
        indices.push_back(2);
        indices.push_back(3);

        numVertices = 4;
        numIndices = 6;

        material = new Material(prog_GLid);
        material->color = color;
        material->isLamp = false;
        material->hasTexture = false;
        material->shininess  = 20.f;
        material->specularIntensity  = 1.f;
    }
};

struct GeneralInfos {
private:
    GLint AmbiantLight_gl;
    GLint ViewPos_gl;

    GLint NbLights_gl;

public:
    glm::mat4 globalMVMatrix;
    glm::mat4 projMatrix;

    glm::vec3 ViewPos; // position camera

    glm::vec3 AmbiantLight = glm::vec3(0, 0, 0);

    std::vector<Material*> MoonMaterials;
    int                    NbMoons;

    std::vector<DirLight*> DirLights;
    std::vector<PointLight*> PointLights;
    std::vector<Material*> Lampes;

    glm::vec2 NbLights = glm::vec2(0, 0);

    Circuit* circuit;

    Wagon*   wagon;

    Rectangle*  floor;

    glimac::TrackballCamera* camera;

    GeneralInfos(GLint prog_GLid)
    {
        AmbiantLight_gl = glGetUniformLocation(prog_GLid, "uAmbiantLight");
        ViewPos_gl = glGetUniformLocation(prog_GLid, "uViewPos");

        NbLights_gl = glGetUniformLocation(prog_GLid, "NbLights");

        ViewPos         = glm::vec3(0, 0, 0);
        AmbiantLight    = glm::vec3(0, 0, 0);
        NbMoons         = 0;

        floor    = new Rectangle(prog_GLid, 20.f, 20.f, glm::vec3(0, 1, 0));
        circuit  = new Circuit(prog_GLid);
        wagon = new Wagon(prog_GLid);

        camera = new glimac::TrackballCamera();
    }

    void ChargeGLints()
    {
        glUniform3f(AmbiantLight_gl, AmbiantLight.x, AmbiantLight.y, AmbiantLight.z);
        glUniform3f(ViewPos_gl, ViewPos.x, ViewPos.y, ViewPos.z);
        glUniform2f(NbLights_gl, NbLights.x, NbLights.y);
    }
};

void HandleEvents(GLFWwindow* window, GeneralInfos* generalInfos)
{
    glfwPollEvents();
    double c_xpos, c_ypos;
    glfwGetCursorPos(window, &c_xpos, &c_ypos);

    generalInfos->camera->rotateUp(c_xpos);
    generalInfos->camera->rotateLeft(c_ypos);

    /* KEYBOARD */
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        generalInfos->camera->moveFront(0.2f);
    }

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        generalInfos->camera->moveFront(-0.2f);
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        glfwWaitEventsTimeout(0.3);
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE) {
            generalInfos->wagon->isActif = !generalInfos->wagon->isActif;
            if (!generalInfos->wagon->isActif) {
                generalInfos->wagon->ResetState();
            }
            else {
                generalInfos->wagon->speed                   = generalInfos->wagon->minSpeed;
                generalInfos->wagon->timeSinceSwitchingIndex = (float)glfwGetTime();
            }
            printf("Switched isActif to %d\n", generalInfos->wagon->isActif);
        }
    }
}

void CircuitGeneration(GeneralInfos* generalInfos, GLuint vbo, glimac::Cylindre cylindre)
{
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, cylindre.getVertexCount() * sizeof(glimac::ShapeVertex), cylindre.getDataPointer(), GL_STATIC_DRAW);

    Circuit * circuit = generalInfos->circuit;

    for (int i = 0; i < circuit->NbCircuitPoints; i++) {
        circuit->CircuitMaterial->color = circuit->CircuitColors[i];

        glm::vec3 Pstart = circuit->CircuitParts[i];
        glm::vec3 Pend   = (circuit->NbCircuitPoints - 1 == i) ? circuit->CircuitParts[0] : circuit->CircuitParts[i + 1];

        glm::vec3 direction = glm::normalize(Pend - Pstart);
        glm::vec3 cylinderDirection = glm::vec3(0, 0, 1);

        float angle = glm::radians(180.f); // set a 180 au cas ou parallele et opposés
        glm::vec3 axis = glm::cross(cylinderDirection, direction);
        if (glm::length(axis) > 0.1f)
            angle = glm::asin(glm::length(axis));
        else{ // si les vecteurs sont paralleles
            // si dans le meme sens, pas de changement
            if (glm::dot(glm::normalize(cylinderDirection), glm::normalize(direction)) >= 0.f) angle = 0.f;
            axis = glm::vec3(0, 1, 0); // on prend un axe qulconque a 90° de l'axe du cylindre (0,0,1)
        }

        glm::mat4 circuitMVMatrix = generalInfos->globalMVMatrix;
        circuitMVMatrix           = glm::translate(circuitMVMatrix, Pstart); // move to start
        circuitMVMatrix           = glm::rotate(circuitMVMatrix, angle, axis); // rotate towards end
        circuitMVMatrix           = glm::scale(circuitMVMatrix, glm::vec3(1, 1, glm::length(Pend - Pstart))); // scale to length

        circuit->CircuitMaterial->ChargeMatrices(circuitMVMatrix, generalInfos->projMatrix);
        circuit->CircuitMaterial->ChargeGLints();

        glDrawArrays(GL_TRIANGLES, 0, cylindre.getVertexCount());
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DrawWagon(GeneralInfos* generalInfos, GLuint vbo){
    // préparations
    Wagon* wagon = generalInfos->wagon;
    Circuit* circuit = generalInfos->circuit;
    glm::vec3 Pstart    = circuit->CircuitParts[wagon->indexPos];
    glm::vec3 Pend      = (wagon->indexPos == circuit->NbCircuitPoints - 1) ? circuit->CircuitParts[0] : circuit->CircuitParts[wagon->indexPos + 1];
    glm::vec3 direction = glm::normalize(Pend - Pstart);

    // si wagon non actif, pas de mouvement
    if(wagon->isActif){

        //gestion vitesse
        if(direction.y > 0.f){
            wagon->speed = (wagon->speed > wagon->minSpeed) ? wagon->speed - 0.01f : wagon->minSpeed;
        }
        else if(direction.y < 0.f){
            wagon->speed = (wagon->speed < wagon->maxSpeed) ? wagon->speed + 0.02f : wagon->maxSpeed;
        }

        // gestion position sur le circuit + déplacement
        float my_time = (float)glfwGetTime() - wagon->timeSinceSwitchingIndex;
        float lengthToMove = wagon->speed * my_time;

        wagon->Position = (glm::length(Pstart - Pend) > lengthToMove) ? Pstart + lengthToMove * direction : Pend;

        if(wagon->Position == Pend) // si arrivé a la fin
        {
            wagon->indexPos = (wagon->indexPos == circuit->NbCircuitPoints - 1) ? 0 : wagon->indexPos + 1;
            wagon->timeSinceSwitchingIndex = (float)glfwGetTime();
        }
    }
    else{
        wagon->Position = circuit->CircuitParts[0];
    }

    // gestion direction du wagon
    glm::vec3 wagonDirection = glm::vec3(1, 0, 0);
    float     angle = glm::radians(180.f); // set a 180 au cas ou parallele et opposés
    glm::vec3 axis  = glm::cross(wagonDirection, direction);
    if (glm::length(axis) > 0.1f)
        angle = glm::asin(glm::length(axis));
    else { // si les vecteurs sont paralleles
        // si dans le meme sens, pas de changement
        if (glm::dot(glm::normalize(wagonDirection), glm::normalize(direction)) >= 0.f)
            angle = 0.f;
        axis = glm::vec3(0, 1, 0); // on prend un axe qulconque a 90° de l'axe du wagon
    }

    // matrice de mouvement
    glm::mat4 wagonMVMatrix = generalInfos->globalMVMatrix;
    wagonMVMatrix           = glm::translate(wagonMVMatrix, wagon->Position);
    wagonMVMatrix           = glm::rotate(wagonMVMatrix, angle, axis); // rotate towards end
    wagonMVMatrix           = glm::translate(wagonMVMatrix, glm::vec3(-0.2, 0.09f, -0.1f));
    wagonMVMatrix           = glm::scale(wagonMVMatrix, glm::vec3(0.1f));

    // chargement des infos
    wagon->WagonMaterial->ChargeMatrices(wagonMVMatrix, generalInfos->projMatrix);
    wagon->WagonMaterial->ChargeGLints();

    // dessin
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, wagon->WagonObject->getVertexCount() * sizeof(unsigned int), wagon->WagonObject->getVertexBuffer(), GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, wagon->WagonObject->getIndexCount(), GL_UNSIGNED_INT, wagon->WagonObject->getIndexBuffer());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DrawFloor(GeneralInfos* generalInfos, GLuint vbo){
    glm::mat4 floorMVMatrix = generalInfos->globalMVMatrix;
    floorMVMatrix           = glm::translate(floorMVMatrix, glm::vec3(0, -0.3, 0));
    floorMVMatrix           = glm::rotate(floorMVMatrix, glm::radians(90.f), glm::vec3(1, 0, 0));

    // charge les infos
    generalInfos->floor->material->ChargeMatrices(floorMVMatrix, generalInfos->projMatrix);
    generalInfos->floor->material->ChargeGLints();

    // dessin
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, generalInfos->floor->numVertices * sizeof(glimac::ShapeVertex), &generalInfos->floor->vertices[0], GL_STATIC_DRAW);
    glDrawElements(GL_TRIANGLES, generalInfos->floor->numIndices, GL_UNSIGNED_INT, generalInfos->floor->indices.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

}

int main(int argc, char* argv[])
{

    /* Initialize the library */
    if (!glfwInit()) {
        return -1;
    }

    /* Create a window and its OpenGL context */
#ifdef __APPLE__
    /* We need to explicitly ask for a 3.3 context on Mac */
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Projet", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    /* Make the window's context current */
    glfwMakeContextCurrent(window);

    /* Intialize glad (loads the OpenGL functions) */
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        return -1;
    }

    /* Hook input callbacks */
    glfwSetKeyCallback(window, &key_callback);
    glfwSetMouseButtonCallback(window, &mouse_button_callback);
    glfwSetScrollCallback(window, &scroll_callback);
    glfwSetCursorPosCallback(window, &cursor_position_callback);
    glfwSetWindowSizeCallback(window, &size_callback);

    glimac::FilePath applicationPath(argv[0]);

    glimac::Program program(loadProgram(applicationPath.dirPath() + "Projet/shaders/3D.vs.glsl",
                                        applicationPath.dirPath() + "Projet/shaders/lights.fs.glsl"));
    program.use();

    glEnable(GL_DEPTH_TEST); // Permet d'activer le test de profondeur du GPU

    /* CREATE ALL THINGS */

    // infos générales
    GeneralInfos* generalInfos = new GeneralInfos(program.getGLId());

    // les lumieres
    // set ambiant light infos and charge in shaders
    generalInfos->AmbiantLight = glm::vec3(0.2, 0.2, 0.2);

    // set point light infos
    int numPL = 0;
    generalInfos->PointLights.push_back(new PointLight(program.getGLId(), numPL));
    PointLight* myPointLight        = generalInfos->PointLights[numPL];
    myPointLight->position          = glm::vec3(2, 4, 0);
    myPointLight->intensity         = 2.f;
    myPointLight->color             = glm::vec3(1, 1, 1);

    Material* myLamp = myPointLight->GenerateLampe(program.getGLId());
    generalInfos->Lampes.push_back(myLamp);

    generalInfos->NbLights = glm::vec2(0, 1);

    generalInfos->ChargeGLints();

    // set circuit  infos
    std::vector<glm::vec3> circuit;
    circuit.push_back(glm::vec3(0, 0, 0));
    circuit.push_back(glm::vec3(2, 0, 0));
    circuit.push_back(glm::vec3(2.5, 0.5, 0));
    circuit.push_back(glm::vec3(3.5, 0.5, 0));
    circuit.push_back(glm::vec3(5.5, 2.5, 0));
    circuit.push_back(glm::vec3(5.5, 2.5, 1));
    circuit.push_back(glm::vec3(3.5, 0.5, 1));
    circuit.push_back(glm::vec3(2.5, 0.5, 1));
    circuit.push_back(glm::vec3(1.5, 0, 1));
    circuit.push_back(glm::vec3(0, 0, 1));
    generalInfos->circuit->CircuitParts    = circuit;
    generalInfos->circuit->NbCircuitPoints = 10;

    for (int i = 0; i < generalInfos->circuit->NbCircuitPoints; i++) {
        generalInfos->circuit->CircuitColors.push_back(glm::vec3(randomFloat(1.f), randomFloat(1.f), randomFloat(1.f)));
    }

    Material* circuitMaterial          = generalInfos->circuit->CircuitMaterial;
    circuitMaterial->color             = glm::vec3(1, 0, 0);
    circuitMaterial->specularIntensity = 1.f;
    circuitMaterial->shininess         = 30;
    circuitMaterial->hasTexture        = false;
    circuitMaterial->isLamp            = false;

    // set wagon infos
    Material* wagonMaterial            = generalInfos->wagon->WagonMaterial;
    wagonMaterial->color               = glm::vec3(1, 1, 0);
    wagonMaterial->specularIntensity   = 1.f;
    wagonMaterial->shininess           = 30;
    wagonMaterial->hasTexture          = false;
    wagonMaterial->isLamp              = false;

    /* CALCULATE MATRICES */
    glm::mat4 projMatrix     = glm::perspective(glm::radians(70.f), float(window_width) / float(window_height), 0.1f, 100.f);
    glm::mat4 globalMVMatrix = glm::translate(glm::mat4(), glm::vec3(0, 0, -5));

    generalInfos->projMatrix = projMatrix;
    generalInfos->globalMVMatrix = globalMVMatrix;

    // Création d'une sphère
    glimac::Sphere sphere(1, 64, 32);

    // Création d'un cylindre
    glimac::Cylindre cylindre(1, .03, 20, 20);

    // Création d'un cone
    glimac::Cone cone(1, 0.5, 30, 5);

    /* VBO + VAO */
    // création du VBO
    GLuint vbo;
    glGenBuffers(1, &vbo);

    // Création du VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Binding du VAO
    glBindVertexArray(vao);
    const GLint VERTEX_ATTR_POSITION = 0;
    const GLint VERTEX_ATTR_NORMAL   = 1;
    glEnableVertexAttribArray(VERTEX_ATTR_POSITION);
    glEnableVertexAttribArray(VERTEX_ATTR_NORMAL);

    // Spécification de l'attribut de sommet et de normal
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(VERTEX_ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(glimac::ShapeVertex), (const GLvoid*)offsetof(glimac::ShapeVertex, position));
    glVertexAttribPointer(VERTEX_ATTR_NORMAL, 3, GL_FLOAT, GL_FALSE, sizeof(glimac::ShapeVertex), (const GLvoid*)offsetof(glimac::ShapeVertex, normal));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Débinding du VAO
    glBindVertexArray(0);

    /* GENERATE MOONS */
    generalInfos->NbMoons = 0;
    std::vector<glm::vec3> randomTransform;

    for (int i = 0; i < generalInfos->NbMoons; ++i) {
        Material* new_moon = new Material(program.getGLId());
        randomTransform.push_back(glm::sphericalRand(2.f));
        new_moon->color         = glm::vec3(glm::linearRand(0.1f, 1.f) * .8, glm::linearRand(0.1f, 1.f) * .5, glm::linearRand(0.1f, 1.f) * .2);
        new_moon->specularIntensity  = 0.2f;
        new_moon->shininess  = randomFloat(20.f);
        new_moon->hasTexture = false;
        generalInfos->MoonMaterials.push_back(new_moon);
    }

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* EVENTS */
        HandleEvents(window, generalInfos);

        /* RENDERING */

        glm::mat4 ViewMatrix  = generalInfos->camera->getViewMatrix();
        generalInfos->ViewPos = glm::vec3(ViewMatrix[0][0], ViewMatrix[0][1], ViewMatrix[0][2]);
        generalInfos->ChargeGLints();

        // clear window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // get camera matrix
        generalInfos->globalMVMatrix = ViewMatrix;

        // earth rotates on itself
        //glm::mat4 earthMVMatrix = generalInfos->globalMVMatrix; // glm::rotate(generalInfos->globalMVMatrix, (float)glfwGetTime(), glm::vec3(0, 1, 0)); // Translation * Rotation

        // send Matrixes values to shader
        circuitMaterial->ChargeMatrices(generalInfos->globalMVMatrix, generalInfos->projMatrix);

        glBindVertexArray(vao);

        /* GESTION LUMIERE */

        PointLight* currentLight = generalInfos->PointLights[0];

        glm::vec3 lightPos(currentLight->position.x, currentLight->position.y * (glm::cos((float)glfwGetTime()) * glm::sin((float)glfwGetTime())), currentLight->position.z); // position mouvement de spirale
        glm::mat4 lightMVMatrix = glm::rotate(generalInfos->globalMVMatrix, (float)glfwGetTime(), glm::vec3(0, 1, 0));                                                        // Translation * Rotation
        glm::vec3 lightPos_vs(lightMVMatrix * glm::vec4(lightPos, 1));

        // /* CHARGEMENT LUMIERE */
        currentLight->ChargeGLints(lightPos_vs);

        /* GENERATION OF CIRCUIT */
        CircuitGeneration(generalInfos, vbo, cylindre);

        // Dessin du sol
        DrawFloor(generalInfos, vbo);

        // Dessin du Wagon
        DrawWagon(generalInfos, vbo);

        // Positionnement de la sphère représentant la lumière
        lightMVMatrix = glm::translate(lightMVMatrix, glm::vec3(lightPos));  // Translation * Rotation * Translation
        lightMVMatrix = glm::scale(lightMVMatrix, glm::vec3(.04, .04, .04)); // Translation * Rotation * Translation * Scale

        myLamp = generalInfos->Lampes[0];
        myLamp->ChargeMatrices(lightMVMatrix, generalInfos->projMatrix);

        /* CHARGEMENT DE LA LAMPE */
        myLamp->ChargeGLints();

        // Dessin de la sphère représentant la lumière
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sphere.getVertexCount() * sizeof(glimac::ShapeVertex), sphere.getDataPointer(), GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, sphere.getVertexCount());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        /* LUNES */

        for (int i = 0; i < generalInfos->NbMoons; ++i) {
            // Transformations nécessaires pour la Lune
            glm::mat4 moonMVMatrix = glm::rotate(generalInfos->globalMVMatrix, (1 + randomTransform[i][0] + randomTransform[i][1] + randomTransform[i][2]) * (float)glfwGetTime(), glm::cross(glm::vec3(1, 1, 1), randomTransform[i])); // Translation * Rotation
            moonMVMatrix           = glm::translate(moonMVMatrix, randomTransform[i]);                                                                                                                                    // Translation * Rotation * Translation
            moonMVMatrix           = glm::scale(moonMVMatrix, glm::vec3(0.2, 0.2, 0.2));                                                                                                                                  // Translation * Rotation * Translation * Scale

            generalInfos->MoonMaterials[i]->ChargeMatrices(moonMVMatrix, generalInfos->projMatrix);

            // glUniform3f(uLightIntensity, .2, .2, .2);
            // glUniform3fv(uLightPos_vs, 1, glm::value_ptr(lightPos_vs));
            // glUniform3f(uKd, .2, 0, 0);
            // glUniform3f(uKs, randomColor[i].r * .8, randomColor[i].g *.5, randomColor[i].b * .2);
            // glUniform1f(uShininess, 2);

            generalInfos->MoonMaterials[i]->ChargeGLints();

            glDrawArrays(GL_TRIANGLES, 0, sphere.getVertexCount());
        }

        glBindVertexArray(0);

        /* Swap front and back buffers */
        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glfwTerminate();

    return argc - 1;
}