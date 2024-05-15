#include "Direct.h"

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

#include "../GL/Geometry.h"
#include "../GL/Shader.h"
#include "../GL/StateManager.h"
#include "../Util/Log.h"

// calculate number of sides for a given radius
int circleSides(float r) {
    return 24 + (int)(0.6 * r);
}

// calculate a perpendicular vector
// https://math.stackexchange.com/a/1681815/1014081
glm::vec3 perpendicular(glm::vec3 v) {
    float lx = v[0] * v[0];
    float ly = v[1] * v[1];
    float lz = v[2] * v[2];

    float smallest = lx;
    int index = 0;
    if (smallest > ly) {
        smallest = ly;
        index = 1;
    }
    if (smallest > lz) {
        index = 2;
    }
    glm::vec3 e = {};
    e[index] = 1;
    return glm::cross(v, e);
}

DirectBuffer::DirectBuffer() {
    shader_ = new gl::ShaderPipeline(
        {new gl::ShaderProgram("assets/shaders/direct.vert"),
         new gl::ShaderProgram("assets/shaders/direct.frag")});
    shader_->setDebugLabel("direct_buffer/shader");

    vao_ = new gl::VertexArray();
    vao_->setDebugLabel("direct_buffer/vao");
    vao_->layout(0, 0, 3, GL_FLOAT, false, 0);
    vao_->layout(0, 1, 3, GL_FLOAT, false, 3 * 4);
    vao_->layout(0, 2, 3, GL_FLOAT, false, 6 * 4);
    vbo_ = new gl::Buffer();
    vbo_->setDebugLabel("direct_buffer/vbo");
    vbo_->allocateEmpty((size_t)1e6, GL_DYNAMIC_STORAGE_BIT);
    // 3 floats position + 3 floats color + 3 float normal
    vao_->bindBuffer(0, *vbo_, 0, (3 + 3 + 3) * 4);
}

DirectBuffer::~DirectBuffer() {
    delete vao_;
    delete vbo_;
    delete shader_;
}

void DirectBuffer::push() {
    MatrixStackEntry head = stack_.back();
    stack_.push_back(head);
}

void DirectBuffer::pop() {
    if (stack_.size() <= 1) {
        stack_[0].positionMatrix = glm::mat4(1.0);
        stack_[0].normalMatrix = glm::mat3(1.0);
        return;
    }

    stack_.pop_back();
}

void DirectBuffer::transform(glm::mat4 matrix) {
    MatrixStackEntry& head = stack_.back();
    head.positionMatrix = head.positionMatrix * matrix;
    head.normalMatrix = glm::inverse(glm::transpose(head.positionMatrix));
}

void DirectBuffer::stroke(float width) {
    stroke_ = width;
}

void DirectBuffer::shaded() {
    shaded_ = true;
}

void DirectBuffer::unshaded() {
    shaded_ = false;
}

void DirectBuffer::color(float r, float g, float b) {
    color_[0] = r;
    color_[1] = g;
    color_[2] = b;
}

void DirectBuffer::color(glm::vec3 c) {
    color(c[0], c[1], c[2]);
}

void DirectBuffer::light(glm::vec3 c) {
    float max = std::max(std::max(c[0], c[1]), c[2]);
    color(c[0] / max, c[1] / max, c[2] / max);
}

void DirectBuffer::vert(glm::vec3 pos) {
    glm::vec3 normal = {};
    MatrixStackEntry& head = stack_.back();
    if (shaded_) {
        normal = head.normalMatrix * normal_;
    }

    pos = head.positionMatrix * glm::vec4(pos, 1.0);
    data_.push_back(pos[0]);
    data_.push_back(pos[1]);
    data_.push_back(pos[2]);
    data_.push_back(color_[0]);
    data_.push_back(color_[1]);
    data_.push_back(color_[2]);
    data_.push_back(normal[0]);
    data_.push_back(normal[1]);
    data_.push_back(normal[2]);
}

void DirectBuffer::tri(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    if (shaded_ && autoShade_) {
        glm::vec3 ab = b - a;
        glm::vec3 ac = c - a;
        normal_ = glm::normalize(glm::cross(ab, ac));
    }
    vert(a);
    vert(c);
    vert(b);
}

