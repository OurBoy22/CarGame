#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "io/file.h"

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const double ACCELERATION = 3;
const double TURN_RATE = 3;
const double FRICTION_MULTIPLIER = 2;
const double PI = 3.1415926;
const double DEADZONE = 0.01;
const double CAR_SCALE = 1;
const double WORLD_SIZE = 50;
const double BOOST_MULTIPLIER = 4;

// struct to track object position and velocity
typedef struct {
    double x;
    double y;
    double z;
    double xGPUCoords;
    double yGPUCoords;
    double zGPUCoords;
    double xVelocity;
    double yVelocity;
    double absVelocity;
    double angle;
} ObjectState;


void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, ObjectState* objState, double dt);
void updatePosition(ObjectState *objState, double dt);
void printObjectState(const ObjectState* objState, double frameTime);
void multiplyMatrices(float *result, const float *a, const float *b);



const char *vertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "uniform mat4 transform;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = transform * vec4(aPos, 1.0);\n"
    "}\0";
const char *fragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "void main()\n"
    "{\n"
    "   FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
    "}\n\0";

const char* worldVertexShaderSource = "#version 330 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "\n"
    "out vec2 worldPos;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    worldPos = aPos.xy;   // keep coordinates\n"
    "    gl_Position = vec4(aPos, 1.0);\n"
    "}\0";


const char* worldFragmentShaderSource = "#version 330 core\n"
    "out vec4 FragColor;\n"
    "\n"
    "in vec2 worldPos;\n"
    "\n"
    "uniform float xOffset;\n"
    "uniform float yOffset;\n"
    "\n"
    "void main()\n"
    "{\n"
    "    // Move the grid downward over time\n"
    "    vec2 p = worldPos * 10.0;\n"
    "    p.x += xOffset;\n"
    "    p.y += yOffset;\n"
    "\n"
    "    // Create repeating pattern\n"
    "    vec2 grid = abs(fract(p) - 0.5);\n"
    "\n"
    "    // Line thickness\n"
    "    float line = min(grid.x, grid.y);\n"
    "\n"
    "    // Grid line threshold\n"
    "    float gridLine = step(line, 0.02);\n"
    "\n"
    "    // Colors\n"
    "    vec3 gridColor = vec3(0.2, 0.8, 0.2);\n"
    "    vec3 background = vec3(0.05);\n"
    "\n"
    "    vec3 color = mix(background, gridColor, gridLine);\n"
    "    FragColor = vec4(color, 1.0);\n"
    "}\0";

