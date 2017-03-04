
#include "camera.hpp"
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include "debug.hpp"

namespace cd
{
    GLMCamera::GLMCamera()
    {
        m_angle_x = 0;
        m_angle_y = 0;
        m_position = glm::vec3(0,0,0);
        m_rotation = glm::mat4();// glm::eulerAngleYX(m_angle_y, m_angle_x);
        m_scale = glm::mat4();
        m_translate = glm::mat4();
        //m_transform = m_rotation * glm::translate(m_position);
        updateTransform();

        //GLdouble angle = M_PI/4;
        //GLdouble angle_degrees = 180 * angle / M_PI;
        //m_projection = glm::perspective(45.0, 1.0, 0.1, 10000.0);
        m_fov = 45.0f;
        m_aspect = 1.0f;
        updatePerspective();
    }
    /*
       void GLMCamera::setTransform(glm::mat4 & transform)
       {
       m_transform = transform;
       }
       void setPosition(glm::vec3 & pos)
       {
       m_position = pos;
       }
       void GLMCamera::setOrientation(
       glm::vec3 & dir,
       glm::Vec3 & up)
       {
       m_rotation = glm::orientation(dir,up);
       }
       */
    void GLMCamera::lookTowards(
            const glm::vec3 & pos,
            const glm::vec3 & dir)
    {
        glm::vec3 dir_n = glm::normalize(dir);
        m_position = pos;
        //glm::vec3 point = pos + dir;
        m_angle_x = std::acos(dir_n.y) - M_PI/2;
        m_angle_y = std::atan2(dir_n.x, -dir_n.z);
        //m_transform = glm::lookAt(pos,point,glm::vec3(0,1,0));
        updateTransform();
    }
    void GLMCamera::rotateLocal(
            const float angle_x,
            const float angle_y)
    {
        m_angle_x += angle_x;
        m_angle_y += angle_y;
        updateTransform();
    }
    void GLMCamera::translateLocal(
            float x,
            float y,
            float z)
    {
        glm::vec3 dir_global = glm::vec3(glm::transpose(m_rotation) * glm::vec4(x,y,z,1));
        m_position += dir_global;
        updateTransform();
    }
    void GLMCamera::setAspect(
            float w,
            float h)
    {
        //m_projection = glm::perspective(45.0f, w/h, 0.1f, 10000.0f);
        m_aspect = w/h;
        //m_projection = glm::infinitePerspective(m_fov, m_aspect, 0.1f);
        updatePerspective();
    }
    void GLMCamera::setOrtho(
            float mix, float max,
            float miy, float may,
            float miz, float maz)
    {
        //m_projection = glm::scale(glm::vec3(1,1,0.5)) * glm::translate(glm::vec3(0,0,-1)) * glm::ortho<float>(mix,max,miy,may,miz,maz);
        m_projection = glm::ortho<float>(mix,max,miy,may,miz,maz);
        // Do not update perspective
    }
    void GLMCamera::setPosX(float x)
    {
        m_position.x = x;
        updateTransform();
    }
    void GLMCamera::setPosY(float y)
    {
        m_position.y = y;
        updateTransform();
    }
    void GLMCamera::setPosZ(float z)
    {
        m_position.z = z;
        updateTransform();
    }
    void GLMCamera::setFOV(float fov)
    {
        m_fov = fov;
        //m_projection = glm::infinitePerspective(fov, m_aspect, 0.1f);
        updatePerspective();
    }
    void GLMCamera::setAngleX(float angle_x)
    {
        m_angle_x = angle_x;
        updateTransform();
    }
    void GLMCamera::setAngleY(float angle_y)
    {
        m_angle_y = angle_y;
        updateTransform();
    }
    void GLMCamera::setAngles(
            float angle_x,
            float angle_y)
    {
        m_angle_x = angle_x;
        m_angle_y = angle_y;
        updateTransform();
    }
    void GLMCamera::setScale(float sx, float sy, float sz)
    {
        m_scale = glm::scale(glm::vec3(sx,sy,sz));
        updateTransform();
    }
    void GLMCamera::setTranslate(float x, float y, float z)
    {
        m_translate = glm::translate(glm::vec3(x,y,z));
        updateTransform();
    }
    void GLMCamera::updateTransform()
    {
        //m_rotation = glm::eulerAngleYX(m_angle_y, m_angle_x);
        m_rotation = glm::rotate(m_angle_x, glm::vec3(1,0,0));
        m_rotation *= glm::rotate(m_angle_y, glm::vec3(0,1,0));
        m_transform = m_rotation * glm::translate(-m_position) * m_translate * m_scale;
        m_mvp = m_projection * m_transform;
        //std::cout << glm::to_string(m_transform) << std::endl;
    }
    void GLMCamera::updatePerspective()
    {
        m_projection = glm::infinitePerspective(m_fov, m_aspect, 0.1f);
        //m_projection = glm::perspective(m_fov,m_aspect,0.1f,1000.0f);
        //m_projection = glm::perspectiveFov(m_fov,m_aspect,m_aspect,0.1f,100000.0f);
    }
    bool GLMCamera::canSee(const glm::vec3 & point)
    {
        glm::vec4 proj_point = m_projection * m_transform * glm::vec4(point,1.0f);
        proj_point /= proj_point.w;
        if( -1 <= proj_point.x and proj_point.x <= 1)
            if( -1 <= proj_point.y and proj_point.y <= 1 )
                if( 0 <= proj_point.z and proj_point.z <= 1 )
                    return true;
        return false;
    }   

