#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <cstddef>
#include <glimac/FilePath.hpp>
#include <glimac/FreeFlyCamera.hpp>
#include <glimac/Image.hpp>
#include <glimac/Program.hpp>
#include <glimac/Sphere.hpp>
#include <glimac/Cone.hpp>
#include <glimac/Cylindre.hpp>
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

struct Vertex2DUV {
    glm::vec2 position;
    glm::vec2 uv;

    Vertex2DUV(){};
    Vertex2DUV(glm::vec2 p, glm::vec2 c)
    {
        position = p;
        uv       = c;
    };
};

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

    Material* EarthMaterial;

    std::vector<Material*> MoonMaterials;
    int                    NbMoons;

    std::vector<DirLight*> DirLights;
    std::vector<PointLight*> PointLights;
    std::vector<Material*> Lampes;

    glm::vec2 NbLights = glm::vec2(0, 0);

    std::vector<glm::vec3> Circuit;
    std::vector<glm::vec3> CircuitColors;
    int NbCircuitPoints = 0;

    GeneralInfos(GLint prog_GLid)
    {
        AmbiantLight_gl = glGetUniformLocation(prog_GLid, "uAmbiantLight");
        ViewPos_gl = glGetUniformLocation(prog_GLid, "uViewPos");

        NbLights_gl = glGetUniformLocation(prog_GLid, "NbLights");

        ViewPos         = glm::vec3(0, 0, 0);
        AmbiantLight    = glm::vec3(0, 0, 0);
        NbMoons         = 0;
        EarthMaterial   = new Material(prog_GLid);
    }

    void ChargeGLints()
    {
        glUniform3f(AmbiantLight_gl, AmbiantLight.x, AmbiantLight.y, AmbiantLight.z);
        glUniform3f(ViewPos_gl, ViewPos.x, ViewPos.y, ViewPos.z);
        glUniform2f(NbLights_gl, NbLights.x, NbLights.y);
    }
};

void HandleEvents(GLFWwindow* window, glimac::TrackballCamera* camera)
{
    double c_xpos, c_ypos;
    glfwGetCursorPos(window, &c_xpos, &c_ypos);

    camera->rotateUp(c_xpos);
    camera->rotateLeft(c_ypos);

    /* KEYBOARD */
    int state = glfwGetKey(window, GLFW_KEY_W);
    if (state == GLFW_PRESS) {
        camera->moveFront(0.2f);
    }
    state = glfwGetKey(window, GLFW_KEY_S);
    if (state == GLFW_PRESS) {
        camera->moveFront(-0.2f);
    }
}

