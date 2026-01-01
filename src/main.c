#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdbool.h>
#include <math.h>

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const double ACCELERATION = 1;
const double FRICTION_MULTIPLIER = 5;
const double PI = 3.1415926;
const double DEADZONE = 0.01;

// struct to track object position and velocity
typedef struct {
    double x;
    double y;
    double xVelocity;
    double yVelocity;
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


int main()
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
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        printf("ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        -0.1f, -0.2f, 0.0f, // left  
         0.1f, -0.2f, 0.0f, // right 
         0.0f,  0.1f, 0.0f  // top   
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


    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    ObjectState objState = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

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

        // glUseProgram(shaderProgram);
        // Translation matrix
        float translationMatrix[16] = {
            1, 0, 0, 0,  // column 0
            0, 1, 0, 0,  // column 1
            0, 0, 1, 0,  // column 2
            objState.x, objState.y, 0, 1 // column 3
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

        // Model = translation * rotation
        float model[16];
        multiplyMatrices(model, translationMatrix, rotationMatrix);
        int transformLoc = glGetUniformLocation(shaderProgram, "transform");
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, model);

        

        // draw our first triangle
        glUseProgram(shaderProgram);
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
    glDeleteProgram(shaderProgram);

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
        objState->xVelocity = 0.0f;
        objState->yVelocity = 0.0f;
        objState->x = 0.0f;
        objState->y = 0.0f;
    }

    bool directionKeyPressed = false;

    // X Direction Handling
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        directionKeyPressed = true;
        // adjust speed by ACCELERATION
        objState->xVelocity += ACCELERATION * dt;
    }
    else if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        directionKeyPressed = true;
        objState->xVelocity -= ACCELERATION * dt;
    }
    
    // Y Direction Handling
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        directionKeyPressed = true;
        objState->yVelocity += ACCELERATION * dt;
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        directionKeyPressed = true;
        objState->yVelocity -= ACCELERATION * dt;
    }

    // otherwise simulate friction to gradually stop the object
    if (!directionKeyPressed)
    {
        // check if there's positive velocity in x
        if (objState->xVelocity > DEADZONE)
            objState->xVelocity -= ACCELERATION * FRICTION_MULTIPLIER * dt;
        else if (objState->xVelocity < -DEADZONE)
            objState->xVelocity += ACCELERATION * FRICTION_MULTIPLIER * dt;
        else
            objState->xVelocity = 0.0f;
        
        // check if there's positive velocity in y
        if (objState->yVelocity > DEADZONE)
            objState->yVelocity -= ACCELERATION * FRICTION_MULTIPLIER * dt;
        else if (objState->yVelocity < -DEADZONE)
            objState->yVelocity += ACCELERATION * FRICTION_MULTIPLIER * dt;
        else
            objState->yVelocity = 0.0f;
        
    }
}

void updatePosition(ObjectState* objState, double dt)
{  
    objState->x += objState->xVelocity * dt;
    objState->y += objState->yVelocity * dt;

    // handle boundary conditions
    if (objState->x > 1.0f)
        objState->x = -1.0f;
    else if (objState->x < -1.0f)
        objState->x = 1.0f;
    
    if (objState->y > 1.0f)
        objState->y = -1.0f;
    else if (objState->y < -1.0f)
        objState->y = 1.0f;

    // update rotation angle based on angular velocity
    objState->angle = atan2(objState->yVelocity, objState->xVelocity) - PI/2; // safer version to handle quadrants   

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