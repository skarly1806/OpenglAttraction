#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <cstddef>
#include <glimac/FilePath.hpp>
#include <glimac/FreeFlyCamera.hpp>
#include <glimac/Image.hpp>
#include <glimac/Program.hpp>
#include <glimac/Sphere.hpp>
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

    GLint kd_gl;
    GLint ks_gl;
    GLint shininess_gl;
    GLint hasTexture_gl;

    GLint uTextures_gl[MAX_TEXTURES];

public:
    glm::vec3  kd;
    glm::vec3  ks;
    float      shininess;
    bool       hasTexture;

    Material(){}

    Material(GLint prog_GLid)
    {
        kd_gl         = glGetUniformLocation(prog_GLid, "uMaterial.ks");
        ks_gl         = glGetUniformLocation(prog_GLid, "uMaterial.kd");
        shininess_gl  = glGetUniformLocation(prog_GLid, "uMaterial.shininess");
        hasTexture_gl = glGetUniformLocation(prog_GLid, "uMaterial.hasTexture");
        uMVPMatrix_gl   = glGetUniformLocation(prog_GLid, "uMVPMatrix");
        uMVMatrix_gl    = glGetUniformLocation(prog_GLid, "uMVMatrix");
        uNormalMatrix_gl = glGetUniformLocation(prog_GLid, "uNormalMatrix");

        // Textures
        for (int i = 0; i < MAX_TEXTURES; i++) {
            std::string name = "uMaterial.textures[";
            name.push_back(char(i));
            name.push_back(']');
            uTextures_gl[i] = glGetUniformLocation(prog_GLid, name.c_str());
        }
    }

    void ChargeGLints(){
        glUniform3f(kd_gl, kd.x, kd.y, kd.z);
        glUniform3f(ks_gl, ks.x, ks.y, ks.z);
        glUniform1f(shininess_gl, shininess);
        glUniform1f(hasTexture_gl, hasTexture);
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

public:
    glm::vec3 position;
    glm::vec3 intensity;

    PointLight() {}

    PointLight(GLint prog_GLid)
    {
        position_gl  = glGetUniformLocation(prog_GLid, "uPointLight.position");
        intensity_gl = glGetUniformLocation(prog_GLid, "uPointLight.intensity");
    }

    void ChargeGLints(glm::vec3 light_position)
    {
        glUniform3fv(position_gl, 1, glm::value_ptr(light_position));
        glUniform3f(intensity_gl, intensity.x, intensity.y, intensity.z);
    }
};

struct DirLight {
private:
    GLint direction_gl;
    GLint intensity_gl;

public:
    glm::vec3 direction;
    glm::vec3 intensity;

    DirLight() {}

    DirLight(GLint prog_GLid)
    {
        direction_gl = glGetUniformLocation(prog_GLid, "uDirLight.direction");
        intensity_gl = glGetUniformLocation(prog_GLid, "uDirLight.intensity");
    }

    void ChargeGLints()
    {
        glUniform3f(direction_gl, direction.x, direction.y, direction.z);
        glUniform3f(intensity_gl, intensity.x, intensity.y, intensity.z);
    }
};


struct GeneralInfos {
private:
    GLint AmbiantLight_gl;

public:
    glm::vec3             AmbiantLight = glm::vec3(0, 0, 0);

    Material      *        EarthMaterial;

    std::vector<Material*> MoonMaterials;
    int                   NbMoons;

    DirLight*              MyDirLight;

    PointLight  *          MyPointLight;
    Material   *           MyLamp;

    GeneralInfos(GLuint prog_GLid)
    {
        AmbiantLight_gl = glGetUniformLocation(prog_GLid, "uAmbiantLight");

        AmbiantLight    = glm::vec3(0, 0, 0);
        NbMoons         = 0;
        EarthMaterial   = new Material(prog_GLid);
        MyDirLight      = new DirLight(prog_GLid);
        MyPointLight    = new PointLight(prog_GLid);
        MyLamp          = new Material(prog_GLid);
    }

