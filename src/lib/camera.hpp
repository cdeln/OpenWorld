#ifndef __CAMERA__
#define __CAMERA__

#include "VectorUtils3.h"
#include <glm/glm.hpp>
//#include <glm/vec3.hpp>
//#include <glm/vec4.hpp>
//#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>

namespace cd
{

    class GLMCamera
    {
        public:
            GLMCamera();
            /*
            void setTransform(glm::mat4 & transform);
            void setOrientation(
                    glm::vec3 & dir,
                    glm::Vec3 & up);
            void setPosition(glm::vec3 & pos);
            */
            //void setViewDirection(glm::vec3 & dir);
            //void setUpDirection(glm::vec3 & up);
            void rotateLocal(
                    float angle_x,
                    float angle_y);
            void translateLocal(
                    float x,
                    float y,
                    float z); 
            void lookTowards(
                    const glm::vec3 & pos,
                    const glm::vec3 & dir);
            void setAspect(
                    float w,
                    float h);
            void setOrtho(
                    float mix, float max,
                    float miy, float may,
                    float miz, float maz);
            void setPosX(float x);
            void setPosY(float y);
            void setPosZ(float z);
            void setFOV(float fov);
            void setAngleX(float angle_x);
            void setAngleY(float angle_y);
            void setAngles(
                    float angle_x, 
                    float angle_y);
            void setScale(float sx, float sy, float sz);
            void setTranslate(float x, float y, float z);
            void updateTransform();
            void updatePerspective();
            bool canSee(const glm::vec3 & point);
            bool canSee(const glm::vec4 points[8]);
            bool boxCutsFrustum(std::vector<glm::vec3> & bb); 
            void bindShader(
                    GLuint shader,
                    const std::string & projection_name = "",
                    const std::string & transform_name = "",
                    const std::string & mvp_name = "",
                    const std::string & normal_name = "");
            /*
            void bindShaderOptimized(
                    GLuint shader,
                    const std::string & mvp_name,
                    const std::string & normal_name);
            */
            glm::vec3 getPos();
            float getAngleX();
            float getAngleY();

        private:
            float m_angle_x;
            float m_angle_y;
            float m_aspect;
            float m_fov;
            glm::vec3 m_position;
            glm::mat4 m_rotation;
            glm::mat4 m_scale;
            glm::mat4 m_translate;
            glm::mat4 m_transform;
            glm::mat4 m_projection;
            glm::mat4 m_mvp;
            glm::vec4 m_point_clip[8];
            GLfloat m_z_buffer[8];
            //std::vector<glm::vec4> m_frustum_planes;
    };

    struct Camera
    {
        // fields
        vec3 pos, vel;
        float speed;
        float pitch, yaw, roll;
        float pitchSpeed, yawSpeed;
        float rotSpeed;
        float rotThresh;
        mat4 transform;
        mat4 projection;

        // members
        Camera();
        void update();
        void setRotateVel(float v_pitch, float v_yaw);
        void setMoveVel(float vx, float vy, float vz);    
        void setTargetY(float targetY);
    };

}

#endif
