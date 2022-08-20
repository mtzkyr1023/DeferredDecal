#ifndef _SH_CALCULATOR_H_
#define _SH_CALCULATOR_H_




//-----------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------

#include <fstream>

#include "sh_calculator.h"
#include "stb_image.h"

///----------------------------------------------------------------------
///<summary>
///6����BMP�t�@�C������L���[�u�}�b�v�f�[�^�𐶐�.
///</summary>
///<param name="filename1">���ʂ̃e�N�X�`���t�@�C����</param>
///<param name="filename2">�E�ʂ̃e�N�X�`���t�@�C����</param>
///<param name="filename3">�O�ʂ̃e�N�X�`���t�@�C����</param>
///<param name="filename4">��ʂ̃e�N�X�`���t�@�C����</param>
///<param name="filename5">�V�ʂ̃e�N�X�`���t�@�C����</param>
///<param name="filename6">��ʂ̃e�N�X�`���t�@�C����</param>
///<return>�����ɐ���������true��ԋp</return>
///----------------------------------------------------------------------
bool SHCalculator::CreateCubeDataFromBMP
(
    const char* filename1,
    const char* filename2,
    const char* filename3,
    const char* filename4,
    const char* filename5,
    const char* filename6,
    CubeData* pCube
)
{
    bool result = true;

    unsigned char* image;
    const char* fileTable[NUM_CUBE_FACE] =
    {
        filename1,
        filename2,
        filename3,
        filename4,
        filename5,
        filename6
    };

    for (unsigned int f = 0; f < NUM_CUBE_FACE; f++)
    {
        int width, height, bpp;
        image = stbi_load(fileTable[f], &width, &height, &bpp, 4);
        if (image == nullptr)
        {
            assert(false);
            result = false;
            break;
        }
        //if (width != CUBE_SIZE)
        //{
        //    assert(false);
        //    result = false;
        //    break;
        //}
        else
        {
            int idx = 0;
            for (int i = 0; i < height; i++) {
                for (int j = 0; j < width; j += 4) {
                    float inv = 1.0f / 127.0f;
                    pCube->r[f][idx] = static_cast<float>(image[width * i + j + 0]) * inv;
                    pCube->g[f][idx] = static_cast<float>(image[width * i + j + 1]) * inv;
                    pCube->b[f][idx] = static_cast<float>(image[width * i + j + 2]) * inv;

                    idx++;
                }
            }
            stbi_image_free(image);
        }
    }

    return result;
}

///----------------------------------------------------------------------
///<summary>
///�e�L�X�g�t�@�C������SH�W���𐶐�
///</summary>
///<param name="filename">SH�W�����i�[����Ă���e�L�X�g�t�@�C����</param>
///<param name="coeff">SH�W�����i�[����ϐ�</param>
///<return>�����ɐ���������true��ԋp</return>
///----------------------------------------------------------------------
bool CreateSHCoefficientFromTXT(const char* filename, SHCoeff& coeff)
{
    bool result = false;

    std::ifstream file(filename);

    if (file.is_open())
    {
        result = true;
        for (unsigned int idx = 0; idx < NUM_COEFF; idx++)
        {
            float r, g, b;
            file >> r >> g >> b;
            coeff.r[idx] = r;
            coeff.g[idx] = g;
            coeff.b[idx] = b;
            //DLOG( "Read Data = ( %f, %f, %f )", coeff.r[idx], coeff.g[idx], coeff.b[idx] );

        }
        file.close();
    }
    else
    {
        assert(false);
    }
    return result;
}


///----------------------------------------------------------------------
///<summary>
///SH�W�����e�L�X�g�t�@�C���ɏo�͂���
///</summary>
///<param name="coeff">�o�͂���SH�W��</param>
///<param name="filename">�o�̓e�L�X�g�t�@�C����</param>
///<return>�o�͂ɐ���������true��ԋp����</param>
///----------------------------------------------------------------------
bool SaveSHCoefficientToTXT(const SHCoeff& coeff, const char* filename)
{
    bool result = false;

    std::ofstream file(filename);
    file.setf(std::ios::fixed);
    if (file.is_open())
    {
        result = true;
        file.precision(6);
        for (unsigned int i = 0; i < NUM_COEFF; i++)
        {
            file << coeff.r[i] << " " << coeff.g[i] << " " << coeff.b[i] << std::endl;
        }
        file.close();
    }
    return result;
}


