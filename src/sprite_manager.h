#ifndef SPRITE_MANAGER_H
#define SPRITE_MANAGER_H

#include "basic_transform.h"
#include "texture_atlas.h"
#include "vector_util.h"
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#define GL_SILENCE_DEPRECATION  // silences macOS OpenGL deprecation warnings

#define glCheck(expr)                                                                              \
    expr;                                                                                          \
    if (int e = glGetError()) {                                                                    \
        std::cerr << "OpenGL error: " << e << " in file " __FILE__ << ", line number " << __LINE__ \
                  << '\n';                                                                         \
        abort();                                                                                   \
    }

using SpriteID = size_t;

class SpriteManager {
public:
    SpriteManager(const sf::Vector2u& atlasSize, unsigned int padding)
        : mVertices(sf::PrimitiveType::Triangles),
          mNumSprites{0},
          mAtlas(atlasSize, padding),
          mShader(std::filesystem::path{"./src/main.vert"}, std::filesystem::path{"./src/main.frag"}) {
        // TODO
        GLuint program = mShader.getNativeHandle();

        mPositionAttribLocation = glCheck(glGetAttribLocation(program, "position"));
        mTexCoordsAttribLocation = glCheck(glGetAttribLocation(program, "texCoords"));
        mSpriteNumberAttribLocation = glCheck(glGetAttribLocation(program, "spriteNumber"));

        mTextureUniformLocation = glCheck(glGetUniformLocation(program, "texture"));
        mTransformsUniformLocation = glCheck(glGetUniformLocation(program, "transforms"));
        mNumTransformsUniformLocation = glCheck(glGetUniformLocation(program, "numTransforms"));

        glCheck(glGenBuffers(1, &mVerticesVBO));
        glCheck(glGenBuffers(1, &mSpriteNumberVBO));
        glCheck(glGenTextures(1, &mTransformsTexture));
        glCheck(glGenVertexArrays(1, &mVAO));
    }

    ~SpriteManager()
    {
        glDeleteBuffers(1, &mVerticesVBO);
        glDeleteBuffers(1, &mSpriteNumberVBO);
        glDeleteTextures(1, &mTransformsTexture);
        glDeleteVertexArrays(1, &mVAO);
    }

    // TODO
    TextureID registerTexture(const std::string& filepath)
    {
        return { mAtlas.registerTexture(filepath) };
    };

    // TODO
    SpriteID createSprite(TextureID textureID)
    {
        sf::FloatRect uvRect = mAtlas.getBoundsUV(textureID);
        sf::Vector2u atlasSize = mAtlas.getSizePixels(textureID);
        float width = static_cast<float>(atlasSize.x);
        float height = static_cast<float>(atlasSize.y);

        sf::Vector2f quadBL{0.0f, 0.0f};
        sf::Vector2f quadBR{width, 0.0f};
        sf::Vector2f quadTL{0.0f, height};
        sf::Vector2f quadTR{width, height};

        sf::Vector2f uvBL{uvRect.position.x, uvRect.position.y + uvRect.size.y};
        sf::Vector2f uvBR{uvRect.position.x + uvRect.size.x, uvRect.position.y + uvRect.size.y};
        sf::Vector2f uvTR{uvRect.position.x + uvRect.size.x, uvRect.position.y};
        sf::Vector2f uvTL{uvRect.position.x, uvRect.position.y};

        mVertices.append(sf::Vertex{quadBL, sf::Color::White, uvBL});
        mVertices.append(sf::Vertex{quadBR, sf::Color::White, uvBR});
        mVertices.append(sf::Vertex{quadTR, sf::Color::White, uvTR});

        mVertices.append(sf::Vertex{quadBL, sf::Color::White, uvBL});
        mVertices.append(sf::Vertex{quadTR, sf::Color::White, uvTR});
        mVertices.append(sf::Vertex{quadTL, sf::Color::White, uvTL});

        mTransforms.emplace_back();
        SpriteID id = mNumSprites++;

        for (int i = 0; i < 6; i++)
        {
            mSpriteNumbers.push_back(static_cast<float>(id));
        }

        return {id};
    };

    // TODO
    void setTexture(SpriteID sprite, TextureID newTexture)
    {
        sf::FloatRect uvRect = mAtlas.getBoundsUV(newTexture);
        sf::Vector2u atlasSize = mAtlas.getSizePixels(newTexture);
        float width = static_cast<float>(atlasSize.x);
        float height = static_cast<float>(atlasSize.y);

        sf::Vector2f quadBL{0.0f, 0.0f};
        sf::Vector2f quadBR{width, 0.0f};
        sf::Vector2f quadTL{0.0f, height};
        sf::Vector2f quadTR{width, height};

        sf::Vector2f uvBL{uvRect.position.x, uvRect.position.y + uvRect.size.y};
        sf::Vector2f uvBR{uvRect.position.x + uvRect.size.x, uvRect.position.y + uvRect.size.y};
        sf::Vector2f uvTR{uvRect.position.x + uvRect.size.x, uvRect.position.y};
        sf::Vector2f uvTL{uvRect.position.x, uvRect.position.y};

        size_t index = sprite * 6;

        mVertices[index + 0] = sf::Vertex{quadBL, sf::Color::White, uvBL};
        mVertices[index + 1] = sf::Vertex{quadBR, sf::Color::White, uvBR};
        mVertices[index + 2] = sf::Vertex{quadTR, sf::Color::White, uvTR};
        mVertices[index + 3] = sf::Vertex{quadBL, sf::Color::White, uvBL};
        mVertices[index + 4] = sf::Vertex{quadTR, sf::Color::White, uvTR};
        mVertices[index + 5] = sf::Vertex{quadTL, sf::Color::White, uvTL};
    };