    void ChargeGLints()
    {
        glUniform3f(AmbiantLight_gl, AmbiantLight.x, AmbiantLight.y, AmbiantLight.z);
    }
};


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
                                        applicationPath.dirPath() + "Projet/shaders/pointlight.fs.glsl"));
    program.use();

    /* GET SHADER ADDRESSES FOR LIGHT */
    GeneralInfos generalInfos(program.getGLId());
    Material*    earthMaterial = generalInfos.EarthMaterial;
    PointLight*  myPointLight  = generalInfos.MyPointLight;
    Material*    myLamp        = generalInfos.MyLamp;
    // DirLight  *     myDirLight    = generalInfos.MyDirLight;

    generalInfos.AmbiantLight = glm::vec3(0.2, 0.2, 0.2);
    generalInfos.ChargeGLints();

    myPointLight->position  = glm::vec3(2, 4, 0);
    myPointLight->intensity = glm::vec3(1, 0, 1);

    myLamp->kd         = glm::vec3(myPointLight->intensity);
    myLamp->ks         = glm::vec3(myPointLight->intensity);
    myLamp->shininess  = 6.f;
    myLamp->hasTexture = false;

    earthMaterial->kd         = glm::vec3(1, 0, 0);
    earthMaterial->ks         = glm::vec3(0, 0, 1);
    earthMaterial->shininess  = 5;
    earthMaterial->hasTexture = false;

    glEnable(GL_DEPTH_TEST); // Permet d'activer le test de profondeur du GPU

    /* CALCULATE MATRICES */
    glm::mat4 projMatrix     = glm::perspective(glm::radians(70.f), float(window_width) / float(window_height), 0.1f, 100.f);
    glm::mat4 globalMVMatrix = glm::translate(glm::mat4(), glm::vec3(0, 0, -5));
    // glm::mat4 normalMatrix = glm::transpose(glm::inverse(MVMatrix));

    // Création d'une sphère
    glimac::Sphere sphere(1, 64, 32);

    // Création d'un cylindre
    glimac::Cylindre cylindre(1, 0.5, 30, 30);

    /* VBO + VAO */
    GLuint vbo;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Envoie des données de vertex
    glBufferData(GL_ARRAY_BUFFER, sphere.getVertexCount() * sizeof(glimac::ShapeVertex), sphere.getDataPointer(), GL_STATIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, cylindre.getVertexCount() * sizeof(glimac::ShapeVertex), cylindre.getDataPointer(), GL_STATIC_DRAW);

    // Débinding du VBO
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Création du VAO
    GLuint vao;
    glGenVertexArrays(1, &vao);

    // Binding du VAO
    glBindVertexArray(vao);
    const GLuint VERTEX_ATTR_POSITION = 0;
    const GLuint VERTEX_ATTR_NORMAL   = 1;
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
    generalInfos.NbMoons = 0;
    std::vector<glm::vec3> randomTransform;

    for (int i = 0; i < generalInfos.NbMoons; ++i) {
        Material* new_moon = new Material(program.getGLId());
        randomTransform.push_back(glm::sphericalRand(2.f));
        new_moon->kd         = glm::vec3(glm::linearRand(0.1f, 1.f) * .8, glm::linearRand(0.1f, 1.f) * .5, glm::linearRand(0.1f, 1.f) * .2);
        new_moon->ks         = glm::vec3(.2, 0, 0);
        new_moon->shininess  = randomFloat(20.f);
        new_moon->hasTexture = false;
        generalInfos.MoonMaterials.push_back(new_moon);
    }

    /* CAMERA */
    glimac::TrackballCamera camera;

    /* Loop until the user closes the window */
    while (!glfwWindowShouldClose(window)) {
        /* EVENTS */

        /* MOUSE */
        glfwPollEvents();
        // double c_xpos, c_ypos;
        // glfwGetCursorPos(window, &c_xpos, &c_ypos);

        // camera.rotateUp(c_xpos);
        // camera.rotateLeft(c_ypos);

        // /* KEYBOARD */
        // int state = glfwGetKey(window, GLFW_KEY_W);
        // if (state == GLFW_PRESS) {
        //     camera.moveFront(0.2f);
        // }
        // state = glfwGetKey(window, GLFW_KEY_S);
        // if (state == GLFW_PRESS) {
        //     camera.moveFront(-0.2f);
        // }

        /* RENDERING */

        glm::mat4 ViewMatrix = camera.getViewMatrix();

        // clear window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // get camera matrix
        globalMVMatrix = ViewMatrix;

        // earth rotates on itself
        glm::mat4 earthMVMatrix = glm::rotate(globalMVMatrix, (float)glfwGetTime(), glm::vec3(0, 1, 0)); // Translation * Rotation

        // send Matrixes values to shader
        earthMaterial->ChargeMatrices(earthMVMatrix, projMatrix);

        glBindVertexArray(vao);

        /* GESTION LUMIERE */

        // glm::mat4 lightMVMatrix = globalMVMatrix;
        glm::vec3 lightPos(myPointLight->position.x, myPointLight->position.y * (glm::cos((float)glfwGetTime()) * glm::sin((float)glfwGetTime())), myPointLight->position.z); // position mouvement de spirale
        glm::mat4 lightMVMatrix = glm::rotate(globalMVMatrix, (float)glfwGetTime(), glm::vec3(0, 1, 0));                                                                      // Translation * Rotation
        glm::vec3 lightPos_vs(lightMVMatrix * glm::vec4(lightPos, 1));

        // /* CHARGEMENT LUMIERE */
        myPointLight->ChargeGLints(lightPos_vs);

        /* CHARGEMENT MATERIAU TERRE */
        earthMaterial->ChargeGLints();

        // Dessin de la terre
        glDrawArrays(GL_TRIANGLES, 0, sphere.getVertexCount());
        //Dessin du cylindre
        glDrawArrays(GL_TRIANGLES, 0, cylindre.getVertexCount());

        // Positionnement de la sphère représentant la lumière
        lightMVMatrix = glm::translate(lightMVMatrix, glm::vec3(lightPos));  // Translation * Rotation * Translation
        lightMVMatrix = glm::scale(lightMVMatrix, glm::vec3(.04, .04, .04)); // Translation * Rotation * Translation * Scale

        myLamp->ChargeMatrices(lightMVMatrix, projMatrix);

        /* CHARGEMENT DE LA LAMPE */
        myLamp->ChargeGLints();

        // Dessin de la sphère représentant la lumière
        glDrawArrays(GL_TRIANGLES, 0, sphere.getVertexCount());

        /* LUNES */

        for (int i = 0; i < generalInfos.NbMoons; ++i) {
            // Transformations nécessaires pour la Lune
            glm::mat4 moonMVMatrix = glm::rotate(globalMVMatrix, (1 + randomTransform[i][0] + randomTransform[i][1] + randomTransform[i][2]) * (float)glfwGetTime(), glm::cross(glm::vec3(1, 1, 1), randomTransform[i])); // Translation * Rotation
            moonMVMatrix           = glm::translate(moonMVMatrix, randomTransform[i]);                                                                                                                                    // Translation * Rotation * Translation
            moonMVMatrix           = glm::scale(moonMVMatrix, glm::vec3(0.2, 0.2, 0.2));                                                                                                                                  // Translation * Rotation * Translation * Scale

            generalInfos.MoonMaterials[i]->ChargeMatrices(moonMVMatrix, projMatrix);

            // glUniform3f(uLightIntensity, .2, .2, .2);
            // glUniform3fv(uLightPos_vs, 1, glm::value_ptr(lightPos_vs));
            // glUniform3f(uKd, .2, 0, 0);
            // glUniform3f(uKs, randomColor[i].r * .8, randomColor[i].g *.5, randomColor[i].b * .2);
            // glUniform1f(uShininess, 2);

            generalInfos.MoonMaterials[i]->ChargeGLints();

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