void applyShader(char* vertexShaderSource, char* fragmentShaderSource, unsigned int* shaderProgram){
    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::VERTEX::COMPILATION_FAILED\n%s\n", infoLog);
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        printf("ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n%s\n", infoLog);
    }
    // link shaders

    glAttachShader(*shaderProgram, vertexShader);
    glAttachShader(*shaderProgram, fragmentShader);
    // check for linking errors
    glGetProgramiv(*shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(*shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

int main(int argc, char** argv)
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to initialize GLAD\n");
        return -1;
    }
    const char* worldVertexShaderSource = loadShaderSource("/src/assets/bg/bgVertex.frag");
    const char* worldFragmentShaderSource = loadShaderSource("/src/assets/bg/bgFragment.frag");
    
  
    
    // printf("Program path: %s\n", PROJECT_ROOT);
    printf("Loaded world shaders:\n%s\n%s\n", worldVertexShaderSource, worldFragmentShaderSource);
    // return 0;
    // build and compile our shader program
    unsigned int shaderProgramCar = glCreateProgram();
    applyShader((char*)vertexShaderSource, (char*)fragmentShaderSource, &shaderProgramCar);
    glLinkProgram(shaderProgramCar);

    unsigned int shaderProgramBG = glCreateProgram();
    applyShader((char*)worldVertexShaderSource, (char*)worldFragmentShaderSource, &shaderProgramBG);
    glLinkProgram(shaderProgramBG);
    

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        -0.03f,  0.03f, 0.0f, // top  
         0.067f,  0.0f, 0.0f, // right 
        -0.03f, -0.03f, 0.0f  // bottom  
    };
    
    // apply scaling
    for (int i = 0; i < 9; i++) {
        vertices[i] *= CAR_SCALE;
    }

    // 6 vertices = 2 triangles
    float quad[] = {
        -1, -1, 0,
        1, -1, 0,
        1,  1, 0,

        -1, -1, 0,
        1,  1, 0,
        -1,  1, 0
    };

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0); 

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0); 


    // Background
    unsigned int bgVAO, bgVBO;
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);

    glBindVertexArray(bgVAO);

    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // render loop
    // -----------
    ObjectState objState = {0.0f, // x
                            0.0f, // y
                            0.0f, // z
                            0.0f, // xGPUCoords
                            0.0f, // yGPUCoords
                            0.0f, // zGPUCoords
                            0.0f, // xVelocity
                            0.0f, // yVelocity
                            0.0f, // absVelocity
                            0.0f  // angle
                           };

    // Variables to track time
    static double lastFrameTime = 0.0;
    double currentFrameTime;
    double deltaTime;

    while (!glfwWindowShouldClose(window))
    {
        // 1. Get current time and calculate delta time
        currentFrameTime = glfwGetTime();
        deltaTime = currentFrameTime - lastFrameTime;
        lastFrameTime = currentFrameTime;



        // input
        // -----
        processInput(window, &objState, deltaTime);
        updatePosition(&objState, deltaTime);
        printObjectState(&objState, deltaTime);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

         // update background offsets
        // Translation matrix
        

        // draw our first triangle
        glUseProgram(shaderProgramBG);
        float xOffset = objState.x * 1;
        float yOffset = objState.y * 1;
        
        int transformLoc = glGetUniformLocation(shaderProgramBG, "xOffset");
        glUniform1f(transformLoc, xOffset);
        transformLoc = glGetUniformLocation(shaderProgramBG, "yOffset");
        glUniform1f(transformLoc, yOffset);

        glBindVertexArray(bgVAO); // bind background VAO
        glDrawArrays(GL_TRIANGLES, 0, 6);


        glUseProgram(shaderProgramCar);

        // Translation matrix
        float translationMatrix[16] = {
            1, 0, 0, 0,  // column 0
            0, 1, 0, 0,  // column 1
            0, 0, 1, 0,  // column 2
            0, 0, 0, 1   // column 3
            // objState.x, objState.y, 0, 1 // column 3
        };
        
        //update rotation
        float cosA = cosf(objState.angle);
        float sinA = sinf(objState.angle);

        float rotationMatrix[16] = {
            cosA, sinA, 0, 0,
            -sinA, cosA, 0, 0,
            0,    0, 1, 0,
            0,    0, 0, 1
        };

        // Model = translation * rotation on car
        float model[16];
        multiplyMatrices(model, translationMatrix, rotationMatrix);
        transformLoc = glGetUniformLocation(shaderProgramCar, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, model);

        
        glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        glDrawArrays(GL_TRIANGLES, 0, 3);

        
        // glBindVertexArray(0); // no need to unbind it every time 
 
        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgramCar);
    glDeleteVertexArrays(1, &bgVAO);
    glDeleteBuffers(1, &bgVBO);
    glDeleteProgram(shaderProgramBG);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void multiplyMatrices(float *result, const float *a, const float *b) {
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a[k * 4 + row] * b[col * 4 + k]; // column-major
            }
            result[col * 4 + row] = sum;
        }
    }
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, ObjectState* objState, double dt)
{  

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
        glfwSetWindowShouldClose(window, true);
    }
    else if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS){
        // reset object state
        objState->x = 0.0f;
        objState->y = 0.0f;
        objState->xVelocity = 0.0f;
        objState->yVelocity = 0.0f;
        objState->absVelocity = 0.0f;
        objState->angle = 0.0f;
        objState->z = 0.0f;
        objState->xGPUCoords = 0.0f;
        objState->yGPUCoords = 0.0f;
        objState->zGPUCoords = 0.0f;
    }

    bool directionKeyPressed = false;
    bool throttleOrBrakePressed = false;

    // can only steer when there's some velocity
    if (fabs(objState->absVelocity) > DEADZONE){
        // Steer right
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
            directionKeyPressed = true;
            // adjust speed by ACCELERATION
            // objState->xVelocity += ACCELERATION * dt;
            objState->angle -= TURN_RATE * dt;

        } // Steer left
        else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
            directionKeyPressed = true;
            // objState->xVelocity -= ACCELERATION * dt;
            objState->angle += TURN_RATE * dt;
        }
    }
    
    // Throttle
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        throttleOrBrakePressed = true;
        // objState->yVelocity += ACCELERATION * dt;

        // check if boost held
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            objState->absVelocity += ACCELERATION * BOOST_MULTIPLIER * dt;
        else
            objState->absVelocity += ACCELERATION * dt;
    } // Brake
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        throttleOrBrakePressed = true;
        // objState->yVelocity -= ACCELERATION * dt;
        objState->absVelocity -= ACCELERATION * FRICTION_MULTIPLIER * 3 * dt;
        if (objState->absVelocity < 0.0f)
            objState->absVelocity = 0.0f;
    }

    // otherwise simulate friction to gradually stop the object
    if (!throttleOrBrakePressed)
    {
        // // check if there's positive velocity in x
        // if (objState->xVelocity > DEADZONE)
        //     objState->xVelocity -= ACCELERATION * FRICTION_MULTIPLIER * dt;
        // else if (objState->xVelocity < -DEADZONE)
        //     objState->xVelocity += ACCELERATION * FRICTION_MULTIPLIER * dt;
        // else
        //     objState->xVelocity = 0.0f;
        
        // // check if there's positive velocity in y
        // if (objState->yVelocity > DEADZONE)
        //     objState->yVelocity -= ACCELERATION * FRICTION_MULTIPLIER * dt;
        // else if (objState->yVelocity < -DEADZONE)
        //     objState->yVelocity += ACCELERATION * FRICTION_MULTIPLIER * dt;
        // else
        //     objState->yVelocity = 0.0f;
        if (objState->absVelocity > DEADZONE)
            objState->absVelocity -= ACCELERATION * FRICTION_MULTIPLIER * dt;
        else if (objState->absVelocity < -DEADZONE)
            objState->absVelocity += ACCELERATION * FRICTION_MULTIPLIER * dt;
        else
            objState->absVelocity = 0.0f;
        
    }
}

