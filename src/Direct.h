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

/**
 * A utility for debug drawing.
 * Using the methods provided many kinds of primitives can be added to the buffer.
 * Using the draw method they are rendered all at once when the frame is finished.
 */
class DirectBuffer {
   private:
    GL::ShaderPipeline* shader_;
    GL::VertexArray* vao_;
    GL::Buffer* vbo_;
    std::vector<float> data_;
    std::vector<MatrixStackEntry> stack_ = {{glm::mat4(1.0), glm::mat3(1.0)}};
    glm::vec3 color_ = glm::vec3(1, 1, 1);
    float stroke_ = 0.05f;
    bool shaded_ = false;
    bool autoShade_ = false;
    glm::vec3 normal_ = {0, 0, 0};

    ~DirectBuffer(){};

   public:
    DirectBuffer(GL::ShaderPipeline* shader);

    void destroy();

    /**
     * Adds a new entry to the transformation stack.
     */
    void push();

    /**
     * Removes the top entry from the transformation stack.
     */
    void pop();

    /**
     * Applies the matrix transformation to the top entry from the transformation stack.
     */
    void transform(glm::mat4 matrix);

    /**
     * Set line stroke width.
     */
    void stroke(float width);

    /**
     * Enables simple shading.
     */
    void shaded();

    /**
     * Disables simple shading. Use flat colors.
     */
    void unshaded();

    /**
     * Sets the active color.
     */
    void color(float r, float g, float b);
    void color(glm::vec3 rgb);

    /**
     * Sets the active color, but normalizs the input.
     */
    void light(glm::vec3 rgb);

    /**
     * Add a vertex to the buffer.
     */
    void vert(glm::vec3 pos);

    /**
     * Add a triangle to the buffer.
     */
    void tri(glm::vec3 a, glm::vec3 b, glm::vec3 c);

    /**
     * Add a quad to the buffer.
     * Vertices should be specified in CW order.
     */
    void quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d);

    /**
     * Add a plane to the buffer.
     * @param min the first corner of the plane.
     * @param max the second corner of the plane.
     * @param up the up vector
     */
    void plane(glm::vec3 min, glm::vec3 max, glm::vec3 up = {0, 1, 0});

    /**
     * Add a line to the buffer.
     */
    void line(glm::vec3 a, glm::vec3 b);

    /**
     * Adds a "hollow" circle to the buffer.
     * @param c center
     * @param n normal
     * @param r radius
     */
    void circleLine(glm::vec3 c, glm::vec3 n, float r);

    /**
     * Adds a "filled" circle (a disc) to the buffer.
     * @param c center
     * @param n normal
     * @param r radius
     */
    void circle(glm::vec3 c, glm::vec3 n, float r);

    /**
     * Adds a "hollow" regular polygon to the buffer.
     * @param c center
     * @param n normal
     * @param r radius
     * @param s sides
     */
    void regularPolyLine(glm::vec3 c, glm::vec3 n, float r, int s);

    /**
     * Adds a "filled" regular polygon to the buffer.
     * @param c center
     * @param n normal
     * @param r radius
     * @param s sides
     */
    void regularPoly(glm::vec3 c, glm::vec3 n, float r, int s);

    /**
     * Adds a "hollow" cone to the buffer.
     * @param c center
     * @param d direction
     * @param a angle phase / rotation
     */
    void coneLine(glm::vec3 c, glm::vec3 d, float a);

    /**
     * Adds a "hollow" regular pyramid to the buffer.
     * @param c center
     * @param d direction
     * @param a angle phase / rotation
     * @param s sides
     */
    void regularPyramidLine(glm::vec3 c, glm::vec3 d, float a, int s);

    /**
     * Adds a "filled" uv sphere to the buffer.
     * @param c center
     * @param r radius
     */
    void uvSphere(glm::vec3 c, float r);

    /**
     * Adds a unit box to the bufffer.
     * Side length one. From `(-0.5, -0.5, -0.5)` to `(0.5, 0.5, 0.5)`.
     * Use transformation for scaling and position.
     */
    void unitBox();

    /**
     * Renders the buffer's contents and empties it for the next frame.
     */
    void draw(glm::mat4 view_proj_mat, glm::vec3 camera_pos);

    void clear();
};