    // TODO
    void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        sf::VertexArray tempArray = mVertices;

        for (size_t i = 0; i < mNumSprites; i++)
        {
            const BasicTransform transform = mTransforms[i];
            size_t index = i * 6;
            for (size_t x = 0; x < 6; x++)
            {
                tempArray[index + x].position = transform.applyPoint(tempArray[index + x].position);
            }
        }

        states.texture = &mAtlas.getTexture();
        states.shader = &mShader;

        mShader.setUniform("texture", sf::Shader::CurrentTexture);
        target.draw(tempArray, states);
    };

    // TODO
    void drawGL()
    {
        if (mNumSprites == 0)
        {
            return;
        }

        GLuint program = mShader.getNativeHandle();
        glCheck(glUseProgram(program));
        glCheck(glBindVertexArray(mVAO));

        glCheck(glBindBuffer(GL_ARRAY_BUFFER, mVerticesVBO));
        glCheck(glBufferData(GL_ARRAY_BUFFER, mVertices.getVertexCount() * sizeof(sf::Vertex), &mVertices[0], GL_DYNAMIC_DRAW));
        glCheck(glEnableVertexAttribArray(mPositionAttribLocation));
        glCheck(glVertexAttribPointer(mPositionAttribLocation, 2, GL_FLOAT, GL_FALSE, sizeof(sf::Vertex), reinterpret_cast<void*>(offsetof(sf::Vertex, position))));
        glCheck(glEnableVertexAttribArray(mTexCoordsAttribLocation));
        glCheck(glVertexAttribPointer(mTexCoordsAttribLocation, 2, GL_FLOAT, GL_FALSE, sizeof(sf::Vertex), reinterpret_cast<void*>(offsetof(sf::Vertex, texCoords))));

        glCheck(glBindBuffer(GL_ARRAY_BUFFER, mSpriteNumberVBO));
        glCheck(glBufferData(GL_ARRAY_BUFFER, mSpriteNumbers.size() * sizeof(float), mSpriteNumbers.data(), GL_DYNAMIC_DRAW));
        glCheck(glEnableVertexAttribArray(mSpriteNumberAttribLocation));
        glCheck(glVertexAttribPointer(mSpriteNumberAttribLocation, 1, GL_FLOAT, GL_FALSE, 0, nullptr));

        glCheck(glActiveTexture(GL_TEXTURE0));
        sf::Texture::bind(&mAtlas.getTexture());
        glCheck(glUniform1i(mTextureUniformLocation, 0));

        glCheck(glActiveTexture(GL_TEXTURE1));
        glCheck(glBindTexture(GL_TEXTURE_1D, mTransformsTexture));
        glCheck(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        glCheck(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        glCheck(glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));

        static_assert(sizeof(BasicTransform) == 4 * sizeof(float), "Must be vec4 layout");
        glCheck(glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA32F_ARB, static_cast<GLsizei>(mTransforms.size()), 0, GL_RGBA, GL_FLOAT, mTransforms.data()));
        glCheck(glUniform1i(mTransformsUniformLocation, 1));

        glCheck(glUniform1i(mNumTransformsUniformLocation, static_cast<GLint>(mTransforms.size())));
        glCheck(glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mVertices.getVertexCount())));

        glCheck(glDisableVertexAttribArray(mPositionAttribLocation));
        glCheck(glDisableVertexAttribArray(mTexCoordsAttribLocation));
        glCheck(glDisableVertexAttribArray(mSpriteNumberAttribLocation));
        glCheck(glBindBuffer(GL_ARRAY_BUFFER, 0));
        glCheck(glBindTexture(GL_TEXTURE_1D, 0));
        glCheck(glActiveTexture(GL_TEXTURE0));
        glCheck(glBindTexture(GL_TEXTURE_2D, 0));
        glCheck(glBindVertexArray(0));
        glCheck(glUseProgram(0));
    };

    // TODO
    BasicTransform& getTransform(SpriteID id)
    {
        return mTransforms[id];
    }

    // TODO
    const TextureAtlas& getAtlas() const
    {
        return mAtlas;
    };

private:
    sf::VertexArray mVertices;
    std::vector<BasicTransform> mTransforms;
    size_t mNumSprites;
    TextureAtlas mAtlas;
    mutable sf::Shader mShader;

    // Use the below only in Batched rendering (part 2):
    std::vector<float> mSpriteNumbers;
    GLuint mVAO;

    GLint mTextureUniformLocation;
    GLint mTransformsUniformLocation;
    GLint mNumTransformsUniformLocation;

    GLint mPositionAttribLocation;
    GLint mTexCoordsAttribLocation;
    GLint mSpriteNumberAttribLocation;

    GLuint mVerticesVBO;
    GLuint mSpriteNumberVBO;
    GLuint mTransformsTexture;
};

#endif  // SPRITE_MANAGER_H
