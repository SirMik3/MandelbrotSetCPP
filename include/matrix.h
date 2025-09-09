#pragma once

#include <cmath>

// Simple 4x4 matrix class for OpenGL transformations
class Mat4 {
public:
    float m[16]; // Column-major order (OpenGL standard)
    
    Mat4() {
        // Initialize as identity matrix
        for (int i = 0; i < 16; i++) {
            m[i] = 0.0f;
        }
        m[0] = m[5] = m[10] = m[15] = 1.0f;
    }
    
    // Create perspective projection matrix
    static Mat4 perspective(float fov, float aspect, float near, float far) {
        Mat4 result;
        float tanHalfFov = tan(fov / 2.0f);
        
        result.m[0] = 1.0f / (aspect * tanHalfFov);
        result.m[5] = 1.0f / tanHalfFov;
        result.m[10] = -(far + near) / (far - near);
        result.m[11] = -1.0f;
        result.m[14] = -(2.0f * far * near) / (far - near);
        result.m[15] = 0.0f;
        
        return result;
    }
    
    // Create look-at view matrix
    static Mat4 lookAt(float eyeX, float eyeY, float eyeZ,
                       float centerX, float centerY, float centerZ,
                       float upX, float upY, float upZ) {
        // Calculate forward vector
        float fx = centerX - eyeX;
        float fy = centerY - eyeY;
        float fz = centerZ - eyeZ;
        
        // Normalize forward
        float flen = sqrt(fx*fx + fy*fy + fz*fz);
        fx /= flen; fy /= flen; fz /= flen;
        
        // Calculate right vector (forward x up)
        float rx = fy * upZ - fz * upY;
        float ry = fz * upX - fx * upZ;
        float rz = fx * upY - fy * upX;
        
        // Normalize right
        float rlen = sqrt(rx*rx + ry*ry + rz*rz);
        rx /= rlen; ry /= rlen; rz /= rlen;
        
        // Calculate up vector (right x forward)
        float ux = ry * fz - rz * fy;
        float uy = rz * fx - rx * fz;
        float uz = rx * fy - ry * fx;
        
        Mat4 result;
        result.m[0] = rx;  result.m[4] = ux;  result.m[8] = -fx;  result.m[12] = -(rx*eyeX + ux*eyeY - fx*eyeZ);
        result.m[1] = ry;  result.m[5] = uy;  result.m[9] = -fy;  result.m[13] = -(ry*eyeX + uy*eyeY - fy*eyeZ);
        result.m[2] = rz;  result.m[6] = uz;  result.m[10] = -fz; result.m[14] = -(rz*eyeX + uz*eyeY - fz*eyeZ);
        result.m[3] = 0;   result.m[7] = 0;   result.m[11] = 0;   result.m[15] = 1;
        
        return result;
    }
    
    // Create rotation matrix around Y axis
    static Mat4 rotateY(float angle) {
        Mat4 result;
        float c = cos(angle);
        float s = sin(angle);
        
        result.m[0] = c;   result.m[8] = s;
        result.m[2] = -s;  result.m[10] = c;
        
        return result;
    }
    
    // Create rotation matrix around X axis
    static Mat4 rotateX(float angle) {
        Mat4 result;
        float c = cos(angle);
        float s = sin(angle);
        
        result.m[5] = c;   result.m[6] = -s;
        result.m[9] = s;   result.m[10] = c;
        
        return result;
    }
    
    // Matrix multiplication
    Mat4 operator*(const Mat4& other) const {
        Mat4 result;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                result.m[i*4 + j] = 0;
                for (int k = 0; k < 4; k++) {
                    result.m[i*4 + j] += m[k*4 + j] * other.m[i*4 + k];
                }
            }
        }
        return result;
    }
};