    bool GLMCamera::canSee(const glm::vec4 points[8]) 
    {
        for(GLuint k = 0; k < 8; ++k)
        {
            glm::vec4 proj_vtx = m_mvp * points[k]; 
            m_z_buffer[k] = proj_vtx.z;
            m_point_clip[k] = proj_vtx / proj_vtx.w;
        }

        GLint side_x, side_y, side_z;
        side_x = side_y = side_z = 0;
        if( m_point_clip[0].x < -1 )
            side_x = -1;
        else if( m_point_clip[0].x > 1 )
            side_x = 1;

        if( m_point_clip[0].y < -1 )
            side_y = -1;
        else if( m_point_clip[0].y > 1 )
            side_y = 1;

        if( m_z_buffer[0] < 0 )
            side_z = -1;
        else if( m_z_buffer[0] > 1 )
            side_z = 1;

        GLint side_xx, side_yy, side_zz;
        GLboolean single_side_x = true;
        GLboolean single_side_y = true;
        GLboolean single_side_z = true;
        for(GLuint k = 1; k < 8; ++k)
        {

            side_xx = side_yy = side_zz = 0;

            if( m_point_clip[k].x < -1 )
                side_xx = -1;
            else if( m_point_clip[k].x > 1 )
                side_xx = 1;

            if( m_point_clip[k].y < -1 )
                side_yy = -1;
            else if( m_point_clip[k].y > 1 )
                side_yy = 1;

            if( m_z_buffer[k] < 0 )
                side_zz = -1;
            else if( m_z_buffer[k] > 1 )
                side_zz = 1;

            if( side_xx != side_x ) 
                single_side_x = false;
            if( side_yy != side_y )
                single_side_y = false;
            if( side_zz != side_z)
                single_side_z = false;

            // early break
            if( !(single_side_x or single_side_y or single_side_z) )
                break;

            /*
               insuff_info = insuff_info or (side_xx != side_x) or (side_yy != side_y);
               if( insuff_info )
               break;
               */
        }

        if( single_side_x )
            if( side_x != 0 )
                return false;
        if( single_side_y )
            if( side_y != 0 )
                return false;
        if( single_side_z )
            if( side_z != 1)
                return false;
        return true;
    }

    /*
     * Some geometry
     */

