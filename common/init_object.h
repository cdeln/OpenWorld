#ifndef __INIT_OBJECT__
#define __INIT_OBJECT__

#include "MicroGlut.h"
#include "VectorUtils3.h"
#include "loadobj.h"

void init_object(GLuint vertexArrayObjID, Model * model, GLuint program){

    // VBO for vertex data

        glUseProgram(program);

        glBindVertexArray(vertexArrayObjID);

        GLuint vertexBufferObjID;
        GLuint normalBufferObjID;
        GLuint indexBufferObjID;
        GLuint texBufferObjID;
        // Allocate Vertex Buffer Objects (VBO)
        glGenBuffers(1, &vertexBufferObjID);
        glGenBuffers(1, &normalBufferObjID);
        glGenBuffers(1, &indexBufferObjID);
        glGenBuffers(1, &texBufferObjID);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObjID);
        glBufferData(GL_ARRAY_BUFFER, 3 * model->numVertices * sizeof(GLfloat), model->vertexArray, GL_STATIC_DRAW);
        glVertexAttribPointer(glGetAttribLocation(program, "in_Position"), 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(glGetAttribLocation(program, "in_Position"));

        printError("vertex");
        // VBO for index data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferObjID);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->numIndices * sizeof(GLuint), model->indexArray, GL_STATIC_DRAW);

        printError("index");

           // VBO for normal data
        glBindBuffer(GL_ARRAY_BUFFER, normalBufferObjID); printError("0");
        glBufferData(GL_ARRAY_BUFFER, 3 * model->numVertices * sizeof(GLfloat), model->normalArray, GL_STATIC_DRAW);  printError("1");
        glVertexAttribPointer(glGetAttribLocation(program, "in_Normal"), 3, GL_FLOAT, GL_FALSE, 0, 0); printError("2");
        glEnableVertexAttribArray(glGetAttribLocation(program, "in_Normal")); printError("3");

        printError("normal");
        // VBO tex data
        
        if (model->texCoordArray != NULL)
        {
            glBindBuffer(GL_ARRAY_BUFFER, texBufferObjID);
            printError("1");
            glBufferData(GL_ARRAY_BUFFER, model->numVertices*2*sizeof(GLfloat), model->texCoordArray, GL_STATIC_DRAW);
            printError("2");
            glVertexAttribPointer(glGetAttribLocation(program, "in_TexCoord"), 2, GL_FLOAT, GL_FALSE, 0, 0);
            printError("3");
            glEnableVertexAttribArray(glGetAttribLocation(program, "in_TexCoord"));
            printError("4");
        }
        else {
            printf("no texture coordinates found\n");
        }
        printError("tex");
}

#endif