/////////////////////////////////////////////////////////////////////////
// SHCalculator class
/////////////////////////////////////////////////////////////////////////

///----------------------------------------------------------------------
///<summary>
///�R���X�g���N�^
///</summary>
///----------------------------------------------------------------------
SHCalculator::SHCalculator()
{
    // �X�^�b�N�ɂ̂�����Ȃ�����...
    for (int i = 0; i < NUM_CUBE_FACE; i++)
    {
        mpCubeSHTable[i] = new SHTable36[CUBE_SIZE * CUBE_SIZE];
    }
}

///----------------------------------------------------------------------
///<summary>
///�f�X�g���N�^
///</summary>
///----------------------------------------------------------------------
SHCalculator::~SHCalculator()
{
    for (int i = 0; i < NUM_CUBE_FACE; i++)
    {
        if (mpCubeSHTable[i])
        {
            delete mpCubeSHTable[i];
            mpCubeSHTable[i] = 0;
        }
    }
}

///----------------------------------------------------------------------
///<summary>
///�B��̃C���X�^���X���擾����
///</summary>
///<return>�B��̃C���X�^���X��ԋp����</return>
///----------------------------------------------------------------------
SHCalculator* SHCalculator::GetInstance()
{
    static SHCalculator instance;
    return &instance;
}

///----------------------------------------------------------------------
///<summary>
///���ʒ��a�֐����v�Z����
///</summary>
///<param name="l">���̐������w�肷��</param>
///<param name="m">-l����l�܂ł̐������w�肷��</param>
///<param name="theta">�p�x��</param>
///<param name="phi">�p�x��</param>
///<returns>SH���֐����v�Z�������ʂ�ԋp����</returns>
///----------------------------------------------------------------------
float SHCalculator::Compute(int l, int m, float theta, float phi)
{
    assert(l >= 0);
    assert(-l <= m && m <= l);

    float result = ScalingFactor(l, m);

    if (0 < m)
    {
        result *= sqrtf(2.0f) * cosf(m * phi) * LegendrePolynomials(l, m, cosf(theta));
    }
    else if (m < 0)
    {
        result *= sqrtf(2.0f) * sinf(-m * phi) * LegendrePolynomials(l, -m, cosf(theta));
    }
    else
    {
        result *= LegendrePolynomials(l, m, cosf(theta));
    }

    return result;
}

///----------------------------------------------------------------------
///<summary>
///���K�����邽�߂̃X�P�[�����O�W�������߂�
///</summary>
///<param name="l">���̐������w��</param>
///<param name="m">-l����l�܂ł̐������w��</param>
///<returns>�v�Z�����l��ԋp����</returns>
///----------------------------------------------------------------------
float SHCalculator::ScalingFactor(int l, int m)
{
    float lpm = 1.0f;
    float lnm = 1.0f;

    if (m < 0) { m = -m; }

    for (int i = (l - m); 0 < i; i--) { lnm *= i; }
    for (int i = (l + m); 0 < i; i--) { lpm *= i; }

    return sqrtf(((2.0f * l + 1.0f) * lnm) / (4.0f * glm::pi<float>() * lpm));
}

///----------------------------------------------------------------------
///<summary>
///���W�����h�����֐����v�Z����
///</summary>
///<param name="l"></param>
///<param name="m"></param>
///<param name="x"></param>
///<returns>�v�Z�����l��ԋp����</returns>
///----------------------------------------------------------------------
float SHCalculator::LegendrePolynomials(int l, int m, float x)
{
    float pmm = 1.0f;

    if (0 < m)
    {
        float somx2 = sqrtf((1.0f - x) * (1.0f + x));
        float fact = 1.0f;
        for (int i = 1; i <= m; i++)
        {
            pmm *= (-fact) * somx2;
            fact += 2.0f;
        }
    }
    if (l == m) { return pmm; }

    float pmmp1 = x * (2.0f * m + 1.0f) * pmm;
    if (l == (m + 1)) { return pmmp1; }

    float pll = 0.0f;

    for (int ll = (m + 2); ll <= l; ll++)
    {
        pll = ((2.0f * ll - 1.0f) * x * pmmp1 - (ll + m - 1.0f) * pmm) / (ll - m);
        pmm = pmmp1;
        pmmp1 = pll;
    }

    return pll;
}