    struct Line
    {
        glm::vec2 start;
        glm::vec2 dir;
    };
    // assumes box is attached to origin
    bool pointInBox(glm::vec2 & point, std::vector<glm::vec2> & box)
    {
        glm::vec2 e0 = box[0]-box[3];
        glm::vec2 e1 = box[1]-box[2];
        glm::vec2 e2 = box[2]-box[1];
        glm::vec2 e3 = box[0]-box[1];

        glm::vec3 l0 = glm::vec3(glm::normalize(e0), - glm::length(box[0]));
        glm::vec3 l1 = glm::vec3(glm::normalize(e1), - glm::length(box[1]));
        glm::vec3 l2 = glm::vec3(glm::normalize(e2), - glm::length(box[3]));
        glm::vec3 l3 = glm::vec3(glm::normalize(e3), - glm::length(box[0]));

        glm::vec3 pos(point,1);
        GLfloat d0 = glm::dot(l0,pos);
        GLfloat d1 = glm::dot(l1,pos);
        GLfloat d2 = glm::dot(l2,pos);
        GLfloat d3 = glm::dot(l3,pos);

        /*
           VERBOSE(glm::to_string(pos));
           VERBOSE(glm::to_string(l0));
           VERBOSE(d0);
           VERBOSE(d1);
           VERBOSE(d2);
           VERBOSE(d3);
           */

        if( d0 <= 0 and d1 <= 0 and d2 <= 0 and d3 <= 0 )
            return true;
        return false;
    }
    bool lineCutsLine(Line & l1, Line & l2)
    {
        glm::vec2 pos_diff = l2.start - l1.start;
        glm::vec2 v0(0,0);
        glm::vec2 v1(l1.dir);
        glm::vec2 v2(l1.dir-l2.dir);
        glm::vec2 v3(-l2.dir);
        std::vector<glm::vec2> box(4);
        box[0] = v0;
        box[1] = v1;
        box[2] = v2;
        box[3] = v3;

        /*
           VERBOSE(glm::to_string(l1.start)); VERBOSE(glm::to_string(l1.dir));
           VERBOSE(glm::to_string(l2.start)); VERBOSE(glm::to_string(l2.dir));
           VERBOSE(pointInBox(pos_diff, box));
           */
        return pointInBox(pos_diff, box);
    }

    bool GLMCamera::boxCutsFrustum(std::vector<glm::vec3> & bounding_box)
    {
        std::vector<glm::vec2> bb(bounding_box.size());
        std::vector<bool> enabled(bb.size(),true);
        for(GLuint i = 0; i < bb.size(); ++i)
        {
            glm::vec4 proj_vtx = m_projection * m_transform * glm::vec4(bounding_box[i],1);
            //VERBOSE(glm::to_string(proj_vtx));
            if( proj_vtx.z >= 0 )
                enabled[i] = true; 
            proj_vtx /= proj_vtx.w;
            bb[i] = glm::vec2(proj_vtx.x, proj_vtx.y);
        } 
        /*
           VERBOSE(enabled[0]);
           VERBOSE(enabled[1]);
           VERBOSE(enabled[2]);
           VERBOSE(enabled[3]);
           VERBOSE(enabled[4]);
           VERBOSE(enabled[5]);
           VERBOSE(enabled[6]);
           VERBOSE(enabled[7]);
           */

        std::vector<Line> lines(4);
        lines[0].start = glm::vec2(-1,-1);
        lines[0].dir = glm::vec2(0,1);
        lines[1].start = glm::vec2(-1,-1);
        lines[1].dir = glm::vec2(1,0);
        lines[2].start = glm::vec2(1,1);
        lines[2].dir = glm::vec2(0,-1);
        lines[3].start = glm::vec2(1,1);
        lines[3].dir = glm::vec2(-1,0);

        for(GLuint i = 0; i < bb.size(); ++i)
        {
            for(GLuint j = i+1; j < bb.size(); ++j)
            {
                if( not enabled[i] or not enabled[j] )
                    continue;
                Line box_line;
                box_line.start = bb[i];
                box_line.dir = bb[j]-bb[i];

                for(GLuint k = 0; k < lines.size(); ++k)
                    if( lineCutsLine(box_line, lines[k]) )
                        return true;
            } 
        }
        return false;
    }

    glm::vec3 GLMCamera::getPos()
    {
        return m_position;
    }
    float GLMCamera::getAngleX()
    {
        return m_angle_x;
    }
    float GLMCamera::getAngleY()
    {
        return m_angle_y;
    }

