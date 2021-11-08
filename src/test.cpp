/***************************************************
 * 定数
****************************************************/
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

/***************************************************
 * ライブラリとかもろもろ
****************************************************/
#include <stdio.h>
#include <fstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glut.h>

#include "./glsl.h"　                               //これだけは床井さんのサイトからもらってます
#include "./math/vector/vector2f.h"                 //Unityっぽくしたかった

/***************************************************
 * ファイル名
****************************************************/
const std::string NAME = "./title.png";
const std::string VSNAME = "./src/guiShader.vert";
const std::string FSNAME = "./src/guiShader.frag";

/***************************************************
 * シェーダ用変数
****************************************************/
GLuint programID;
GLuint vertexShaderID;
GLuint fragmentShaderID;

/***************************************************
 * テクスチャ用変数
****************************************************/
GLuint texId;
Vector2f textureSize;

/***************************************************
 * vao,vbo用変数
****************************************************/
GLuint vao;
GLuint vboVertex;
GLuint vboUv;
GLuint vboIndex;

/***************************************************
 * SDL_Rectっぽくしたかったから無理やり作った
****************************************************/
class GuiRect
{
  private:
  public:
    Vector2f pos;
    Vector2f size;
    operator float *() { return (float *)pos; }
    operator const float *() const { return (const float *)pos; }
    GuiRect() {}
    GuiRect(Vector2f pos, Vector2f size)
    {
        this->pos = pos;
        this->size = size;
    }
    GuiRect(float px, float py, float sx, float sy)
    {
        this->pos = Vector2f(px, py);
        this->size = Vector2f(sx, sy);
    }
};

/***************************************************
 * 描画用の関数
 * SDLっぽくかけます
 * 明るさやz軸を含めた補正ができます
****************************************************/
void draw(GuiRect *srcRect, GuiRect *dstRect, float brightness, Vector3f pos)
{
    if (brightness < 0)
    {
        brightness = 0.0;
    }
    if (brightness > 1)
    {
        brightness = 1.0f;
    }

    GLint loc = glGetUniformLocation(programID, "brightness");
    glUniform1f(loc, brightness);

    glBindTexture(GL_TEXTURE_2D, texId);
    glBindVertexArray(vao);

    if (srcRect == NULL)
    {
        srcRect = new GuiRect(0, 0, textureSize.x, textureSize.y);
    }

    if (dstRect == NULL)
    {
        dstRect = new GuiRect(-WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2, WINDOW_WIDTH, WINDOW_HEIGHT);
    }

    const GLfloat vertexData[] = {
        dstRect->pos.x + pos.x, dstRect->pos.y + pos.y, pos.z,
        dstRect->pos.x + dstRect->size.x + pos.x, dstRect->pos.y + pos.y, pos.z,
        dstRect->pos.x + pos.x, dstRect->pos.y - dstRect->size.y + pos.y, pos.z,
        dstRect->pos.x + dstRect->size.x + pos.x, dstRect->pos.y - dstRect->size.y + pos.y, pos.z};

    const GLfloat uvData[] = {
        srcRect->pos.x / textureSize.x, srcRect->pos.y / textureSize.y,
        (srcRect->pos.x + srcRect->size.x) / textureSize.x, srcRect->pos.y / textureSize.y,
        srcRect->pos.x / textureSize.x, (srcRect->pos.y + srcRect->size.y) / textureSize.y,
        (srcRect->pos.x + srcRect->size.x) / textureSize.x, (srcRect->pos.y + srcRect->size.y) / textureSize.y};

    glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertexData), vertexData);
    glBindBuffer(GL_ARRAY_BUFFER, vboUv);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(uvData), uvData);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

/***************************************************
 * シェーダの読み込みからコンパイルまでやります
 * 戻り地としてシェーダに割り振られたIDが帰ってきます
****************************************************/
GLuint loadShader(const char *file, int type)
{
    GLuint ShaderID = glCreateShader(type);

    //読み込み
    if (readShaderSource(ShaderID, file))
    {
        exit(1);
    }
    //コンパイル
    GLint compiled;
    glCompileShader(ShaderID);
    glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &compiled);
    printShaderInfoLog(ShaderID);
    if (compiled == GL_FALSE)
    {
        fprintf(stderr, "Error --> glCompileShader()\n");
    }

    return ShaderID;
}
/***************************************************
 * Main関数
****************************************************/
int main(int argc, char *argv[])
{
    /***************************************************
     * SDL初期化
    ****************************************************/
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK))
    {
        fprintf(stderr, "Error --> SDL_Init()\n");
        return false;
    }
    /***************************************************
     * window初期化
    ****************************************************/
    SDL_Window *window;
    SDL_GLContext glcontext;
    SDL_Renderer *renderer;
    
    window = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
    glcontext = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(1);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    if (window == NULL)
    {
        fprintf(stderr, "Error --> SDL_CreateWindow()\n");
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (renderer == NULL)
    {
        fprintf(stderr, "Error --> SDL_CreateRenderer()\n");
        return false;
    }

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    glClearColor(1.0, 1.0, 1.0, 0.0);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glAlphaFunc(GL_GREATER, 0.5);
    glEnable(GL_ALPHA_TEST);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    /***************************************************
     * shader読み込み
    ****************************************************/
    vertexShaderID = loadShader(VSNAME.c_str(), GL_VERTEX_SHADER);
    fragmentShaderID = loadShader(FSNAME.c_str(), GL_FRAGMENT_SHADER);
    programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    
    
    //（本当はここでAttlibuteを指定する必要があるが今回は省略）
    
    
    GLint linked;
    glLinkProgram(programID);
    glGetProgramiv(programID, GL_LINK_STATUS, &linked);
    printProgramInfoLog(programID);
    glValidateProgram(programID);
    /***************************************************
     * 画像を読み込む→テクスチャを作る
    ****************************************************/
    SDL_Surface* surface = IMG_Load(NAME.c_str());
    if (surface == NULL)
    {
        fprintf(stderr, "Error --> loadTextureFile(%s)\n", NAME.c_str());
        return -1;
    }

    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->w, surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, surface->pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    textureSize = Vector2f(surface->w, surface->h);
    SDL_FreeSurface(surface);

    /***************************************************
     * 四角形がかけるようなvao vboを用意する
    ****************************************************/

    const GLuint indexData[] = {
        0, 2, 3,
        0, 3, 1};
    const GLfloat vertexData[] = {
        0.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f};
    const GLfloat uvData[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f};

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vboIndex);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIndex);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indexData), indexData, GL_STATIC_DRAW);

    glGenBuffers(1, &vboVertex);
    glBindBuffer(GL_ARRAY_BUFFER, vboVertex);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glGenBuffers(1, &vboUv);
    glBindBuffer(GL_ARRAY_BUFFER, vboUv);
    glBufferData(GL_ARRAY_BUFFER, sizeof(uvData), uvData, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

    glBindVertexArray(0);

    /***************************************************
     * メインloop
    ****************************************************/
    while(1){
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glPushMatrix();
        glLoadIdentity();

        float bright = 1.0;
        GuiRect dst = GuiRect(-393, 256, 786, 255);
        
        glUseProgram(programID);
        draw(NULL, &dst, bright, Vector3f_ZERO);
        glUseProgram(0);
        
        glPopMatrix();
        glFlush();
        SDL_GL_SwapWindow(window);
    }
    return 0;
}