void CircuitGeneration(GeneralInfos* generalInfos, GLuint vbo, glimac::Cylindre cylindre)
{
    printf("new:\n");
    for (int i = 0; i < generalInfos->NbCircuitPoints - 1; i++) {
        generalInfos->EarthMaterial->color = generalInfos->CircuitColors[i];

        glm::vec3 Pstart = generalInfos->Circuit[i];
        glm::vec3 Pend = generalInfos->Circuit[i+1];
        glm::vec3 direction = glm::normalize(Pend - Pstart);
        glm::vec3 cylinderDirection = glm::vec3(0, 0, 1);

        float angle = 180.f;
        glm::vec3 axis = glm::cross(cylinderDirection, direction);
        if (glm::length(axis) > 0.1f)
            angle = glm::asin(glm::length(axis));
        else
            axis = glm::vec3(0, 1, 0);

        printf("%f, %f %f %f, LEN : %f\n", angle, axis.x, axis.y, axis.z, glm::length(direction));

        glm::mat4 circuitMVMatrix = generalInfos->globalMVMatrix;
        circuitMVMatrix           = glm::translate(circuitMVMatrix, Pstart);
        circuitMVMatrix           = glm::rotate(circuitMVMatrix, angle, axis);

        generalInfos->EarthMaterial->ChargeMatrices(circuitMVMatrix, generalInfos->projMatrix);
        generalInfos->EarthMaterial->ChargeGLints();

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, cylindre.getVertexCount() * sizeof(glimac::ShapeVertex), cylindre.getDataPointer(), GL_STATIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, cylindre.getVertexCount());
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
    printf("\n\n");
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

    GeneralInfos* generalInfos = new GeneralInfos(program.getGLId());

    std::vector<glm::vec3> circuit ;
    circuit.push_back(glm::vec3(0.00f, 0.00f, 0.00f));
    circuit.push_back(glm::vec3(0.55f, 0.30f, 0.15f));
    circuit.push_back(glm::vec3(1.33f, 0.52f, 0.81f));
    circuit.push_back(glm::vec3(2.11f, 0.91f, 1.96f));
    circuit.push_back(glm::vec3(2.39f, 2.52f, 2.41f));
    circuit.push_back(glm::vec3(3.35f, 1.83f, 2.98f));
    circuit.push_back(glm::vec3(3.76f, 3.71f, 3.58f));
    circuit.push_back(glm::vec3(4.22f, 4.00f, 4.06f));
    circuit.push_back(glm::vec3(5.15f, 3.47f, 5.05f));
    circuit.push_back(glm::vec3(5.42f, 5.18f, 5.86f));
    circuit.push_back(glm::vec3(5.45f, 5.50f, 6.29f));
    generalInfos->Circuit         = circuit;
    generalInfos->NbCircuitPoints = 11;

    for(int i = 0; i<generalInfos->NbCircuitPoints-1; i++){
        generalInfos->CircuitColors.push_back(glm::vec3(randomFloat(1.f), randomFloat(1.f), randomFloat(1.f)));

        // couleurs pour les tests
        if (i == 0) generalInfos->CircuitColors[0] = glm::vec3(1, 1, 1);
        if (i == 1) generalInfos->CircuitColors[1] = glm::vec3(1, 0, 0);
        if (i == 2) generalInfos->CircuitColors[2] = glm::vec3(0, 1, 0);
        if (i == 3) generalInfos->CircuitColors[3] = glm::vec3(0, 0, 1);
    }
    Material* earthMaterial = generalInfos->EarthMaterial;

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

    // set earth infos
    earthMaterial->color = glm::vec3(1, 0, 0);
    earthMaterial->specularIntensity = 1.f;
    earthMaterial->shininess  = 30;
    earthMaterial->hasTexture = false;
    earthMaterial->isLamp = false;

    /* CALCULATE MATRICES */
    glm::mat4 projMatrix     = glm::perspective(glm::radians(70.f), float(window_width) / float(window_height), 0.1f, 100.f);
    glm::mat4 globalMVMatrix = glm::translate(glm::mat4(), glm::vec3(0, 0, -5));

    generalInfos->projMatrix = projMatrix;
    generalInfos->globalMVMatrix = globalMVMatrix;

    // Création d'une sphère
    glimac::Sphere sphere(1, 64, 32);

    // Création d'un cylindre
    glimac::Cylindre cylindre(1, .03, 5, 5);

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

    /* CAMERA */
    glimac::TrackballCamera* camera = new glimac::TrackballCamera();

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* EVENTS */
        glfwPollEvents();

        /* MOUSE */
        HandleEvents(window, camera);

        /* RENDERING */

        glm::mat4 ViewMatrix  = camera->getViewMatrix();
        generalInfos->ViewPos = glm::vec3(ViewMatrix[0][0], ViewMatrix[0][1], ViewMatrix[0][2]);
        generalInfos->ChargeGLints();

        // clear window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // get camera matrix
        generalInfos->globalMVMatrix = ViewMatrix;

        // earth rotates on itself
        //glm::mat4 earthMVMatrix = generalInfos->globalMVMatrix; // glm::rotate(generalInfos->globalMVMatrix, (float)glfwGetTime(), glm::vec3(0, 1, 0)); // Translation * Rotation

        // send Matrixes values to shader
        earthMaterial->ChargeMatrices(generalInfos->globalMVMatrix, generalInfos->projMatrix);

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

        /* CHARGEMENT MATERIAU TERRE */
        //earthMaterial->ChargeGLints();

        // Dessin de la terre
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sphere.getVertexCount() * sizeof(glimac::ShapeVertex), sphere.getDataPointer(), GL_STATIC_DRAW);
        //glDrawArrays(GL_TRIANGLES, 0, sphere.getVertexCount());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Dessin du cylindre
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, cylindre.getVertexCount() * sizeof(glimac::ShapeVertex), cylindre.getDataPointer(), GL_STATIC_DRAW);
        //glDrawArrays(GL_TRIANGLES, 0, cylindre.getVertexCount());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Dessin du cone
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, cone.getVertexCount() * sizeof(glimac::ShapeVertex), cone.getDataPointer(), GL_STATIC_DRAW);
        //glDrawArrays(GL_TRIANGLES, 0, cone.getVertexCount());
        glBindBuffer(GL_ARRAY_BUFFER, 0);

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
        /* Poll for and process events */
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    glfwTerminate();

    return argc - 1;
}