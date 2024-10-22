#pragma once
#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

using namespace std;



#define M_PI 3.1415926535

struct Player
{
	uint64_t Ptr;
	uint64_t Mesh;
	uint64_t BoneArry;
	uint64_t PlayerState;
	int Team;
	int Health;
	int Kill;
	string Name;
};

struct ItemBase
{
	uint64_t Ptr;
	uint64_t Mesh;
	int Team;//99991：车辆； 99992：空投； 99993：枪支；99994：配件； 99995：防具；99996：药品；99997：投掷
	string name;
	int color;

};


class Game
{
	class Vector3
	{
	public:
		Vector3() : x(0.f), y(0.f), z(0.f)
		{

		}

		Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z)
		{

		}
		~Vector3()
		{

		}

		float x;
		float y;
		float z;
		inline Vector3 ToFRotator()
		{
			float RADPI = (float)(180.0f / M_PI);
			float Yaw = (float)atan2f(y, x) * RADPI;
			float Pitch = (float)atan2f(z, sqrtf(powf(x, 2.0f) + powf(y, 2.0f))) * RADPI;
			float Roll = 0.0f;
			return  Vector3(Pitch, Yaw, Roll);
		}

		inline Vector3 Clamp() {
			if (x > 180.0f)
				x -= 360.0f;
			else if (x < -180.0f)
				x += 360.0f;

			if (y > 180.0f)
				y -= 360.0f;
			else if (y < -180.0f)
				y += 360.0f;

			if (x < -89.0f) { x = -89.0f; }

			if (x > 89.0f) { x = 89.0f; }

			while (y < -180.0f) { y += 360.0f; }

			while (y > 180.0f) { y -= 360.0f; }
			z = 0.0f;
			return Vector3(x, y, z);
		}


		inline float Dot(Vector3 v)
		{
			return x * v.x + y * v.y + z * v.z;
		}

		inline float Distance(Vector3 v)
		{
			return float(sqrtf(powf(v.x - x, 2.0) + powf(v.y - y, 2.0) + powf(v.z - z, 2.0)));
		}
		inline float Length()
		{
			return sqrtf(x * x + y * y + z * z);
		}
		inline Vector3& operator+=(const Vector3& v) {
			x += v.x; y += v.y; z += v.z; return *this;
		}

		inline Vector3& operator-=(const Vector3& v) {
			x -= v.x; y -= v.y; z -= v.z; return *this;
		}

		inline Vector3& operator*=(const Vector3& v) {
			x *= v.x; y *= v.y; z *= v.z; return *this;
		}

		inline Vector3& operator/=(const Vector3& v) {
			x /= v.x; y /= v.y; z /= v.z; return *this;
		}

		inline Vector3& operator+=(float v) {
			x += v; y += v; z += v; return *this;
		}

		inline Vector3& operator-=(float v) {
			x -= v; y -= v; z -= v; return *this;
		}

		inline Vector3& operator*=(float v) {
			x *= v; y *= v; z *= v; return *this;
		}

		inline Vector3& operator/=(float v) {
			x /= v; y /= v; z /= v; return *this;
		}

		inline Vector3 operator-() const {
			return Vector3(-x, -y, -z);
		}

		inline Vector3 operator+(const Vector3& v) const {
			return Vector3(x + v.x, y + v.y, z + v.z);
		}

		inline Vector3 operator-(const Vector3& v) const {
			return Vector3(x - v.x, y - v.y, z - v.z);
		}

		inline Vector3 operator*(const Vector3& v) const {
			return Vector3(x * v.x, y * v.y, z * v.z);
		}

		inline Vector3 operator/(const Vector3& v) const {
			return Vector3(x / v.x, y / v.y, z / v.z);
		}

		inline Vector3 operator+(float v) const {
			return Vector3(x + v, y + v, z + v);
		}

		inline Vector3 operator-(float v) const {
			return Vector3(x - v, y - v, z - v);
		}

		inline Vector3 operator*(float v) const {
			return Vector3(x * v, y * v, z * v);
		}

