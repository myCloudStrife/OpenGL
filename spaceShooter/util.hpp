#ifndef SPACE_UTIL_HPP_
#define SPACE_UTIL_HPP_

#include <random>

#define GL_COMPRESSED_RGB_S3TC_DXT1_EXT 0x83F0
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3

inline double rand(double low, double high) {
    return (double) rand() / RAND_MAX * (high - low) + low;
}

GLuint load_DDS(const char *path) {
    unsigned char header[124];
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "%s could not be opened.\n", path);
        return 0;
    }
    unsigned signature;
    fread(&signature, sizeof(signature), 1, f);
    if (signature != 0x20534444) { // "DDS " signature
        fclose(f);
        fprintf(stderr, "%s has not dds format.\n", path);
        return 0;
    }
    fread(header, sizeof(header), 1, f);
    unsigned height = *(unsigned *) (header + 8);
    unsigned width = *(unsigned *) (header + 12);
    unsigned linearSize = *(unsigned *) (header + 16);
    unsigned mipMapCount = *(unsigned *) (header + 24);
    unsigned fourCC = *(unsigned *) (header + 80);

    unsigned bufsize = mipMapCount > 1 ? linearSize * 2 : linearSize;
    unsigned char *buffer = new unsigned char [bufsize];
    fread(buffer, 1, bufsize, f);
    fclose(f);

    unsigned format, blockSize;
    switch(fourCC) {
    case 0x31545844:    // Equivalent to "DXT1" in ASCII
        format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
        blockSize = 8;
        break;
    case 0x33545844:    // Equivalent to "DXT3" in ASCII
        format = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
        blockSize = 16;
        break;
    case 0x35545844:    // Equivalent to "DXT5" in ASCII
        format = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
        blockSize = 16;
        break;
    default:
        delete [] buffer;
        fprintf(stderr, "load_DDS: format error\n");
        return 0;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    unsigned offset = 0;
    for (unsigned level = 0; level < mipMapCount; ++level) {
        unsigned size = ((width + 3) / 4) * ((height + 3) / 4) * blockSize;
        glCompressedTexImage2D(GL_TEXTURE_2D, level, format, width, height,
                0, size, buffer + offset);
        offset += size;
        width  /= 2;
        height /= 2;
        if(width < 1) {
            width = 1;
        }
        if(height < 1) {
            height = 1;
        }
    }

    delete [] buffer;
    return textureID;
}

#endif /* SPACE_UTIL_HPP_ */
