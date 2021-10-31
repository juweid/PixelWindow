
#include <assert.h>

#include <glad/gl.h>

#include "PixelWindow/PixelWindow.h"



namespace sr {

    const float vertices[12] = {
        -1.0f,  1.0f, // topLeft
        1.0f,  1.0f, // topRight
        -1.0f, -1.0f, // bottomLeft
        -1.0f, -1.0f, // bottomLeft
        1.0f,  1.0f, // topRight
        1.0f, -1.0f  // bottomRight
    };

    const float texCoords[12] = {
         0.0f,  1.0f,
         1.0f,  1.0f,
         0.0f,  0.0f,
         0.0f,  0.0f,
         1.0f,  1.0f,
         1.0f,  0.0f
    };

    const char* vertexShader =
        "#version 420\n"
        "layout(location = 0) in vec2 vp;"
        "layout(location = 1) in vec2 texin;"
        "out vec2 texcoord;"
        "void main(){"
        "   gl_Position = vec4(vp, 0.0, 1.0);"
        "   texcoord = texin;"
        "}";

    const char* fragmentShader =
        "#version 420\n"
        "in vec2 texcoord;"
        "out vec4 fragout;"
        "layout(binding = 0) uniform sampler2D basic_texture;"
        "void main(){"
        "   fragout = texture(basic_texture, texcoord);"
        "}";


	PixelWindow::PixelWindow(int width, int height, const char* title) : Window(width, height, title), 
		width(width), height(height)
	{
        int nPixels = this->width * this->height;
        int dataSize = nPixels * 4;
        this->pixelBuffer = new int[nPixels];
		
        // Resize Callback
        this->addResizeCallback([this](int width, int height) { 
            this->width = width;
            this->height = height;
            glViewport(0, 0, width, height);
        });
        
        this->addRefreshCallback([this]() {
            int nPixels = this->width * this->height;
            int dataSize = nPixels * 4;

            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[0]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_STREAM_DRAW);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[1]);
            glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_STREAM_DRAW);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

            glBindTexture(GL_TEXTURE_2D, this->textureId);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[pboIndex]);
            
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, this->width, this->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
        });

        // Buffer vertex vertices
        glGenBuffers(1, &this->vertVbo);
        glBindBuffer(GL_ARRAY_BUFFER, vertVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // Buffer texture coordinates
        glGenBuffers(1, &this->texVbo);
        glBindBuffer(GL_ARRAY_BUFFER, this->texVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);

        // Create VAO
        glGenVertexArrays(1, &this->vao);
        glBindVertexArray(this->vao);

        glBindBuffer(GL_ARRAY_BUFFER, this->vertVbo);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        glBindBuffer(GL_ARRAY_BUFFER, this->texVbo);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);

        // Shader creation
        this->vs = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(this->vs, 1, &vertexShader, NULL);
        glCompileShader(this->vs);

        this->fs = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(this->fs, 1, &fragmentShader, NULL);
        glCompileShader(this->fs);

        this->shader = glCreateProgram();
        glAttachShader(this->shader, this->vs);
        glAttachShader(this->shader, this->fs);
        glLinkProgram(this->shader);

        // Texture creation
        glGenTextures(1, &this->textureId);
        glBindTexture(GL_TEXTURE_2D, this->textureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, this->pixelBuffer);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Pbo Creation
        glGenBuffers(2, pbos);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[0]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos[1]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_STREAM_DRAW);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
	}


	void PixelWindow::setBackgroundColor(int color) {
        assert(this->pixelBuffer != nullptr);
        int nPixles = this->width * this->height;
        
        for (int i = 0; i < nPixles; ++i) this->pixelBuffer[i] = color;
	}

    void PixelWindow::setPixel(int x, int y, int color) {
        assert(this->pixelBuffer != nullptr);
        assert(!(x < 0 || x >= this->width || y < 0 || y >= this->height));
        this->pixelBuffer[this->width * y + x] = color;
    }

	void PixelWindow::beginFrame() {
        int dataSize = this->width * this->height * 4;

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, this->pbos[this->pboIndex]);
        glBufferData(GL_PIXEL_UNPACK_BUFFER, dataSize, nullptr, GL_STREAM_DRAW);
        this->pixelBuffer = reinterpret_cast<int*>(glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY));

        this->pboIndex = (this->pboIndex + 1) % 2;
	}

	void PixelWindow::endFrame() {
        glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);

        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(this->shader);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->textureId);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, this->pbos[this->pboIndex]);

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->width, this->height, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glBindVertexArray(vao);

        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(0);

        glBindVertexArray(0);

        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glActiveTexture(0);

        glUseProgram(0);

        this->swapBuffers();
	}

    int PixelWindow::getWidth() const noexcept {
        return this->width;
    }

    int PixelWindow::getHeight() const noexcept {
        return this->height;
    }

}