void DirectBuffer::triLine(glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    line(a, b);
    line(b, c);
    line(c, a);
}

void DirectBuffer::quad(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
    tri(a, b, c);
    tri(d, c, b);
}

void DirectBuffer::quadLine(glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
    triLine(a, b, c);
    triLine(d, c, b);
}

void DirectBuffer::plane(glm::vec3 min, glm::vec3 max, glm::vec3 up) {
    up = glm::normalize(up);

    glm::vec3 diag = max - min;
    glm::vec3 cross = glm::cross(diag, up);
    normal_ = glm::normalize(glm::cross(cross, diag));

    glm::vec3 a = min;
    glm::vec3 b = min + (diag + cross) * 0.5f;
    glm::vec3 c = min + (diag - cross) * 0.5f;
    glm::vec3 d = max;

    quad(a, b, c, d);
}

void DirectBuffer::line(glm::vec3 a, glm::vec3 b) {
    glm::vec3 v = b - a;
    glm::vec3 scale = {glm::length(stack_.back().positionMatrix[0]), glm::length(stack_.back().positionMatrix[1]), glm::length(stack_.back().positionMatrix[2])};
    glm::vec3 normal = glm::normalize(perpendicular(v)) * stroke_ / scale;
    glm::vec3 bitangent = glm::normalize(glm::cross(normal, v)) * stroke_ / scale;
    glm::vec3 a0 = glm::vec3(a[0] + normal[0] / 2, a[1] + normal[1] / 2, a[2] + normal[2] / 2);
    glm::vec3 a1 = glm::vec3(a[0] - normal[0] / 2, a[1] - normal[1] / 2, a[2] - normal[2] / 2);
    glm::vec3 b0 = glm::vec3(b[0] + normal[0] / 2, b[1] + normal[1] / 2, b[2] + normal[2] / 2);
    glm::vec3 b1 = glm::vec3(b[0] - normal[0] / 2, b[1] - normal[1] / 2, b[2] - normal[2] / 2);
    quad(a0, b0, a1, b1);
    a0 = glm::vec3(a[0] + bitangent[0] / 2, a[1] + bitangent[1] / 2, a[2] + bitangent[2] / 2);
    a1 = glm::vec3(a[0] - bitangent[0] / 2, a[1] - bitangent[1] / 2, a[2] - bitangent[2] / 2);
    b0 = glm::vec3(b[0] + bitangent[0] / 2, b[1] + bitangent[1] / 2, b[2] + bitangent[2] / 2);
    b1 = glm::vec3(b[0] - bitangent[0] / 2, b[1] - bitangent[1] / 2, b[2] - bitangent[2] / 2);
    quad(a0, b0, a1, b1);
}

void DirectBuffer::axes(glm::mat4 transformation, float scale) {
    glm::vec3 center = transformation * glm::vec4(0, 0, 0, 1);
    color(1.0, 0.0, 0.0);
    line(center, transformation * glm::vec4(scale, 0, 0, 1));
    color(0.0, 1.0, 0.0);
    line(center, transformation * glm::vec4(0, scale, 0, 1));
    color(0.0, 0.0, 1.0);
    line(center, transformation * glm::vec4(0, 0, scale, 1));
}

void DirectBuffer::axes(glm::vec3 position, glm::mat3 orientation, float scale) {
    color(1.0, 0.0, 0.0);
    line(position, position + orientation * glm::vec3(scale, 0, 0));
    color(0.0, 1.0, 0.0);
    line(position, position + orientation * glm::vec3(0, scale, 0));
    color(0.0, 0.0, 1.0);
    line(position, position + orientation * glm::vec3(0, 0, scale));
}

void DirectBuffer::axes(glm::vec3 position, glm::quat orientation, float scale) {
    axes(position, glm::mat3_cast(orientation), scale);
}

void DirectBuffer::circleLine(glm::vec3 c, glm::vec3 n, float r) {
    regularPolyLine(c, n, r, circleSides(r));
}

void DirectBuffer::circle(glm::vec3 c, glm::vec3 n, float r) {
    regularPoly(c, n, r, circleSides(r));
}

