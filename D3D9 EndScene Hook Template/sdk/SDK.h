#include <Windows.h>
#include <cstdint>
#include <string>
#include <cmath>          // std::sqrt
#define M_PI       3.14159265358979323846   // pi

int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
#define GetDX(x) (x - 1 * (65536 / ScreenWidth))
#define GetDY(y) (y - 1* (65536 / ScreenHeight) )

struct Vector2
{
    float x, y;
    Vector2()
    {

    }
    Vector2(float x_, float y_)
    {
        x = x_;
        y = y_;
    }
    std::string ToString()
    {
        std::string x_(std::to_string(x));
        std::string y_(std::to_string(y));
        std::string output = x_ + ", " + y_;
        return output;
    }
};

struct Vector3
{
    float x, y, z;
    Vector3()
    {

    }
    Vector3(float x_, float y_, float z_)
    {
        x = x_;
        y = y_;
        z = z_;
    }
    std::string ToString()
    {
        std::string x_(std::to_string(x));
        std::string y_(std::to_string(y));
        std::string z_(std::to_string(z));
        std::string output = x_ + ", " + y_ + ", " + z_;
        return output;
    }
};

struct Vector4
{
    float x, y, z, w;
    Vector4()
    {

    }
    Vector4(float x_, float y_, float z_, float w_)
    {
        x = x_;
        y = y_;
        z = z_;
        w = w_;
    }
    std::string ToString()
    {
        std::string x_(std::to_string(x));
        std::string y_(std::to_string(y));
        std::string z_(std::to_string(z));
        std::string w_(std::to_string(w));
        std::string output = x_ + ", " + y_ + ", " + z_ + ", " + w_;
        return output;
    }
};

class Refdefs
{
public:
    uint32_t X; //0x0000
    uint32_t Y; //0x0004
    uint32_t Width; //0x0008
    uint32_t Height; //0x000C
    Vector2 FOV; //0x0010
    float FullFOV; //0x0018
    Vector3 Origin; //0x001C
    char pad_0028[4]; //0x0028
    Vector3 ViewMatrix[3]; //0x002C
    char pad_0050[28]; //0x0050
}; //Size: 0x006C
inline Refdefs* refdef = (Refdefs*)0x3520338;

Vector3 Subtract(Vector3 src, Vector3 dst)
{
    Vector3 diff;
    diff.x = src.x - dst.x;
    diff.y = src.y - dst.y;
    diff.z = src.z - dst.z;
    return diff;
}

float DotProduct(Vector3 src, Vector3 dst)
{
    return src.x * dst.x + src.y * dst.y + src.z * dst.z;
}

bool WorldToScreen(Vector3 world, Vector2& screen)
{
    auto Position = Subtract(world, refdef->Origin);
    Vector3 Transform;

    // get our dot products from viewAxis
    Transform.x = DotProduct(Position, refdef->ViewMatrix[1]);
    Transform.y = DotProduct(Position, refdef->ViewMatrix[2]);
    Transform.z = DotProduct(Position, refdef->ViewMatrix[0]);

    // make sure it is in front of us
    if (Transform.z < 0.1f)
        return false;

    // get the center of the screen
    Vector2 Center;
    Center.x = (float)(refdef->Width * 0.5f);
    Center.y = (float)(refdef->Height * 0.5f);

    screen.x = Center.x * (1 - (Transform.x / refdef->FOV.x / Transform.z));
    screen.y = Center.y * (1 - (Transform.y / refdef->FOV.y / Transform.z));
    return true;
}

LPDIRECT3DTEXTURE9 Red, Green, Blue, Yellow;
HRESULT GenerateTexture(IDirect3DDevice9* pDevice, IDirect3DTexture9** ppD3Dtex, DWORD colour32)
{
    if (FAILED(pDevice->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
        return E_FAIL;

    WORD colour16 = ((WORD)((colour32 >> 28) & 0xF) << 12)
        | (WORD)(((colour32 >> 20) & 0xF) << 8)
        | (WORD)(((colour32 >> 12) & 0xF) << 4)
        | (WORD)(((colour32 >> 4) & 0xF) << 0);

    D3DLOCKED_RECT d3dlr;
    (*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
    WORD* pDst16 = (WORD*)d3dlr.pBits;

    for (int xy = 0; xy < 8 * 8; xy++)
        *pDst16++ = colour16;

    (*ppD3Dtex)->UnlockRect(0);

    return S_OK;
}

const DWORD CheatOpcodeBase = 0x00418545;
const char* originalOpCodes = "\xF3\x0F\x5C\xC1\xF3\x0F\x59\x05\x3C\x9F\x83\x00\xF3\x0F\x11\x84\xB7\x24\x01\x00\x00\x83\xC6\x01\x83\xFE\x03\x0F\x8C\x36\xFF\xFF\xFF";
const char* newOpCodes = "\xF3\x0F\x10\x05\x8C\xD1\x8E\x01\xF3\x0F\x11\x87\x24\x01\x00\x00\xF3\x0F\x10\x05\x90\xD1\x8E\x01\xF3\x0F\x11\x87\x28\x01\x00\x00\x90";
int OpCodeLength = 33; // Need the length here because \x00 is same as end of string D: so, opcodes don't all get written

void Patch(DWORD address_, LPVOID data_, int size_) {
    DWORD oldProtection;
    VirtualProtect((LPVOID)address_, size_, PAGE_EXECUTE_READWRITE, &oldProtection);
    memcpy((LPVOID)address_, data_, size_);
    VirtualProtect((LPVOID)address_, size_, oldProtection, NULL);
}

float Calc3D_Distance(Vector3 src, Vector3 dst)
{
    return sqrt((src.x - dst.x) * (src.x - dst.x) + (src.y - dst.y) * (src.y - dst.y) + (src.z - src.y));
}

void CalcAngle(float* angles, Vector3 src, Vector3 dst)
{
    float delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
    double distance = sqrt(pow(src.x - dst.x, 2.0) + pow(src.y - dst.y, 2.0));
    angles[0] = (float)(atanf(delta[2] / distance) * 180 / M_PI) + 800 / distance;
    angles[1] = (float)(atanf(delta[1] / delta[0]) * 180 / M_PI) + 100 / distance;
    if (delta[0] >= 0.0)
        angles[1] + 180.0f;
}

bool compare_targets(Vector3 src, Vector3 dst)
{
    return Calc3D_Distance(src, dst) > Calc3D_Distance(dst, src);
}