    void GLMCamera::bindShader(
            GLuint shader,
            const std::string & projection_name,
            const std::string & transform_name,
            const std::string & mvp_name,
            const std::string & normal_name)
    {
        glUseProgram(shader);
        if( ! projection_name.empty() )
            glUniformMatrix4fv(
                    glGetUniformLocation(shader, projection_name.c_str()),
                    1, GL_FALSE, &m_projection[0][0]);
        if( ! transform_name.empty() )
            glUniformMatrix4fv(
                    glGetUniformLocation(shader, transform_name.c_str()),
                    1, GL_FALSE, &m_transform[0][0]);
        if( ! mvp_name.empty() )
            glUniformMatrix4fv(
                    glGetUniformLocation(shader, mvp_name.c_str()),
                    1, GL_FALSE, &m_mvp[0][0]);
        if( ! normal_name.empty() )
        {
            //glm::mat3 normal_matrix = glm::transpose(glm::inverse(glm::mat3(m_transform)));
            glm::mat3 normal_matrix = glm::mat3(m_transform); // Assume non shearing
            glUniformMatrix3fv(
                    glGetUniformLocation(shader, normal_name.c_str()),
                    1, GL_FALSE, &normal_matrix[0][0]);
        }
    }
    /*
    void GLMCamera::bindShaderOptimized(
            GLuint shader,
            const std::string & mvp_name,
            const std::string & normal_name)
    {
        if( ! mvp_name.empty() )
            glUniformMatrix4fv(
                    glGetUniformLocation(shader, mvp_name.c_str()),
                    1, GL_FALSE, &m_mvp[0][0]);
        if( ! normal_name.empty() )
        {
            glm::mat3 normal_matrix = glm::mat3(m_transform);
            glUniformMatrix4fv(
                    glGetUniformLocation(shader, normal_name.c_str()),
                    1, GL_FALSE, &normal_matrix[0][0]);
        }

    }
    */

    /////////////////////////////////

    Camera::Camera() 
    {
        pos = SetVector(0,5,30);
        vel = SetVector(0,0,0);
        speed = 10.0f;
        pitch = 0;
        yaw = 0;
        rotSpeed = 0.05f;
        rotThresh = 0.01f;
        transform = IdentityMatrix();
        projection = perspective(45, 1.0f, 0.2f, 1500.0f);
    }


    mat4 matFromAngles(float pitch, float yaw)
    {
        mat4 rotx = Rx(pitch);
        mat4 roty = Ry(yaw);
        mat4 rot = Mult(rotx, roty);
        return rot; 
    }

    void Camera::update() 
    {
        pos = VectorAdd(pos, vel);
        pitch += pitchSpeed;
        yaw += yawSpeed;
        mat4 R = matFromAngles(pitch, yaw);
        mat4 n = T( - pos.x, - pos.y, - pos.z); // obs : negative
        transform = Mult(R, n);
    }

    void Camera::setRotateVel(float v_pitch, float v_yaw)
    { 
        v_pitch *= rotSpeed;
        v_yaw *= rotSpeed;
        if(v_pitch < 0 && pitch <= (-M_PI/2+0.05)) 
            pitchSpeed = 0;
        else if(v_pitch > 0 && pitch >= (M_PI/2+0.05)) 
            pitchSpeed = 0;
        else if( fabs(v_pitch) > rotThresh )
            pitchSpeed = v_pitch;
        else
            pitchSpeed = 0;
        if( fabs(v_yaw) > rotThresh )
            yawSpeed = v_yaw;
        else
            yawSpeed = 0;
    }

    void Camera::setMoveVel(float vx, float vy, float vz)
    {
        vx *= speed;
        vy *= speed;
        vz *= speed;
        vec4 moveVecLocal = vec3tovec4(SetVector(vx,vy,vz));
        mat4 baseMat = Transpose(matFromAngles(pitch, yaw));
        vec4 moveVecGlobal = MultVec4(baseMat, moveVecLocal);
        //moveVecGlobal.y = 0; // inte flyga
        vel = vec4tovec3(moveVecGlobal);
    }

    void Camera::setTargetY(float targetY)
    {
        //float alpha = 0.5;
        //pos.y *= (1.0 - alpha);
        //pos.y += alpha * targetY;
        pos.y = targetY;
    }

}
