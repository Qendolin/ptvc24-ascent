#pragma once
#define GLEW_STATIC
#include <GL/glew.h>

#include <glm/glm.hpp>

#include "GL/Geometry.h"
#include "GL/Shader.h"
#include "Utils.h"

typedef struct MatrixStackEntry {
    glm::mat4 positionMatrix;
    glm::mat3 normalMatrix;
} MatrixStackEntry;

class DirectBuffer {
   private:
    GL::ShaderPipeline* shader_;
    GL::VertexArray* vao_;
    GL::Buffer* vbo_;
    std::vector<float> data_;
    std::vector<MatrixStackEntry> stack_ = {{glm::mat4(1.0), glm::mat3(1.0)}};
    glm::vec3 color_ = glm::vec3(1, 1, 1);
    float stroke_ = 0.05;
    bool shaded_ = false;
    bool autoShade_ = false;
    glm::vec3 normal_ = {0, 0, 0};

    int circleSides(float r) {
        return 24 + (int)(0.6 * r);
    }

   public:
    DirectBuffer(GL::ShaderPipeline* shader);

    void push();

    void pop();

    void transform(glm::mat4 matrix);

    void stroke(float width);

    void shaded();

    void unshaded();

    void color(float r, float g, float b);
    void color(glm::vec3 c);

    void light(glm::vec3 c);

    void vert(glm::vec3 pos);

    void tri(glm::vec3 a, glm::vec3 b, glm::vec3 c);

    void quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);

    void plane(glm::vec3 min, glm::vec3 max, glm::vec3 n);

    void line(glm::vec3 a, glm::vec3 b);

    void circleLine(glm::vec3 c, glm::vec3 n, float r);

    void circle(glm::vec3 c, glm::vec3 n, float r);

    void regularPolyLine(glm::vec3 c, glm::vec3 n, float r, int s);

    void regularPoly(glm::vec3 c, glm::vec3 n, float r, int s);

    void coneLine(glm::vec3 c, glm::vec3 d, float a);

    void regularPyramidLine(glm::vec3 c, glm::vec3 d, float a, int s);

    void uvSphere(glm::vec3 c, float r);

    void unitBox();

    void draw(glm::mat4 viewProj, glm::vec3 camPos);

    void clear();
};