		inline Vector3 operator/(float v) const {
			return Vector3(x / v, y / v, z / v);
		}
	};

	typedef struct _Vector2
	{
		float x;
		float y;
	} Vector2, * PVector2;

	struct FTransform_MOD
	{
		float Rotation_X;
		float Rotation_Y;
		float Rotation_Z;
		float Rotation_W;
		float Translation_X;
		float Translation_Y;
		float Translation_Z;
		float Null;
		float Scale3D_X;
		float Scale3D_Y;
		float Scale3D_Z;
	};

	typedef struct StringA
	{
		char buffer[64];
	};

	typedef enum _EntityType
	{
		EntityTypeUnknown = 0,
		EntityTypePlayer,
		EntityTypeItem,
		XC02,
		XC03,
		XC04,
		XC01,
		XC05,
		ProGrenade,
		XC06,
		XC07,
		XC08,
		XC09,
		XC0010
	}EntityType, * PEntityType;

	typedef struct EntityNameIDTypeMap
	{
		EntityType Type;
		int Color;
		const char* Name;
		EntityNameIDTypeMap(EntityType type, const char* name) :Type(type), Color(NULL), Name(name) {}
		EntityNameIDTypeMap(EntityType type, int color, const char* name) :Type(type), Color(color), Name(name) {}
		EntityNameIDTypeMap() :Type(EntityType::EntityTypeUnknown), Color(NULL), Name(nullptr) {}
	}*PEntityNameIDTypeMap;

	typedef uint64_t(__fastcall* Decrypt_)(int sed, uint64_t ShieldDet);

public:

	void Start();
	void GameStart();
	void InitNameTypeMap();
	bool DecryptInit(uint64_t Encrypt);
	uint64_t DecryptCall(uint64_t a1);
	DWORD DecryptIndex(DWORD value);
	ULONG DecryptID(uint64_t a1);
	std::string GetNames(DWORD ID);
	void DecryptUWorld();
	void DecryptEntity();
	void EntityDraw();
	void AimatPlayer();
	void push_back(vector<Player>* desire, Player member);
	bool back_End(vector<Player> desire, uint64_t ptr);
	void ipush_back(vector<ItemBase>* desire, ItemBase member);
	bool iback_End(vector<ItemBase> desire, uint64_t ptr);
	void GetAxes(Vector3 Rotation, Vector3* X, Vector3* Y, Vector3* Z);
	void SubVector(Vector3 VecA, Vector3 VecB, Vector3* VecC);
	bool WorldScreen(Vector3 Location, Vector2* Screen, float* Distance);
	float DotProduct(Vector3 VecA, Vector3 VecB);
	void GetBonePos(Vector3 VE, Vector3* Result);
	bool DrawPlayer(Player p, FTransform_MOD Mat);
	bool DrawMatrix(uint64_t BoneArry, int Color, FTransform_MOD Mat, float Heath);
	void DrawPlayerInfo(float x, float y, float distance, float health, int kill, int team, string name);
	bool DrawBox(uint64_t BoneArry, int Color);
	void AimAt(Vector2 Screen);
	void aimbot(float x, float y);
	std::string GetPlayName(uint64_t pName);

public:

	BYTE buffer[1024];
	std::unordered_map<ULONG, EntityNameIDTypeMap> EntityIDTypeMap = std::unordered_map<ULONG, EntityNameIDTypeMap>();
	Decrypt_ Decrypt_Call = { 0 };
	uint64_t GNames = NULL;
	uint64_t GameNULL = NULL;
	uint64_t ShieldPtr = NULL;
	uint64_t UWorld = NULL;
	uint64_t Persistent = NULL;
	uint64_t GameInstance = NULL;
	uint64_t PlayerController = NULL;
	uint64_t LocalPawn = NULL;
	uint64_t CameraManager = NULL;
	uint64_t LocalMesh = NULL;
	uint32_t LocalNumber = NULL;
	uint64_t Actor = NULL;
	DWORD GamePid;
	float Tangent;
	Vector3 Pos, Rot, AxisX, AxisY, AxisZ;
	FTransform_MOD Matrix;

	uint64_t CloseMesh = NULL;

};