void DirectBuffer::regularPolyLine(glm::vec3 c, glm::vec3 n, float r, int s) {
    normal_ = glm::normalize(n);
    float step = glm::two_pi<float>() / s;
    float r0 = r - stroke_ / 2;
    float r1 = r + stroke_ / 2;
    glm::mat3 rot = glm::rotate(glm::mat4(1.0), step, glm::normalize(n));
    glm::vec3 v0 = glm::normalize(perpendicular(n));
    for (int i = 0; i < s; i++) {
        glm::vec3 v1 = rot * v0;
        quad(c + v0 * r0, c + v0 * r1, c + v1 * r0, c + v1 * r1);
        v0 = v1;
    }
}

void DirectBuffer::regularPoly(glm::vec3 c, glm::vec3 n, float r, int s) {
    normal_ = glm::normalize(n);
    float step = glm::two_pi<float>() / s;
    glm::mat3 rot = glm::rotate(glm::mat4(1.0), step, glm::normalize(n));
    glm::vec3 v0 = glm::normalize(perpendicular(n));
    for (int i = 0; i < s; i++) {
        glm::vec3 v1 = rot * v0;
        tri(c, c + v0 * r, c + v1 * r);
        v0 = v1;
    }
}

void DirectBuffer::coneLine(glm::vec3 c, glm::vec3 d, float a) {
    float r = glm::length(d) * cos(a);
    regularPyramidLine(c, d, a, circleSides(r));
}

void DirectBuffer::regularPyramidLine(glm::vec3 c, glm::vec3 d, float a, int s) {
    float r = glm::length(d) * sin(a);
    float step = glm::two_pi<float>() / s;
    glm::mat3 rot = glm::rotate(glm::mat4(1.0), step, glm::normalize(d));
    glm::vec3 n = glm::normalize(perpendicular(d));
    for (int i = 0; i < s; i++) {
        normal_ = n;
        glm::vec3 v = d + n * r;
        line(c, c + v);
        n = rot * n;
    }
}

void DirectBuffer::uvSphere(glm::vec3 c, float r) {
    autoShade_ = true;
    int rings = circleSides(r) / 2;
    int segments = circleSides(r);

    double dTheta = glm::pi<float>() / rings;
    double dPhi = -glm::pi<float>() / segments;

    std::vector<glm::vec3> prevRing(segments);
    std::vector<glm::vec3> currRing(segments);

    for (int ring = 0; ring < rings + 1; ring++) {
        double theta = ring * dTheta;
        for (int segment = 0; segment < segments; segment++) {
            double phi = 2 * segment * dPhi;

            float x = r * static_cast<float>(glm::sin(theta) * glm::cos(phi));
            float y = r * static_cast<float>(glm::cos(theta));
            float z = r * static_cast<float>(glm::sin(theta) * glm::sin(phi));
            glm::vec3 v = c + glm::vec3(x, y, z);

            currRing[segment] = v;
            if (segment > 0) {
                if (ring > 0) {
                    quad(currRing[segment - 1], currRing[segment], prevRing[segment - 1], prevRing[segment]);
                }
            }
        }
        if (ring > 0) {
            quad(currRing[segments - 1], currRing[0], prevRing[segments - 1], prevRing[0]);
        }
        currRing.swap(prevRing);
    }
    autoShade_ = false;
}

void DirectBuffer::box(glm::vec3 c, glm::vec3 d) {
    glm::vec3 nxnynz = glm::vec3(c.x - 0.5 * d.x, c.y - 0.5 * d.y, c.z - 0.5 * d.z), pxpypz = glm::vec3(c.x + 0.5 * d.x, c.y + 0.5 * d.y, c.z + 0.5 * d.z);
    glm::vec3 pxnynz = glm::vec3(c.x + 0.5 * d.x, c.y - 0.5 * d.y, c.z - 0.5 * d.z), nxpypz = glm::vec3(c.x - 0.5 * d.x, c.y + 0.5 * d.y, c.z + 0.5 * d.z);
    glm::vec3 pxpynz = glm::vec3(c.x + 0.5 * d.x, c.y + 0.5 * d.y, c.z - 0.5 * d.z), nxnypz = glm::vec3(c.x - 0.5 * d.x, c.y - 0.5 * d.y, c.z + 0.5 * d.z);
    glm::vec3 pxnypz = glm::vec3(c.x + 0.5 * d.x, c.y - 0.5 * d.y, c.z + 0.5 * d.z), nxpynz = glm::vec3(c.x - 0.5 * d.x, c.y + 0.5 * d.y, c.z - 0.5 * d.z);
    normal_ = glm::vec3(0, 0, -1);  // -z
    quad(nxpynz, pxpynz, nxnynz, pxnynz);
    normal_ = glm::vec3(0, 0, 1);  // +z
    quad(nxpypz, pxpypz, nxnypz, pxnypz);
    normal_ = glm::vec3(-1, 0, 0);  // -x
    quad(nxpynz, nxpypz, nxnynz, nxnypz);
    normal_ = glm::vec3(1, 0, 0);  // +x
    quad(pxpynz, pxpypz, pxnynz, pxnypz);
    normal_ = glm::vec3(0, -1, 0);  // -y
    quad(nxnypz, pxnypz, nxnynz, pxnynz);
    normal_ = glm::vec3(0, 1, 0);  // +y
    quad(nxpypz, pxpypz, nxpynz, pxpynz);
}