void updatePosition(ObjectState* objState, double dt)
{  
    // update velocity components based on current angle and absolute velocity
    // objState->xVelocity = objState->absVelocity * sin(objState->angle + PI/2) ;
    // objState->yVelocity = objState->absVelocity * cos(objState->angle + PI/2);
    objState->xVelocity = objState->absVelocity * cos(objState->angle);
    objState->yVelocity = objState->absVelocity * sin(objState->angle);
    
    // update position based on velocity
    objState->x += objState->xVelocity * dt;
    objState->y += objState->yVelocity * dt;

    // handle boundary conditions
    if (objState->x > WORLD_SIZE)
        // objState->x = -WORLD_SIZE;
        objState->x = WORLD_SIZE;
    else if (objState->x < -WORLD_SIZE)
        // objState->x = WORLD_SIZE;
        objState->x = -WORLD_SIZE;
    
    if (objState->y > WORLD_SIZE)
        // objState->y = -WORLD_SIZE;
        objState->y = WORLD_SIZE;
    else if (objState->y < -WORLD_SIZE)
        // objState->y = WORLD_SIZE;
        objState->y = -WORLD_SIZE;

    // update rotation angle based on angular velocity
    // objState->angle = atan2(objState->yVelocity, objState->xVelocity) - PI/2; // safer version to handle quadrants       

}

void printObjectState(const ObjectState* objState, double frameTime)
{
    printf("Position: (%.4f, %.4f) Velocity: (%.4f, %.4f) Angle: %.4f Frame Time: %.12f seconds | FPS: %.2f\n", objState->x, objState->y, objState->xVelocity, objState->yVelocity, objState->angle*180/PI, frameTime, 1.0 / frameTime);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}