///----------------------------------------------------------------------
///<summary>
///SH�e�[�u���̏��������s��.
///</summary>
///----------------------------------------------------------------------
void SHCalculator::InitSHTable()
{
    // ���̊p���Z�o
    ComputeSolidAngle(mDeltaFormFactor);

    // SH�e�[�u���̏�����
    glm::vec3 vec;
    SHTable36 sh;

    for (unsigned int v = 0; v < CUBE_SIZE; v++)
    {
        for (unsigned int u = 0; u < CUBE_SIZE; u++)
        {
            // left 
            {
                vec.x = -1.0f;
                vec.z = -(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.y = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec = glm::normalize(vec);

                Compute36(vec, sh);
                for (unsigned int i = 0; i < NUM_COEFF; ++i)
                {
                    mpCubeSHTable[CUBE_FACE_LEFT][v * CUBE_SIZE + u].value[i] = sh.value[i];
                }
            }

            // right
            {
                vec.x = +1.0f;
                vec.z = +(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.y = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec = glm::normalize(vec);

                Compute36(vec, sh);
                for (unsigned int i = 0; i < NUM_COEFF; ++i)
                {
                    mpCubeSHTable[CUBE_FACE_RIGHT][v * CUBE_SIZE + u].value[i] = sh.value[i];
                }
            }

            // front 
            {
                vec.z = -1.0f;
                vec.x = +(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.y = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec = glm::normalize(vec);

                Compute36(vec, sh);
                for (unsigned int i = 0; i < NUM_COEFF; ++i)
                {
                    mpCubeSHTable[CUBE_FACE_FRONT][v * CUBE_SIZE + u].value[i] = sh.value[i];
                }
            }

            // back
            {
                vec.z = +1.0f;
                vec.x = -(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.y = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec = glm::normalize(vec);

                Compute36(vec, sh);
                for (unsigned int i = 0; i < NUM_COEFF; ++i)
                {
                    mpCubeSHTable[CUBE_FACE_BACK][v * CUBE_SIZE + u].value[i] = sh.value[i];
                }
            }

            // top
            {
                vec.y = -1.0f;
                vec.x = +(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.z = +(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec = glm::normalize(vec);

                Compute36(vec, sh);
                for (unsigned int i = 0; i < NUM_COEFF; ++i)
                {
                    mpCubeSHTable[CUBE_FACE_TOP][v * CUBE_SIZE + u].value[i] = sh.value[i];
                }
            }

            // bottom
            {
                vec.y = +1.0f;
                vec.x = +(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.z = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec = glm::normalize(vec);

                Compute36(vec, sh);
                for (unsigned int i = 0; i < NUM_COEFF; ++i)
                {
                    mpCubeSHTable[CUBE_FACE_BOTTOM][v * CUBE_SIZE + u].value[i] = sh.value[i];
                }
            }
        }
    }
}

///----------------------------------------------------------------------
///<summary>
///���̊p���Z�o����
///</summary>
///<param name="deltaFormFactor">�Z�o�������̊p���i�[����ϐ�</param>
///----------------------------------------------------------------------
void SHCalculator::ComputeSolidAngle(SolidAngleTable& deltaFormFactor)
{
    glm::vec3 vec;
    const float dS = (2.0f * 2.0f) / static_cast<float>(CUBE_SIZE * CUBE_SIZE);

    for (unsigned int v = 0; v < CUBE_SIZE; v++)
    {
        for (unsigned int u = 0; u < CUBE_SIZE; u++)
        {
            // left
            {
                vec.x = -1.0f;
                vec.z = -(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.y = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);

                float mag = glm::length(vec) * glm::pow(glm::length(vec), 2.0f);
                float d = 1.0f / mag;
                deltaFormFactor.value[CUBE_FACE_LEFT][v * CUBE_SIZE + u] = d * dS;
            }

            // right 
            {
                vec.x = +1.0f;
                vec.z = +(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.y = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);

                float mag = glm::length(vec) * glm::pow(glm::length(vec), 2.0f);
                float d = 1.0f / mag;
                deltaFormFactor.value[CUBE_FACE_RIGHT][v * CUBE_SIZE + u] = d * dS;
            }

            // front 
            {
                vec.z = -1.0f;
                vec.x = +(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.y = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);

                float mag = glm::length(vec) * glm::pow(glm::length(vec), 2.0f);
                float d = 1.0f / mag;
                deltaFormFactor.value[CUBE_FACE_FRONT][v * CUBE_SIZE + u] = d * dS;
            }

            // back 
            {
                vec.z = +1.0f;
                vec.x = -(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.y = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);

                float mag = glm::length(vec) * glm::pow(glm::length(vec), 2.0f);
                float d = 1.0f / mag;
                deltaFormFactor.value[CUBE_FACE_BACK][v * CUBE_SIZE + u] = d * dS;
            }

            // top
            {
                vec.y = -1.0f;
                vec.x = +(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.z = +(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);

                float mag = glm::length(vec) * glm::pow(glm::length(vec), 2.0f);
                float d = 1.0f / mag;
                deltaFormFactor.value[CUBE_FACE_TOP][v * CUBE_SIZE + u] = d * dS;
            }

            // bottom
            {
                vec.y = +1.0f;
                vec.x = +(2.0f * ((static_cast<float>(u) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);
                vec.z = -(2.0f * ((static_cast<float>(v) + 0.5f) / static_cast<float>(CUBE_SIZE)) - 1.0f);

                float mag = glm::length(vec) * glm::pow(glm::length(vec), 2.0f);
                float d = 1.0f / mag;
                deltaFormFactor.value[CUBE_FACE_BOTTOM][v * CUBE_SIZE + u] = d * dS;
            }

        }
    }
}

///----------------------------------------------------------------------
///<summary>
///5���܂�(=36��)�̋��ʒ��a�֐��W�J���s��.
///</summary>
///<param name="vec">�����x�N�g��</param>
///<param name="sh">�v�Z���ʂ��i�[����ϐ�</param>
///----------------------------------------------------------------------
void SHCalculator::Compute36(const glm::vec3& vec, SHTable36& sh)
{
    // ���O�v�Z�ŋ��߂Ă������W������
    SHTable36 c;
    c.value[0] = 0.282095f;          // Y{0,  0}

    c.value[1] = -0.488603f;          // Y{1, -1}
    c.value[2] = 0.488603f;          // Y{1,  0}
    c.value[3] = -0.488603f;          // Y{1,  1}

    c.value[4] = 1.092548f;          // Y{2, -2}
    c.value[5] = -1.092548f;          // Y{2, -1}
    c.value[6] = 0.315392f;          // Y{2,  0}
    c.value[7] = -1.092548f;          // Y{2,  1}
    c.value[8] = 0.546274f;          // Y{2,  2}

    c.value[9] = -0.590044f;          // Y{3, -3}
    c.value[10] = 2.89061f;           // Y{3, -2}
    c.value[11] = -0.457046f;          // Y{3, -1}
    c.value[12] = 0.373176f;          // Y{3,  0}
    c.value[13] = -0.457046f;          // Y{3,  1}
    c.value[14] = 1.44531f;           // Y{3,  2}
    c.value[15] = -0.590044f;          // Y{3,  3}

    c.value[16] = 2.503343f;          // Y{4, -4}
    c.value[17] = -1.770131f;          // Y{4, -3}
    c.value[18] = 0.946175f;          // Y{4, -2}
    c.value[19] = -0.669047f;          // Y{4, -1}
    c.value[20] = 0.105786f;          // Y{4,  0}
    c.value[21] = -0.669047f;          // Y{4,  1}
    c.value[22] = 0.473087f;          // Y{4,  2}
    c.value[23] = -1.770131f;          // Y{4,  3}
    c.value[24] = 0.625836f;          // Y{4,  4}

    c.value[25] = -0.656383f;          // Y{5, -5}
    c.value[26] = 8.302649f;          // Y{5, -4}
    c.value[27] = -0.489238f;          // Y{5, -3}
    c.value[28] = 4.793537f;          // Y{5, -2}
    c.value[29] = -0.452947f;          // Y{5, -1}
    c.value[30] = 0.116950f;          // Y{5,  0}
    c.value[31] = -0.452947f;          // Y{5,  1}
    c.value[32] = 2.396768f;          // Y{5,  2}
    c.value[33] = -0.489238f;          // Y{5,  3}
    c.value[34] = 2.075662f;          // Y{5,  4}
    c.value[35] = -0.656383f;          // Y{5,  5}

    // Y{0,0} 
    sh.value[0] = c.value[0];

    // Y{1,-1}, Y{1,0}, Y{1,1}
    sh.value[1] = c.value[1] * vec.y;
    sh.value[2] = c.value[2] * vec.z;
    sh.value[3] = c.value[3] * vec.x;

    // Y{2, -2}, Y{2,-1}, Y{2,1}
    sh.value[4] = c.value[4] * vec.x * vec.y;
    sh.value[5] = c.value[5] * vec.y * vec.z;
    sh.value[7] = c.value[7] * vec.x * vec.z;

    // Y{2,0} 
    sh.value[6] = c.value[6] * (3.0f * vec.z * vec.z - 1.0f);

    // Y{2,2} 
    sh.value[8] = c.value[8] * (vec.x * vec.x - vec.y * vec.y);

    // Y{3, -3} = A * sqrt(5/8) * (3 * x^2 * y - y^3)
    sh.value[9] = c.value[9] * (3.0f * vec.x * vec.x * vec.y - vec.y * vec.y * vec.y);

    // Y{3, -2} = A * sqrt(15) * x * y * z 
    sh.value[10] = c.value[10] * vec.x * vec.y * vec.z;

    // Y{3, -1} = A * sqrt(3/8) * y * (5 * z^2 - 1)
    sh.value[11] = c.value[11] * vec.y * (5.0f * vec.z * vec.z - 1.0f);

    // Y{3,  0} = A * (1/2) * (5 * z^3 - 3 *z)	
    sh.value[12] = c.value[12] * (5.0f * vec.z * vec.z * vec.z - 3.0f * vec.z);

    // Y{3,  1} = A * sqrt(3/8) * x * (5 * z^2 - 1)
    sh.value[13] = c.value[13] * vec.x * (5.0f * vec.z * vec.z - 1.0f);

    // Y{3,  2} = A * sqrt(15/4) * z *(x^2 - y^2)
    sh.value[14] = c.value[14] * vec.z * (vec.x * vec.x - vec.y * vec.y);

    // Y{3,  3} = A * sqrt(5/8) * (x^3 - 3 * x * y^2)
    sh.value[15] = c.value[15] * (vec.x * vec.x * vec.x - 3.0f * vec.x * vec.y * vec.y);

    float x2 = vec.x * vec.x;
    float y2 = vec.y * vec.y;
    float z2 = vec.z * vec.z;
    float x4 = x2 * x2;
    float y4 = y2 * y2;
    float z4 = z2 * z2;
    sh.value[16] = c.value[16] * vec.y * vec.x * (x2 - y2);               // 4, -4
    sh.value[17] = c.value[17] * vec.y * (3.0f * x2 - y2) * vec.z;        // 4, -3
    sh.value[18] = c.value[18] * vec.y * vec.x * (-1.0f + 7.0f * z2);     // 4, -2
    sh.value[19] = c.value[19] * vec.y * vec.z * (-3.0f + 7.0f * z2);     // 4, -1
    sh.value[20] = c.value[20] * (35.0f * z4 - 30.0f * z2 + 3.0f);        // 4, 0
    sh.value[21] = c.value[21] * vec.x * vec.z * (-3.0f + 7.0f * z2);     // 4, 1
    sh.value[22] = c.value[22] * (x2 - y2) * (-1.0f + 7.0f * z2);       // 4, 2
    sh.value[23] = c.value[23] * vec.x * (x2 - 3.0f * y2) * vec.z;        // 4, 3
    sh.value[24] = c.value[24] * (x4 - 6.0f * y2 * x2 + y4);              // 4, 4

    sh.value[25] = c.value[25] * vec.y * (5.0f * x4 - 10.0f * y2 * x2 + y4);          // 5, -5
    sh.value[26] = c.value[26] * vec.y * vec.x * (x2 - y2) * vec.z;                   // 5, -4
    sh.value[27] = c.value[27] * vec.y * (3.0f * x2 - y2) * (-1.0f + 9.0f * z2);    // 5, -3
    sh.value[28] = c.value[28] * vec.y * vec.x * vec.z * (-1.0f + 3.0f * z2);         // 5, -2
    sh.value[29] = c.value[29] * vec.y * (-14.0f * z2 + 21.0f * z4 + 1.0f);           // 5, -1
    sh.value[30] = c.value[30] * vec.z * (63.0f * z4 - 70.0f * z2 + 15.0f);           // 5, 0
    sh.value[31] = c.value[31] * vec.x * (-14.0f * z2 + 21.0f * z4 + 1.0f);           // 5, 1
    sh.value[32] = c.value[32] * (x2 - y2) * vec.z * (-1.0f + 3.0f * z2);           // 5, 2
    sh.value[33] = c.value[33] * vec.x * (x2 - 3.0f * y2) * (-1.0f + 9.0f * z2);    // 5, 3
    sh.value[34] = c.value[34] * (x4 - 6.0f * y2 * x2 + y4) * vec.z;                  // 5, 4
    sh.value[35] = c.value[35] * vec.x * (x4 - 10.0f * y2 * x2 + 5.0f * y4);          // 5, 5

}

///----------------------------------------------------------------------
///<summary>
///�L���[�u�}�b�v��SH�W�������߂�.
///(�L���[�u�f�[�^�����k����j
///</summary>
///<param name="pCube">���k����L���[�u�f�[�^</param>
///<param name="coeff">SH�W�����i�[����ϐ�</param>
///----------------------------------------------------------------------
void SHCalculator::Compress(const CubeData* pCube, SHCoeff& coeff)
{
    InitSHTable();

    unsigned int cnt = 0;
    for (unsigned int l = 0; l < NUM_COEFF; ++l)
    {
        double r = 0.0;
        double g = 0.0;
        double b = 0.0;

        for (unsigned int f = 0; f < NUM_CUBE_FACE; ++f)
        {
            for (unsigned int p = 0; p < CUBE_SIZE * CUBE_SIZE; ++p)
            {
                double value = (static_cast<double>(mDeltaFormFactor.value[f][p]) * static_cast<double>(mpCubeSHTable[f][p].value[l]));
                r += (static_cast<double>(pCube->r[f][p]) * value);
                g += (static_cast<double>(pCube->g[f][p]) * value);
                b += (static_cast<double>(pCube->b[f][p]) * value);
            }
        }

        coeff.r[cnt] = static_cast<float>(r);
        coeff.g[cnt] = static_cast<float>(g);
        coeff.b[cnt] = static_cast<float>(b);

        cnt++;
    }
}

///----------------------------------------------------------------------
///<summary>
///SH�W������L���[�u�f�[�^�𐶐�����.
///(�L���[�u�f�[�^���𓀂���).
///</summary>
///<param name="coeff">SH�W��</param>
///<param name="pCube">�L���[�u�f�[�^���i�[����ϐ��ւ̃|�C���^</param>
///----------------------------------------------------------------------
void SHCalculator::Decompress(const SHCoeff& coeff, CubeData* pCube)
{
    InitSHTable();

    // �[���N���A���Ă���
    unsigned int cnt = 0;
    for (unsigned int f = 0; f < NUM_CUBE_FACE; f++)
    {
        memset(pCube->r[f], 0, sizeof(float) * CUBE_SIZE * CUBE_SIZE);
        memset(pCube->g[f], 0, sizeof(float) * CUBE_SIZE * CUBE_SIZE);
        memset(pCube->b[f], 0, sizeof(float) * CUBE_SIZE * CUBE_SIZE);
    }

    // �𓀂���
    for (unsigned int l = 0; l < NUM_COEFF; l++)
    {
        for (unsigned int f = 0; f < NUM_CUBE_FACE; f++)
        {
            for (unsigned int p = 0; p < CUBE_SIZE * CUBE_SIZE; p++)
            {
                pCube->r[f][p] += (coeff.r[l] * mpCubeSHTable[f][p].value[l]);
                pCube->g[f][p] += (coeff.g[l] * mpCubeSHTable[f][p].value[l]);
                pCube->b[f][p] += (coeff.b[l] * mpCubeSHTable[f][p].value[l]);
            }
        }
    }

    // �v�Z�덷�̂��߂��l�����������Ȃ�̂ŁC
    // 0.0�`1.0�͈͓̔���clamp����
    for (unsigned int f = 0; f < NUM_CUBE_FACE; f++)
    {
        for (unsigned int p = 0; p < CUBE_SIZE * CUBE_SIZE; p++)
        {
            pCube->r[f][p] = (pCube->r[f][p] > 1.0f) ? 1.0f : pCube->r[f][p];
            pCube->g[f][p] = (pCube->g[f][p] > 1.0f) ? 1.0f : pCube->g[f][p];
            pCube->b[f][p] = (pCube->b[f][p] > 1.0f) ? 1.0f : pCube->b[f][p];

            pCube->r[f][p] = (pCube->r[f][p] < 0.0f) ? 0.0f : pCube->r[f][p];
            pCube->g[f][p] = (pCube->g[f][p] < 0.0f) ? 0.0f : pCube->g[f][p];
            pCube->b[f][p] = (pCube->b[f][p] < 0.0f) ? 0.0f : pCube->b[f][p];
        }
    }
}

#endif