void DirectBuffer::boxLine(glm::vec3 c, glm::vec3 d) {
    glm::vec3 nxnynz = glm::vec3(c.x - 0.5 * d.x, c.y - 0.5 * d.y, c.z - 0.5 * d.z), pxpypz = glm::vec3(c.x + 0.5 * d.x, c.y + 0.5 * d.y, c.z + 0.5 * d.z);
    glm::vec3 pxnynz = glm::vec3(c.x + 0.5 * d.x, c.y - 0.5 * d.y, c.z - 0.5 * d.z), nxpypz = glm::vec3(c.x - 0.5 * d.x, c.y + 0.5 * d.y, c.z + 0.5 * d.z);
    glm::vec3 pxpynz = glm::vec3(c.x + 0.5 * d.x, c.y + 0.5 * d.y, c.z - 0.5 * d.z), nxnypz = glm::vec3(c.x - 0.5 * d.x, c.y - 0.5 * d.y, c.z + 0.5 * d.z);
    glm::vec3 pxnypz = glm::vec3(c.x + 0.5 * d.x, c.y - 0.5 * d.y, c.z + 0.5 * d.z), nxpynz = glm::vec3(c.x - 0.5 * d.x, c.y + 0.5 * d.y, c.z - 0.5 * d.z);
    normal_ = glm::vec3(0, 0, -1);  // -z
    quadLine(nxpynz, pxpynz, nxnynz, pxnynz);
    normal_ = glm::vec3(0, 0, 1);  // +z
    quadLine(nxpypz, pxpypz, nxnypz, pxnypz);
    normal_ = glm::vec3(-1, 0, 0);  // -x
    quadLine(nxpynz, nxpypz, nxnynz, nxnypz);
    normal_ = glm::vec3(1, 0, 0);  // +x
    quadLine(pxpynz, pxpypz, pxnynz, pxnypz);
    normal_ = glm::vec3(0, -1, 0);  // -y
    quadLine(nxnypz, pxnypz, nxnynz, pxnynz);
    normal_ = glm::vec3(0, 1, 0);  // +y
    quadLine(nxpypz, pxpypz, nxpynz, pxpynz);
}

void DirectBuffer::render(glm::mat4 view_proj_mat, glm::vec3 camera_pos) {
    if (data_.empty()) {
        return;
    }

    gl::pushDebugGroup("DirectBuffer::render");

    size_t buffer_size = data_.size() * sizeof(float);
    if (vbo_->grow(buffer_size)) {
        vao_->reBindBuffer(0, *vbo_);
    }
    vbo_->write(0, data_.data(), buffer_size);

    vao_->bind();
    shader_->bind();
    shader_->vertexStage()->setUniform("u_view_projection_mat", view_proj_mat);
    shader_->fragmentStage()->setUniform("u_camera_position", camera_pos);
    gl::manager->setEnabled({gl::Capability::DepthTest, gl::Capability::PolygonOffsetFill});
    gl::manager->depthFunc(gl::DepthFunc::GreaterOrEqual);
    gl::manager->depthMask(true);
    gl::manager->polygonOffset(-1, -1);
    glDrawArrays(GL_TRIANGLES, 0, data_.size() / 9);

    clear();

    gl::popDebugGroup();
}

void DirectBuffer::clear() {
    data_.clear();
}