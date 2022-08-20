#pragma once

#include "../glm-master/glm/glm.hpp"
#include "../glm-master/glm/gtc/matrix_transform.hpp"
#include "../glm-master/glm/gtc/quaternion.hpp"


const int CUBE_SIZE = 2048;
const int NUM_COEFF = 36;

enum
{
    CUBE_FACE_LEFT = 0,
    CUBE_FACE_RIGHT,
    CUBE_FACE_FRONT,
    CUBE_FACE_BACK,
    CUBE_FACE_TOP,
    CUBE_FACE_BOTTOM,

    NUM_CUBE_FACE,
};

typedef struct _SHCoeff
{
    float r[NUM_COEFF];
    float g[NUM_COEFF];
    float b[NUM_COEFF];
} SHCoeff;

typedef struct _CubeData
{
    float r[NUM_CUBE_FACE][CUBE_SIZE * CUBE_SIZE];
    float g[NUM_CUBE_FACE][CUBE_SIZE * CUBE_SIZE];
    float b[NUM_CUBE_FACE][CUBE_SIZE * CUBE_SIZE];
} CubeData;

//----------------------------------------------------------------------
// Functions
//----------------------------------------------------------------------

bool CreateSHCoefficientFromTXT(
    const char* filename,
    SHCoeff& coeff);


bool SaveSHCoefficientToTXT(
    const SHCoeff& coeff,
    const char* filename);

////////////////////////////////////////////////////////////////////////
// SHCalculator class
////////////////////////////////////////////////////////////////////////
class SHCalculator
{
    //------------------------------------------------------------------
    // list of friend classes and methods.
    //------------------------------------------------------------------
    /* NOTHING */

public:
    //------------------------------------------------------------------
    // public variables
    //------------------------------------------------------------------
    static SHCalculator* GetInstance();

    //------------------------------------------------------------------
    // public methods
    //------------------------------------------------------------------
    float Compute(int l, int m, float theta, float phi);
    void  Decompress(const SHCoeff& coeff, CubeData* pCube);
    void  Compress(const CubeData* pCube, SHCoeff& coeff);


    bool CreateCubeDataFromBMP(
        const char* filename1,
        const char* filename2,
        const char* filename3,
        const char* filename4,
        const char* filename5,
        const char* filename6,
        CubeData* pCube);


protected:
    //-------------------------------------------------------------------
    // protected variables
    //-------------------------------------------------------------------
    typedef struct _SHTable36
    {
        float value[NUM_COEFF];
    } SHTable36;

    typedef struct _SolidAngleTable
    {
        float value[NUM_CUBE_FACE][CUBE_SIZE * CUBE_SIZE];
    } SolidAngleTable;

    SHTable36* mpCubeSHTable[NUM_CUBE_FACE];
    SolidAngleTable     mDeltaFormFactor;

    //-------------------------------------------------------------------
    // protected methods
    //-------------------------------------------------------------------
    float ScalingFactor(int l, int m);
    float LegendrePolynomials(int l, int m, float x);
    void  InitSHTable();
    void  ComputeSolidAngle(SolidAngleTable& table);
    void  Compute36(const glm::vec3& vec, SHTable36& sh);

public:

    const SolidAngleTable& GetSHTable() { return mDeltaFormFactor; }


private:
    //-------------------------------------------------------------------
    // private variables
    //-------------------------------------------------------------------
    /* NOTHING */

    //-------------------------------------------------------------------
    // private methods
    //-------------------------------------------------------------------
    SHCalculator();
    ~SHCalculator();
    SHCalculator(const SHCalculator& value) {}
    SHCalculator& operator = (const SHCalculator& value) { return (*this); }
};
