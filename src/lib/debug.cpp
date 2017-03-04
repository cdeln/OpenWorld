
#include "debug.hpp"


namespace cd
{
    GLenum checkFBO()
    {
        GLenum status;
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);                                                
        switch(status)
        {   
            case GL_FRAMEBUFFER_COMPLETE: std::cout << "FBO ok" << std::endl; break;                      
            case GL_FRAMEBUFFER_UNDEFINED: FBO_PRINT(GL_FRAME_BUFFER_UNDEFINED); break;                   
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT); break;     
            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT); break;
            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER); break;   
            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER); break;
            case GL_FRAMEBUFFER_UNSUPPORTED: FBO_PRINT(GL_FRAMEBUFFER_UNSUPPORTED); break;                
            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE); break;
            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS : FBO_PRINT(GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS); break;
            default: FBO_PRINT(Unknown error);                                                            
        }   
        std::cout << "FBO status = " << status << std::endl;                                              
        return status;                                                                                    
    }
}
