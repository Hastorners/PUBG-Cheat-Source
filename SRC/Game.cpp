#include "Game.h"
#include "overlay.h"
#include "Offset.h"
#include "vmmProc.h"


ImVec4 YColor(DWORD rgb, FLOAT a) {
	return ImVec4((float)((rgb >> 16) & 0xFF) * (1.0f / 255.0f), (float)((rgb >> 8) & 0xFF) * (1.0f / 255.0f), (float)((rgb >> 0) & 0xFF) * (1.0f / 255.0f), a);
}


bool Decrypt = false, BoolGame = false, BoolData = false, BoolRead = false, DataRead = false;
bool GHD01 = true;
bool NM0001 = true;
bool NM0002 = true;
Overlay Darw = Overlay();

auto BBF0 = vector<Player>{};
auto BBD0 = vector<ItemBase>{};

VMMDLL_SCATTER_HANDLE TS;
VMMDLL_SCATTER_HANDLE HS;
VMMDLL_SCATTER_HANDLE AS;


static DWORD WINAPI StaticMessageStart(void* Param)
{
	Game* ov = (Game*)Param;
	ov->GameStart();
	return 0;
}

static DWORD WINAPI StartUWorld(void* Param)
{
	Game* ov = (Game*)Param;
	ov->DecryptUWorld();
	return 0;
}

static DWORD WINAPI StartEntity(void* Param)
{
	Game* ov = (Game*)Param;
	ov->DecryptEntity();
	return 0;
}

static DWORD WINAPI StartPlayer(void* Param)
{
	Game* ov = (Game*)Param;
	ov->EntityDraw();
	return 0;
}


void Game::Start()
{
	DWORD ThreadID;
	CreateThread(NULL, 0, StaticMessageStart, (void*)this, 0, &ThreadID);
}

void Game::GameStart()
{
	if (!VMMDLL_Initialize())
		ExitProcess(1);

	while (Darw.GameWindow == NULL) {

		//Darw.GameWindow = FindWindowA("Notepad", NULL);
		//if (Darw.GameWindow == NULL)
		//{
		//	Darw.GameWindow = FindWindowA("SDL_app", NULL);
		//	if (Darw.GameWindow == NULL)
		//	{
		//		Darw.GameWindow = GetDesktopWindow();
		//	}
		//}
		//Darw.GameWindow = FindWindowA("SDL_app", NULL);
		Darw.GameWindow = GetDesktopWindow();

		Sleep(300);
	}

	vector<DWORD> pidList = GetProcessPidList();


	for (size_t i = 0; i < pidList.size(); i++)
	{
		LPSTR szName = ProcessGetInformationString(pidList[i]);

		if (strstr(szName, "TslGame.exe") != NULL) {
			GamePid = pidList[i];
			SetProcessPid(GamePid);
			GameNULL = GetModuleFromName("TslGame.exe");

			if (Read<uint64_t>(GameNULL + Offset::UWorld) > 0)
			{
				GamePid = pidList[i];
				break;
			}
		}

		VMMDLL_MemFree(szName);
	}

	printf("游戏ID:%d\n", GamePid);

	SetProcessPid(GamePid);
	GameNULL = GetModuleFromName("TslGame.exe");
	HS = Scatter_Initialize(VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_ZEROPAD_ON_FAIL);
	TS = Scatter_Initialize(VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_ZEROPAD_ON_FAIL);
	AS = Scatter_Initialize(VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_ZEROPAD_ON_FAIL);
	ShieldPtr = Read<uint64_t>(GameNULL + Offset::Shield);
	DecryptInit(ShieldPtr);
	CreateThread(NULL, 0, StartUWorld, (void*)this, 0, NULL);
	CreateThread(NULL, 0, StartEntity, (void*)this, 0, NULL);
	CreateThread(NULL, 0, StartPlayer, (void*)this, 0, NULL);

	Darw.Start();
}

void Game::DecryptUWorld()
{
	std::unordered_map<ULONG, EntityNameIDTypeMap>::iterator IDTypeMapIt = std::unordered_map<ULONG, EntityNameIDTypeMap>::iterator();
	DWORD Time = NULL;

	while (!Decrypt){

		GNames = DecryptCall(Read<uint64_t>(GameNULL + Offset::GNames));
		GNames = DecryptCall(Read<uint64_t>(GNames)); if (GNames <= 65536)return;

		InitNameTypeMap();
		if (EntityIDTypeMap.empty() == false) 
		{
			printf("[+]缓存完成！\n");
			Decrypt = true;
		}
		else {
			Decrypt = false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
	while (true){
		if (Decrypt){
			if (GetTickCount64() - Time >= 3000){
				UWorld = DecryptCall(Read<uint64_t>(GameNULL + Offset::UWorld)); if (UWorld <= 65536) continue;

				auto UWorldID = DecryptID(UWorld); if (UWorldID <= 0) continue;

				IDTypeMapIt = EntityIDTypeMap.find(UWorldID);
				if (IDTypeMapIt == EntityIDTypeMap.end()) { continue; }
				if (IDTypeMapIt->second.Type == EntityType::XC0010)
					BoolGame = true;
				if (IDTypeMapIt->second.Type == EntityType::XC09)
				{
					BoolGame = false;
					BoolData = false;
		
				}
				if (BoolGame == true && BoolData == false)
				{
					Persistent = DecryptCall(Read<uint64_t>(UWorld + Offset::Level)); if (Persistent <= 65536) continue;
					GameInstance = DecryptCall(Read<uint64_t>(UWorld + Offset::GameInstence)); if (GameInstance <= 65536) continue;
					GameInstance = DecryptCall(Read<uint64_t>(Read<uint64_t>(GameInstance + Offset::LocalPlayer))); if (GameInstance <= 65536) continue;
					PlayerController = DecryptCall(Read<uint64_t>(GameInstance + Offset::PlayerController)); if (PlayerController <= 65536) continue;
					LocalPawn = DecryptCall(Read<uint64_t>(PlayerController + Offset::AcknowledgedPawn)); if (LocalPawn <= 65536) continue;
					CameraManager = Read<uint64_t>(PlayerController + Offset::PlayerCameraManager); if (CameraManager <= 65536) continue;
					Actor = DecryptCall(Read<uint64_t>(Persistent + Offset::Actor)); if (Actor <= 65536) continue;
					LocalMesh = Read<uint64_t>(LocalPawn + Offset::Mesh); if (LocalMesh <= 65536) continue;
					LocalNumber = Read<int>(LocalPawn + Offset::TeamNumber);
					SClear(HS, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_ZEROPAD_ON_FAIL);
					SPrepare(HS, CameraManager, 6000);
					SPrepare(HS, LocalMesh + Offset::Position, 12);

					BoolData = true;
				}

				Time = GetTickCount64();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void Game::DecryptEntity()
{
	std::unordered_map<ULONG, EntityNameIDTypeMap>::iterator IDTypeMapIt = std::unordered_map<ULONG, EntityNameIDTypeMap>::iterator();

	auto LLM01 = vector<Player>{};
	auto KJN01 = vector<ItemBase>{};
	auto LLM02 = Player();
	auto KJN02 = ItemBase();

	while (true)
	{
		if (GetAsyncKeyState(112))
		{
			GHD01 = !GHD01;
		}
		if (GetAsyncKeyState(113))
		{
			NM0001 = !NM0001;
		}
		if (GetAsyncKeyState(114))
		{
			NM0002 = !NM0002;
		}
		if (BoolGame && BoolData)
		{
			auto Count = Read<int>(Actor + 8);
			//printf("Count:%d\n", Count);
			if (Count > 1 && Count < 9000) {
				auto Object = Read<uint64_t>(Actor); if (Object <= 65536) continue;
				for (auto i = 0; i < Count; i++) {
					auto ObjectPtr = Read<uint64_t>(Object + i * 8); if (ObjectPtr <= 65536) continue;
					auto ObjectID = DecryptID(ObjectPtr); if (ObjectID <= 0) continue;

					if (ObjectPtr == LocalPawn) continue;
					IDTypeMapIt = EntityIDTypeMap.find(ObjectID);
					if (IDTypeMapIt == EntityIDTypeMap.end()) continue;
					if (IDTypeMapIt->second.Type == EntityType::EntityTypePlayer)   //判断是否人物
					{ 
						if (back_End(LLM01, ObjectPtr)) { continue; }
						auto Health = Read<float>(ObjectPtr + Offset::Health);
						if (Health == 0.f && Health < 0.1f) { continue; }
						auto Mesh = Read<uint64_t>(ObjectPtr + Offset::Mesh); if (Mesh <= 65536) continue;
						auto Team = Read<int>(ObjectPtr + Offset::TeamNumber);
						if (Team >= 100000) { Team -= 100000; }
						auto PlayerState = Read<uint64_t>(ObjectPtr + Offset::PlayerState);

						auto pName = Read<uint64_t>(ObjectPtr + Offset::Playname);

						//热能
				/*		if (Write(ObjectPtr + 0x1268, 0x1))
						{
							printf("111:\n");
						}
						else
						{
							printf("222:\n");
						}*/
						
						//热能
						//printf("N:%s\n", GetPlayName(pName));
					
						LLM02.Mesh = Mesh;
						LLM02.Ptr = ObjectPtr;
						LLM02.Team = Team;
						LLM02.PlayerState = PlayerState;
						LLM02.Name = GetPlayName(pName);

						push_back(&LLM01, LLM02);
					}

					if (IDTypeMapIt->second.Type == EntityType::XC06)
					{
						if (!NM0001) continue;
						
						if (iback_End(KJN01, ObjectPtr)) { continue; }
					
						//auto Mesh = Read<uint64_t>(ObjectPtr + Offset::DroppedItemGroup);  if (Mesh <= 65536) continue;
						auto Mesh = Read<uint64_t>(Read<uint64_t>(ObjectPtr + Offset::DroppedItemGroup));  if (Mesh <= 65536) continue;

						KJN02.Ptr = ObjectPtr;
						KJN02.Mesh = Mesh;
						KJN02.Team = 99991;
						KJN02.name = IDTypeMapIt->second.Name;
						KJN02.color = IDTypeMapIt->second.Color;


						ipush_back(&KJN01, KJN02);
					}

					if (IDTypeMapIt->second.Type == EntityType::XC08)
					{
						if (!NM0002) continue;

						if (iback_End(KJN01, ObjectPtr)) { continue; }

						//auto Mesh = Read<uint64_t>(ObjectPtr + Offset::DroppedItemGroup);  if (Mesh <= 65536) continue;
						auto Mesh = Read<uint64_t>(Read<uint64_t>(ObjectPtr + Offset::DroppedItemGroup));  if (Mesh <= 65536) continue;

						KJN02.Ptr = ObjectPtr;
						KJN02.Mesh = Mesh;
						KJN02.Team = 99992;
						KJN02.name = IDTypeMapIt->second.Name;
						KJN02.color = IDTypeMapIt->second.Color;


						ipush_back(&KJN01, KJN02);
					}


					if (IDTypeMapIt->second.Type == EntityType::EntityTypeItem)
					{
						if (!GHD01) continue;
						uint64_t pItemArray = Read<uint64_t>(ObjectPtr + Offset::DroppedItemGroup);
						int pItemCount = Read<int>(ObjectPtr + Offset::DroppedItemGroup + 8);

						if (pItemArray > 0 && pItemCount < 50 && pItemCount > 0)
						{
							for (int n = 0; n < pItemCount; n++)
							{
								uint64_t pItemObjPtr = Read<uint64_t>(pItemArray + (n * 16)); if (pItemObjPtr <= 65536) continue;
								uint64_t pUItemAddress = Read<uint64_t>(pItemObjPtr + Offset::DroppedItemGroup_UItem); if (pUItemAddress <= 65536) continue;
								int pUItemID = Read<int>(Read<uint64_t>(pUItemAddress + Offset::OP0) + Offset::OP01);

								if (pUItemID < 0)
								{
									continue;
								}

								IDTypeMapIt = EntityIDTypeMap.find(pUItemID);
								if (IDTypeMapIt == EntityIDTypeMap.end()) continue;

								if (IDTypeMapIt->second.Type == EntityType::XC02)
								{
									if (iback_End(KJN01, ObjectPtr)) { continue; }

									KJN02.Ptr = pItemObjPtr;
									KJN02.Mesh = pItemObjPtr;
									KJN02.Team = 99993;
									KJN02.name = IDTypeMapIt->second.Name;
									KJN02.color = IDTypeMapIt->second.Color;

									ipush_back(&KJN01, KJN02);
								}
								//XC03
								if (IDTypeMapIt->second.Type == EntityType::XC03)
								{
									if (iback_End(KJN01, ObjectPtr)) { continue; }

									KJN02.Ptr = pItemObjPtr;
									KJN02.Mesh = pItemObjPtr;
									KJN02.Team = 99994;
									KJN02.name = IDTypeMapIt->second.Name;
									KJN02.color = IDTypeMapIt->second.Color;

									ipush_back(&KJN01, KJN02);

								}
								//XC04
								if (IDTypeMapIt->second.Type == EntityType::XC04)
								{
									if (iback_End(KJN01, ObjectPtr)) { continue; }

									KJN02.Ptr = pItemObjPtr;
									KJN02.Mesh = pItemObjPtr;
									KJN02.Team = 99995;
									KJN02.name = IDTypeMapIt->second.Name;
									KJN02.color = IDTypeMapIt->second.Color;

									ipush_back(&KJN01, KJN02);
								}
								//XC01
								if (IDTypeMapIt->second.Type == EntityType::XC01)
								{
									if (iback_End(KJN01, ObjectPtr)) { continue; }

									KJN02.Ptr = pItemObjPtr;
									KJN02.Mesh = pItemObjPtr;
									KJN02.Team = 99996;
									KJN02.name = IDTypeMapIt->second.Name;
									KJN02.color = IDTypeMapIt->second.Color;

									ipush_back(&KJN01, KJN02);
								}
								//XC05
								if (IDTypeMapIt->second.Type == EntityType::XC05)
								{
									if (iback_End(KJN01, ObjectPtr)) { continue; }

									KJN02.Ptr = pItemObjPtr;
									KJN02.Mesh = pItemObjPtr;
									KJN02.Team = 99997;
									KJN02.name = IDTypeMapIt->second.Name;
									KJN02.color = IDTypeMapIt->second.Color;

									ipush_back(&KJN01, KJN02);
								}
							}
						}
					}

					continue;
				}
				DataRead = false;
				if (BoolRead == false){
					BBF0.clear();
					BBF0 = LLM01;

					BBD0.clear();
					BBD0 = KJN01;
				}
				DataRead = true;
				LLM01.clear();
				KJN01.clear();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(400));
	}

}

void Game::EntityDraw()
{
	Vector2 Screen;
	float Distance;
	Vector3 playerPos;
	auto NMK01 = vector<Player>{};
	auto NMK02 = vector<ItemBase>{};


	auto NMK03 = vector<Player>{};
	auto NMK04 = vector<FTransform_MOD>{};

	auto NMK05 = Player();

	Vector3 LocalPos;

	clock_t start, end;
	double endtime;
	float Width1 = 100;
	float Height1 = 160;

	float Heath;
	int HColor;

	while (true)
	{
		if (DataRead == true) {
			BoolRead = true;
			NMK01 = BBF0;
			NMK02 = BBD0;
			BoolRead = false;
		}
		//printf("玩家:%d\n", 临时玩家.size());


		if (BoolGame && BoolData)
		{
			ExecuteRead(HS);
			Tangent = tan(SRead<float>(HS, CameraManager + Offset::CameraFov) * (float)M_PI / 360.0f);
			Pos = SRead<Vector3>(HS, CameraManager + Offset::CameraPos);
			Rot = SRead<Vector3>(HS, CameraManager + Offset::CameraRot);
			GetAxes(Rot, &AxisX, &AxisY, &AxisZ);
			float GHY01 = 20.0f;
			float 行距 = 300;
			float 间隔 = 20.0f;

			Darw.BeginDraw();
			行距 += 间隔;
			Darw.DrawNewText(GHY01, 行距, GHD01 ? NNR7: NNR8, 18.f, u8"F1  %s    [%s]", "显示物资", GHD01 ? "开启" : "关闭");
			行距 += 间隔;
			Darw.DrawNewText(GHY01, 行距, NM0001 ? NNR1: NNR8, 18.f, u8"F2  %s    [%s]", "显示载具", NM0001 ? "开启" : "关闭");
			行距 += 间隔;
			Darw.DrawNewText(GHY01, 行距, NM0002 ? NNR0: NNR8, 18.f, u8"F3  %s    [%s]", "显示盒子", NM0002 ? "开启" : "关闭");
			ImGui::Begin(u8"  HOME 显示  隐藏  ");
			ImGui::CollapsingHeader("esp");
			ImGui::Checkbox(u8"载具", &NM0002);
			
			
			
			ImGui::End();
			for (auto i = 0; i < NMK01.size(); i++)
			{
				////热能
				//Write(临时玩家[i].Ptr + 0x1268, 0x1);
				////热能
				if (LocalNumber == NMK01[i].Team)
					continue;
				SPrepare(TS, NMK01[i].Mesh + Offset::BoneArry, 50);
				SPrepare(TS, NMK01[i].Mesh + Offset::Bone, 4);
				SPrepare(TS, NMK01[i].Ptr + Offset::Health, 4);
				SPrepare(TS, NMK01[i].PlayerState + Offset::PlayerSatisitc, 4);
			}

			ExecuteRead(TS);

			for (auto i = 0; i < NMK01.size(); i++)
			{
				if (LocalNumber == NMK01[i].Team)
					continue;
				Matrix = SRead<FTransform_MOD>(TS, NMK01[i].Mesh + Offset::BoneArry);
				auto BoneArry = SRead<uint64_t>(TS, NMK01[i].Mesh + Offset::Bone);
				auto Health = SRead<float>(TS, NMK01[i].Ptr + Offset::Health);
				auto PlayerSatisitc = SRead<int>(TS, NMK01[i].PlayerState + Offset::PlayerSatisitc);

				if (Health <= 0 )
				{
					continue;
				}
				NMK05.Health = Health;
				NMK05.Kill = PlayerSatisitc;
				NMK05.BoneArry = BoneArry;
				NMK05.Ptr = NMK01[i].Ptr;
				NMK05.Team = NMK01[i].Team;
				NMK05.Mesh = NMK01[i].Mesh;
				NMK05.Name = NMK01[i].Name;

				NMK03.push_back(NMK05);
				NMK04.push_back(Matrix);
			}
			SClear(TS, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_ZEROPAD_ON_FAIL);

			for (auto i = 0; i < NMK03.size(); i++)
			{
				SPrepare(TS, NMK03[i].BoneArry + 736, 12);
				SPrepare(TS, NMK03[i].BoneArry + 256, 12);
				SPrepare(TS, NMK03[i].BoneArry + 64, 12);
				SPrepare(TS, NMK03[i].BoneArry + 4240, 12);
				SPrepare(TS, NMK03[i].BoneArry + 4288, 12);
				SPrepare(TS, NMK03[i].BoneArry + 4336, 12);
				SPrepare(TS, NMK03[i].BoneArry + 5536, 12);
				SPrepare(TS, NMK03[i].BoneArry + 5584, 12);
				SPrepare(TS, NMK03[i].BoneArry + 5632, 12);
				SPrepare(TS, NMK03[i].BoneArry + 8272, 12);
				SPrepare(TS, NMK03[i].BoneArry + 8320, 12);
				SPrepare(TS, NMK03[i].BoneArry + 8368, 12);
				SPrepare(TS, NMK03[i].BoneArry + 8560, 12);
				SPrepare(TS, NMK03[i].BoneArry + 8608, 12);
				SPrepare(TS, NMK03[i].BoneArry + 8656, 12);

			}
			ExecuteRead(TS);
			
			for (auto i = 0; i < NMK03.size(); i++)
			{
				DrawPlayer(NMK03[i], NMK04[i]);
				//DrawMatrix(临时数组[i], NNR7, NMK04[i], 临时血量[i]);

			}
			SClear(TS, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_ZEROPAD_ON_FAIL);

			NMK03.clear();
			NMK04.clear();

			for (auto i = 0; i < NMK02.size(); i++)
			{
				SPrepare(TS, NMK02[i].Mesh + Offset::Position, 12);
			}
			ExecuteRead(TS);
			for (auto i = 0; i < NMK02.size(); i++)
			{

				Vector3 VE = SRead<Vector3>(TS, NMK02[i].Mesh + Offset::Position);
				if (WorldScreen(VE, &Screen, &Distance) == false)
					continue;

		
				Darw.DrawNewText(Screen.x, Screen.y, NMK02[i].color, 18.f, u8"%s [%.0fM]", NMK02[i].name, Distance);

			}
			SClear(TS, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_ZEROPAD_ON_FAIL);

			NMK02.clear();
			Darw.EndDraw();
	
		}
		else
		{
			//std::this_thread::sleep_for(std::chrono::milliseconds(100));

		}
	}
}


bool Game::DrawPlayer(Player p,FTransform_MOD Mat)
{
	uint64_t BoneArry = p.BoneArry;
	int Color = NNR3;

	float Distance;
	Vector3 VE = Vector3(0, 0, 0), Pos = Vector3(0, 0, 0);
	Vector2 Screen;
	float X_, Y_, Z_, X, Y, Z, _X, _Y, _Z, _11, _12, _13, _21, _22, _23, _31, _32, _33;
	float bx, by, vx, vy, tx, ty, dx, dy, mX, my;

	X = Mat.Rotation_X + Mat.Rotation_X;
	Y = Mat.Rotation_Y + Mat.Rotation_Y;
	Z = Mat.Rotation_Z + Mat.Rotation_Z;
	_X = Mat.Rotation_X * X;
	_Y = Mat.Rotation_Y * Y;
	_Z = Mat.Rotation_Z * Z;
	_11 = (1 - (_Y + _Z)) * Mat.Scale3D_X;
	_22 = (1 - (_X + _Z)) * Mat.Scale3D_Y;
	_33 = (1 - (_X + _Y)) * Mat.Scale3D_Z;
	_Z = Mat.Rotation_Y * Z;
	_X = Mat.Rotation_W * X;
	_32 = (_Z - _X) * Mat.Scale3D_Z;
	_23 = (_Z + _X) * Mat.Scale3D_Y;
	_Y = Mat.Rotation_X * Y;
	_Z = Mat.Rotation_W * Z;
	_21 = (_Y - _Z) * Mat.Scale3D_Y;
	_12 = (_Y + _Z) * Mat.Scale3D_X;
	_Y = Mat.Rotation_W * Y;
	_Z = Mat.Rotation_X * Z;
	_31 = (_Z + _Y) * Mat.Scale3D_Z;
	_13 = (_Z - _Y) * Mat.Scale3D_X;

	VE = SRead<Vector3>(TS, BoneArry + 736);



	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;


	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	mX = Screen.x;
	my = Screen.y;



	//if (true)//距离

	float Width1 = 100;
	int HColor;
	{
		char dist[64];
		sprintf_s(dist, "[%.0fM]", Distance);
		Darw.DrawNewText(Screen.x - Width1 / -4, Screen.y - 1.5, NNR8, 14, dist);
	}
	DrawPlayerInfo(mX, my, Distance, p.Health, p.Kill, p.Team, p.Name);


	VE = SRead<Vector3>(TS, BoneArry + 256);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	bx = Screen.x;
	by = Screen.y;


	Darw.DrawLine(mX, my, bx, by, Color, 1);

	VE = SRead<Vector3>(TS, BoneArry + 64);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	vx = Screen.x;
	vy = Screen.y;

	Darw.DrawLine(vx, vy, bx, by, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 4240);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	tx = Screen.x;
	ty = Screen.y;

	Darw.DrawLine(tx, ty, bx, by, Color, 1);

	VE = SRead<Vector3>(TS, BoneArry + 4288);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	dx = Screen.x;
	dy = Screen.y;

	Darw.DrawLine(dx, dy, tx, ty, Color, 1);

	VE = SRead<Vector3>(TS, BoneArry + 4336);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	Darw.DrawLine(Screen.x, Screen.y, dx, dy, Color, 1.2);

	VE = SRead<Vector3>(TS, BoneArry + 5536);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	tx = Screen.x;
	ty = Screen.y;

	Darw.DrawLine(tx, ty, bx, by, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 5584);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	dx = Screen.x;
	dy = Screen.y;

	Darw.DrawLine(dx, dy, tx, ty, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 5632);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	Darw.DrawLine(Screen.x, Screen.y, dx, dy, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 8272);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	tx = Screen.x;
	ty = Screen.y;
	Darw.DrawLine(tx, ty, vx, vy, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 8320);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	dx = Screen.x;
	dy = Screen.y;
	Darw.DrawLine(dx, dy, tx, ty, Color, 1);

	VE = SRead<Vector3>(TS, BoneArry + 8368);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	Darw.DrawLine(Screen.x, Screen.y, dx, dy, Color, 1.2);

	VE = SRead<Vector3>(TS, BoneArry + 8560);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	tx = Screen.x;
	ty = Screen.y;
	Darw.DrawLine(tx, ty, vx, vy, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 8608);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	dx = Screen.x;
	dy = Screen.y;
	Darw.DrawLine(dx, dy, tx, ty, Color, 1.2);

	VE = SRead<Vector3>(TS, BoneArry + 8656);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	Darw.DrawLine(Screen.x, Screen.y, dx, dy, Color, 1);



	return true;
}

bool Game::DrawMatrix(uint64_t BoneArry, int Color, FTransform_MOD Mat, float Heath)
{
	float Distance;
	Vector3 VE = Vector3(0, 0, 0), Pos = Vector3(0, 0, 0);
	Vector2 Screen;
	float X_, Y_, Z_, X, Y, Z, _X, _Y, _Z, _11, _12, _13, _21, _22, _23, _31, _32, _33;
	float bx, by, vx, vy, tx, ty, dx, dy, mX, my;

	X = Mat.Rotation_X + Mat.Rotation_X;
	Y = Mat.Rotation_Y + Mat.Rotation_Y;
	Z = Mat.Rotation_Z + Mat.Rotation_Z;
	_X = Mat.Rotation_X * X;
	_Y = Mat.Rotation_Y * Y;
	_Z = Mat.Rotation_Z * Z;
	_11 = (1 - (_Y + _Z)) * Mat.Scale3D_X;
	_22 = (1 - (_X + _Z)) * Mat.Scale3D_Y;
	_33 = (1 - (_X + _Y)) * Mat.Scale3D_Z;
	_Z = Mat.Rotation_Y * Z;
	_X = Mat.Rotation_W * X;
	_32 = (_Z - _X) * Mat.Scale3D_Z;
	_23 = (_Z + _X) * Mat.Scale3D_Y;
	_Y = Mat.Rotation_X * Y;
	_Z = Mat.Rotation_W * Z;
	_21 = (_Y - _Z) * Mat.Scale3D_Y;
	_12 = (_Y + _Z) * Mat.Scale3D_X;
	_Y = Mat.Rotation_W * Y;
	_Z = Mat.Rotation_X * Z;
	_31 = (_Z + _Y) * Mat.Scale3D_Z;
	_13 = (_Z - _Y) * Mat.Scale3D_X;

	VE = SRead<Vector3>(TS, BoneArry + 736);

	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	mX = Screen.x;
	my = Screen.y;

	//if (true)//距离
	//{
	//	char dist[64];
	//	sprintf_s(dist, "[%.0fM]", Distance);
	//	Darw.DrawNewText(Screen.x - Width1 / 4, Screen.y - 30, NNR3, 18, dist);
	//}
	float Width1 = 100;
	int HColor;

	DrawPlayerInfo(mX, my, Distance, Heath,0,20,"AAAAAA");


	VE = SRead<Vector3>(TS, BoneArry + 256);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	bx = Screen.x;
	by = Screen.y;


	Darw.DrawLine(mX, my, bx, by, Color, 1);

	VE = SRead<Vector3>(TS, BoneArry + 64);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	vx = Screen.x;
	vy = Screen.y;

	Darw.DrawLine(vx, vy, bx, by, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 4240);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	tx = Screen.x;
	ty = Screen.y;

	Darw.DrawLine(tx, ty, bx, by, Color, 1);

	VE = SRead<Vector3>(TS, BoneArry + 4288);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	dx = Screen.x;
	dy = Screen.y;

	Darw.DrawLine(dx, dy, tx, ty, Color, 1);

	VE = SRead<Vector3>(TS, BoneArry + 4336);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	Darw.DrawLine(Screen.x, Screen.y, dx, dy, Color, 1.2);

	VE = SRead<Vector3>(TS, BoneArry + 5536);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	tx = Screen.x;
	ty = Screen.y;

	Darw.DrawLine(tx, ty, bx, by, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 5584);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;

	dx = Screen.x;
	dy = Screen.y;

	Darw.DrawLine(dx, dy, tx, ty, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 5632);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	Darw.DrawLine(Screen.x, Screen.y, dx, dy, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 8080);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	tx = Screen.x;
	ty = Screen.y;
	Darw.DrawLine(tx, ty, vx, vy, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 8128);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	dx = Screen.x;
	dy = Screen.y;
	Darw.DrawLine(dx, dy, tx, ty, Color, 1);

	VE = SRead<Vector3>(TS, BoneArry + 8176);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	Darw.DrawLine(Screen.x, Screen.y, dx, dy, Color, 1.2);

	VE = SRead<Vector3>(TS, BoneArry + 8368);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	tx = Screen.x;
	ty = Screen.y;
	Darw.DrawLine(tx, ty, vx, vy, Color, 1);


	VE = SRead<Vector3>(TS, BoneArry + 8416);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	dx = Screen.x;
	dy = Screen.y;
	Darw.DrawLine(dx, dy, tx, ty, Color, 1.2);

	VE = SRead<Vector3>(TS, BoneArry + 8464);
	Pos.x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Mat.Translation_X;
	Pos.y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Mat.Translation_Y;
	Pos.z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Mat.Translation_Z;
	if (WorldScreen(Pos, &Screen, &Distance) == false)
		return false;
	Darw.DrawLine(Screen.x, Screen.y, dx, dy, Color, 1);



	return true;
}

void Game::DrawPlayerInfo(float x, float y, float distance, float health, int kill, int team, string name) {
	float HealthMaxWidth = 80.0f - distance / (12.0f + distance / 100.0f * 8.0f);
	float IntervalWidth = 20.0f - distance / 15.0f;
	IntervalWidth = IntervalWidth < 15.0f ? 15.0f : IntervalWidth;
	float HealthX = x - (HealthMaxWidth / 2.0f);
	float HealthY = y - IntervalWidth + 5.0f;
	float HealthWidth = (HealthMaxWidth * (health / 100.f));
	HealthWidth = HealthWidth < 1 ? 1 : HealthWidth;

	char PlayerTeamStr[4];

	snprintf(PlayerTeamStr, sizeof(PlayerTeamStr), "%d", team);
	char myName[64];
	snprintf(myName, sizeof(myName), "%s", name);

	//Darw.DrawFilledRect(HealthX - 2, HealthY - 22, HealthMaxWidth + 4, 28, NNR3, 100, 0);
	Darw.DrawNewText(HealthX + 25, HealthY - 22.f, NNR3, 16.f, myName);

	//Darw.DrawFilledRect(HealthX - 33, HealthY - 22, 30, 28, NNR3, 100, 0);
	Darw.DrawNewText(HealthX - 18, HealthY - 18.5f, NNR8, 16.f, u8"%s", PlayerTeamStr);


	//Darw.DrawFilledRect(HealthX + HealthMaxWidth + 3, HealthY - 22, 30, 28, NNR3, 100, 0);
	Darw.DrawNewText(HealthX + HealthMaxWidth + 10.5f, HealthY - 14.5f, NNR1, 16.f, u8"%2d", kill);

	Darw.DrawLine(HealthX, HealthY, HealthX + HealthMaxWidth, HealthY, NNR8, 4);
	Darw.DrawLine(HealthX, HealthY, HealthX + HealthWidth, HealthY, NNR7, 4);
}

bool Game::DecryptInit(uint64_t Encrypt)
{
	BYTE ShieIDcode[100] = { NULL };

	int DecryptID = 0;
	do {
		DecryptID++;
	} while (ReadBYTE(Encrypt + DecryptID, 3) != vector<BYTE>{ 72, 139, 193 });

	SIZE_T Legth = DecryptID - 3;
	uint64_t DecryptRax = Encrypt + Read<int>(Encrypt + 3) + 7;
	uint64_t DecryptAdd = (uint64_t)VirtualAlloc(0, Legth * 2, 4096, 64);
	if (!DecryptAdd)
		return false;
	*(BYTE*)DecryptAdd = 72;
	*(BYTE*)(DecryptAdd + 1) = 184;
	*(uint64_t*)(DecryptAdd + 2) = DecryptRax;

	ReadMemory(Encrypt + 7, ShieIDcode, Legth);
	CopyMemory((PVOID)(DecryptAdd + 10), ShieIDcode, Legth);

	Decrypt_Call = (Decrypt_)DecryptAdd;
	return Decrypt_Call != 0;
}

uint64_t Game::DecryptCall(uint64_t a1)
{
	return a1 != 0 ? Decrypt_Call(0, a1) : NULL;
}

DWORD Game::DecryptIndex(DWORD value)
{
	UINT32 v10, result;
	if (Offset::PZ == 1)
		v10 = _rotr(value ^ Offset::DecryptOne, Offset::TableOne);
	else
		v10 = _rotl(value ^ Offset::DecryptOne, Offset::TableOne);
	result = v10 ^ (v10 << Offset::TableTwo) ^ Offset::DecryptTwo;

	return result;
}

ULONG Game::DecryptID(uint64_t a1)
{
	return DecryptIndex(Read<int>(a1 + Offset::ObjID));
}

std::string Game::GetNames(DWORD ID)
{
	std::string emp = "Unknown";
	if (ID <= 0) return emp;
	uint32_t IdDiv = ID / Offset::IDD;
	uint32_t Idtemp = ID % Offset::IDD;
	uint64_t Serial = Read<uint64_t>(GNames + IdDiv * 0x8);
	if (!Serial || Serial < 0x100000)
		return emp;
	uint64_t pName = Read<uint64_t>(Serial + 0x8 * Idtemp);
	if (!pName || pName < 0x100000)
		return emp;
	StringA names = Read<StringA>(pName + 0x10);
	char te[64];
	memset(&te, 0, 64);
	if (memcmp(names.buffer, te, 64) == 0)
		return emp;
	std::string str(names.buffer);
	return str;
}
std::string Game::GetPlayName(uint64_t Ptr)
{
	std::string emp = "敌人";
	if (Ptr <= 0) return emp;
	uint64_t pName = Read<uint64_t>(Ptr + Offset::Playname);
	if (!pName || pName < 0x100000)
		return emp;
	StringA names = Read<StringA>(pName + 0x10);
	char te[64];
	memset(&te, 0, 64);
	if (memcmp(names.buffer, te, 64) == 0)
		return emp;
	std::string str(names.buffer);
	return str;
}

void Game::InitNameTypeMap()
{
	std::unordered_map<std::string, EntityNameIDTypeMap> EntityNameIDMap = std::unordered_map<std::string, EntityNameIDTypeMap>();

	/*判断类*/
	{
		EntityNameIDMap[("Chimera_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 1, "Chimera_Main");
		EntityNameIDMap[("Desert_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 2, "Desert_Main");
		EntityNameIDMap[("DihorOtok_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 3, "DihorOtok_Main");
		EntityNameIDMap[("Erangel_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 4, "Erangel_Main");
		EntityNameIDMap[("Range_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 5, "Range_Main");
		EntityNameIDMap[("Savage_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 6, "Savage_Main");
		EntityNameIDMap[("Summerland_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 7, "Summerland_Main");
		EntityNameIDMap[("Heaven_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 8, "Heaven_Main");
		EntityNameIDMap[("Kiki_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 9, "Kiki_Main");
		EntityNameIDMap[("Tiger_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 10, "Tiger_Main");
		EntityNameIDMap[("Neon_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 11, "Neon_Main");

		EntityNameIDMap[("Baltic_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 4, "Baltic_Main");
		EntityNameIDMap[("Tutorial_Main")] = EntityNameIDTypeMap(EntityType::XC0010, 0, "Tutorial_Main");
		EntityNameIDMap[("TslLobby_Persistent_Main")] = EntityNameIDTypeMap(EntityType::XC09, 0, "TslLobby_Persistent_Main");
	}

	/*主类*/
	{
		EntityNameIDMap[("RegistedPlayer")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Bot");
		EntityNameIDMap[("AIPawn_Base_C")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Bot");
		EntityNameIDMap[("AIPawn_Base_Female_C")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Bot");
		EntityNameIDMap[("AIPawn_Base_Male_C")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Bot");
		EntityNameIDMap[("AIPawn_Base_Male_C")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Bot");
		EntityNameIDMap[("UltAIPawn_Base_C")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Bot");
		EntityNameIDMap[("UltAIPawn_Base_Female_C")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Bot");
		EntityNameIDMap[("UltAIPawn_Base_Male_C")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Bot");
		EntityNameIDMap[("PlayerMale_A")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Player");
		EntityNameIDMap[("PlayerMale_A_C")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Player");
		EntityNameIDMap[("PlayerFemale_A")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Player");
		EntityNameIDMap[("PlayerFemale_A_C")] = EntityNameIDTypeMap(EntityType::EntityTypePlayer, NNR0, "Player");
		EntityNameIDMap[("DroppedItemGroup")] = EntityNameIDTypeMap(EntityType::EntityTypeItem, NNR0, nullptr);
	}
	/*XC06*/
	{
		EntityNameIDMap[("Weapon_Drone_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR2, "无人机侦察");
		EntityNameIDMap[("Uaz_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR4, "吉普");
		EntityNameIDMap[("BP_ATV_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR4, "四驱车");
		EntityNameIDMap[("Uaz_A_01")] = EntityNameIDTypeMap(EntityType::XC06, NNR4, "吉普");
		EntityNameIDMap[("Uaz_B_01")] = EntityNameIDTypeMap(EntityType::XC06, NNR4, "吉普");
		EntityNameIDMap[("Uaz_C_01")] = EntityNameIDTypeMap(EntityType::XC06, NNR4, "吉普");
		EntityNameIDMap[("Uaz_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR1, "吉普");
		EntityNameIDMap[("Uaz_B_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR1, "吉普");
		EntityNameIDMap[("Uaz_C_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR1, "吉普");
		EntityNameIDMap[("Dacia_A_01")] = EntityNameIDTypeMap(EntityType::XC06, NNR1, "轿车");
		EntityNameIDMap[("Dacia_A_02")] = EntityNameIDTypeMap(EntityType::XC06, NNR1, "轿车");
		EntityNameIDMap[("Dacia_A_03")] = EntityNameIDTypeMap(EntityType::XC06, NNR1, "轿车");
		EntityNameIDMap[("Dacia_A_01")] = EntityNameIDTypeMap(EntityType::XC06, NNR1, "轿车");
		EntityNameIDMap[("Dacia_A_04")] = EntityNameIDTypeMap(EntityType::XC06, NNR1, "轿车");
		EntityNameIDMap[("Buggy_A_01")] = EntityNameIDTypeMap(EntityType::XC06, NNR3, "蹦蹦");
		EntityNameIDMap[("Buggy_A_02")] = EntityNameIDTypeMap(EntityType::XC06, NNR3, "蹦蹦");
		EntityNameIDMap[("Buggy_A_03")] = EntityNameIDTypeMap(EntityType::XC06, NNR3, "蹦蹦");
		EntityNameIDMap[("Buggy_A_04")] = EntityNameIDTypeMap(EntityType::XC06, NNR3, "蹦蹦");
		EntityNameIDMap[("Buggy_A_05")] = EntityNameIDTypeMap(EntityType::XC06, NNR3, "蹦蹦");
		EntityNameIDMap[("Buggy_A_06")] = EntityNameIDTypeMap(EntityType::XC06, NNR3, "蹦蹦");
		EntityNameIDMap[("AquaRail_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR3, "摩艇");
		EntityNameIDMap[("BP_Drone_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR2, "无人机");
		EntityNameIDMap[("BP_Van_A_01")] = EntityNameIDTypeMap(EntityType::XC06, NNR4, "公交");
		EntityNameIDMap[("BP_Van_A_02")] = EntityNameIDTypeMap(EntityType::XC06, NNR4, "公交");
		EntityNameIDMap[("BP_Van_A_03")] = EntityNameIDTypeMap(EntityType::XC06, NNR6, "公交");
		EntityNameIDMap[("BP_Porter_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR6, "五菱");
		EntityNameIDMap[("BP_CoupeRB_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR6, "库佩");
		EntityNameIDMap[("Buggy_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR6, "蹦蹦");
		EntityNameIDMap[("Buggy_A_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR6, "蹦蹦");
		EntityNameIDMap[("Buggy_A_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR6, "蹦蹦");
		EntityNameIDMap[("Buggy_A_04_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR6, "蹦蹦");
		EntityNameIDMap[("Buggy_A_05_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR6, "蹦蹦");
		EntityNameIDMap[("Buggy_A_06_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR6, "蹦蹦");
		EntityNameIDMap[("Dacia_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "轿车");
		EntityNameIDMap[("Dacia_A_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "轿车");
		EntityNameIDMap[("Dacia_A_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "轿车");
		EntityNameIDMap[("Dacia_A_04_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "轿车");
		EntityNameIDMap[("Boat_PG117_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "游艇");
		EntityNameIDMap[("PG117_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "游艇");
		EntityNameIDMap[("BP_Niva_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "NIVA");
		EntityNameIDMap[("BP_Niva_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "NIVA");
		EntityNameIDMap[("BP_Niva_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "NIVA");
		EntityNameIDMap[("BP_Niva_04_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "NIVA");
		EntityNameIDMap[("BP_Niva_05_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "NIVA");
		EntityNameIDMap[("BP_Niva_06_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "NIVA");
		EntityNameIDMap[("BP_Dirtbike_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "越野摩托");
		EntityNameIDMap[("BP_Van_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "公交");
		EntityNameIDMap[("BP_Van_A_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "公交");
		EntityNameIDMap[("BP_Van_A_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR9, "公交");
		//EntityNameIDMap[("Dacia_A_01_v2")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "轿车");
		EntityNameIDMap[("Dacia_A_04_v2")] = EntityNameIDTypeMap(EntityType::XC06, NNR7, "轿车");
		EntityNameIDMap[("AquaRail_A_01")] = EntityNameIDTypeMap(EntityType::XC06, NNR7, "摩艇");
		//EntityNameIDMap[("BP_PonyCoupe_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "AE86");
		EntityNameIDMap[("BP_KillTruck_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR7, "KILLTRUCK");
		//EntityNameIDMap[("BP_LootTruck_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "物资车");
		EntityNameIDMap[("BP_TukTukTuk_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR7, "TUK");
		EntityNameIDMap[("Dacia_A_03_v2_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR7, "轿车");
		EntityNameIDMap[("Dacia_A_04_v2_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR7, "轿车");
		EntityNameIDMap[("BP_Motorbike_04")] = EntityNameIDTypeMap(EntityType::XC06, NNR7, "摩托");
		EntityNameIDMap[("AquaRail_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR7, "摩艇");
		EntityNameIDMap[("BP_Helicopter_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR7, "Helicopter");
		//EntityNameIDMap[("BP_Motorglider_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "滑翔机");
		EntityNameIDMap[("BP_M_Rony_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "跑车");
		EntityNameIDMap[("BP_M_Rony_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "罗尼");
		EntityNameIDMap[("BP_M_Rony_A_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "罗尼");
		EntityNameIDMap[("BP_M_Rony_A_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "罗尼");
		EntityNameIDMap[("ABP_Motorbike_03")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "摩托");
		EntityNameIDMap[("ABP_Motorbike_04")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "摩托");
		EntityNameIDMap[("BP_Mirado_A_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "跑车");
		EntityNameIDMap[("BP_Motorbike_04_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "摩托");
		EntityNameIDMap[("BP_Motorbike_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "摩托");
		EntityNameIDMap[("BP_Scooter_01_A_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR2, "SCOOTER");
		EntityNameIDMap[("BP_Scooter_02_A_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR2, "SCOOTER");
		EntityNameIDMap[("BP_Scooter_03_A_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR2, "SCOOTER");
		EntityNameIDMap[("BP_Scooter_04_A_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR2, "SCOOTER");
		EntityNameIDMap[("BP_Snowmobile_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR2, "SMOBILE");
		EntityNameIDMap[("BP_Snowmobile_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR2, "SMOBILE");
		EntityNameIDMap[("BP_Snowmobile_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR4, "SMOBILE");
		EntityNameIDMap[("ABP_Motorbike_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "摩托");
		EntityNameIDMap[("ABP_Motorbike_04_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "摩托");
		EntityNameIDMap[("BP_Mirado_Open_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "跑车");
		EntityNameIDMap[("BP_Mirado_Open_04_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "跑车");
		EntityNameIDMap[("BP_TukTukTuk_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "三轮");
		EntityNameIDMap[("BP_TukTukTuk_A_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "三轮");
		EntityNameIDMap[("BP_TukTukTuk_A_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "三轮");
		EntityNameIDMap[("BP_PickupTruck_A_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "皮卡");
		EntityNameIDMap[("BP_PickupTruck_A_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "皮卡");
		EntityNameIDMap[("BP_PickupTruck_A_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "皮卡");
		EntityNameIDMap[("BP_PickupTruck_A_04_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "皮卡");
		EntityNameIDMap[("BP_PickupTruck_A_05_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "皮卡");
		EntityNameIDMap[("BP_PickupTruck_B_01_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "皮卡");
		EntityNameIDMap[("BP_PickupTruck_B_02_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "皮卡");
		EntityNameIDMap[("BP_PickupTruck_B_03_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "皮卡");
		EntityNameIDMap[("BP_PickupTruck_B_04_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "皮卡");
		EntityNameIDMap[("BP_Motorbike_04_Desert")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "摩托");
		EntityNameIDMap[("ABP_Motorbike_04_Sidecar")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "摩托");
		EntityNameIDMap[("BP_Motorbike_04_Desert_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR0, "摩托");
		EntityNameIDMap[("BP_DummyTransportAircraft_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "AIRCRAFT");
		EntityNameIDMap[("BP_DummyTransportAircraft_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "AIRCRAFT");
		//EntityNameIDMap[("BP_Motorbike_04_SideCar_Desert")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "摩托");
		//EntityNameIDMap[("BP_Motorbike_04_SideCar_Desert_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "摩托");
		EntityNameIDMap[("BP_BRDM_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "装甲车");
		EntityNameIDMap[("Item_Mountainbike_C")] = EntityNameIDTypeMap(EntityType::XC06, NNR8, "自行车");
	}
	/*XC02*/
	{
		EntityNameIDMap[("Item_Weapon_K2_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|K2");
		EntityNameIDMap[("Item_Weapon_HK416")] = EntityNameIDTypeMap(EntityType::XC02, NNR0, "|M416");
		EntityNameIDMap[("Item_Weapon_AWM_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR0, "|AWM");
		EntityNameIDMap[("Item_Weapon_M24_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR0, "|M24");
		EntityNameIDMap[("Item_Weapon_SKS_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|SKS");
		EntityNameIDMap[("Item_Weapon_G36C_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|G36C");
		EntityNameIDMap[("Item_Weapon_M249_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|M249");
		EntityNameIDMap[("Item_Weapon_DP28_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|DP28");
		EntityNameIDMap[("Item_Weapon_Mk12_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|MK12");
		EntityNameIDMap[("Item_Weapon_Mk14_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|MK14");
		EntityNameIDMap[("Item_Weapon_AK47_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|AKM");
		EntityNameIDMap[("Item_Weapon_HK416_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR0, "|M416");
		EntityNameIDMap[("Item_Weapon_QBU88_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|QBU8");
		EntityNameIDMap[("Item_Weapon_M16A4_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|M16A");
		EntityNameIDMap[("Item_Weapon_QBZ95_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|QB95");
		EntityNameIDMap[("Item_Weapon_Groza_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR0, "|狗杂");
		EntityNameIDMap[("Item_Weapon_FNFal_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|FNFal");
		EntityNameIDMap[("Item_Weapon_Mosin_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|莫甘辛");
		EntityNameIDMap[("Item_Weapon_Mortar_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|迫击炮");
		EntityNameIDMap[("Item_Weapon_SCAR-L_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|SCAR-L");
		EntityNameIDMap[("Item_Weapon_Kar98k_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR0, "|Kar98k");
		EntityNameIDMap[("Item_Weapon_Mini14_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|Mini14");
		EntityNameIDMap[("Item_Weapon_FlareGun_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR0, "|信号枪");
		EntityNameIDMap[("Item_Weapon_BerylM762_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR0, "|M762");
		EntityNameIDMap[("Item_Weapon_Mk47Mutant_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|MK47");
		EntityNameIDMap[("Item_Weapon_ACE32_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR5, "|ACE32");
		EntityNameIDMap[("Item_Weapon_P90_C")] = EntityNameIDTypeMap(EntityType::XC02, NNR0, "|P90");
	}
	/*XC03*/
	{
		EntityNameIDMap[("Item_Attach_Weapon_Upper_CQBSS_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR0, "|八倍");
		EntityNameIDMap[("Item_Attach_Weapon_Upper_ACOG_01_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR0, "|四倍");
		EntityNameIDMap[("Item_Attach_Weapon_Upper_Scope6x_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR0, "|六倍");
		EntityNameIDMap[("Item_Attach_Weapon_Upper_PM2_01_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR0, "|十五倍");
		EntityNameIDMap[("Item_Attach_Weapon_Lower_Foregrip_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR1, "|垂直");
		EntityNameIDMap[("Item_Attach_Weapon_Upper_Holosight_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR1, "|全息");
		EntityNameIDMap[("Item_Attach_Weapon_Upper_DotSight_01_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR1, "|红点");
		EntityNameIDMap[("Item_Attach_Weapon_Stock_AR_Composite_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR1, "|枪托");
		EntityNameIDMap[("Item_Attach_Weapon_Muzzle_Suppressor_Large_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR1, "|枪消");
		EntityNameIDMap[("Item_Attach_Weapon_Magazine_Extended_Large_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR1, "|普扩");
		EntityNameIDMap[("Item_Attach_Weapon_Muzzle_Compensator_Large_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR0, "|枪补");
		EntityNameIDMap[("Item_Attach_Weapon_Stock_SniperRifle_CheekPad_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR1, "|托腮");
		EntityNameIDMap[("Item_Attach_Weapon_Magazine_Extended_SniperRifle_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR1, "|狙扩");
		EntityNameIDMap[("Item_Attach_Weapon_Muzzle_Suppressor_SniperRifle_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR0, "|狙消");
		EntityNameIDMap[("Item_Attach_Weapon_Muzzle_Compensator_SniperLarge_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR0, "|狙补");
		EntityNameIDMap[("Item_Attach_Weapon_Magazine_ExtendedQuickDraw_Large_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR0, "|快扩");
		EntityNameIDMap[("Item_Attach_Weapon_Magazine_ExtendedQuickDraw_SniperRifle_C")] = EntityNameIDTypeMap(EntityType::XC03, NNR0, "|狙扩");
	}
	/*XC04*/
	{
		EntityNameIDMap[("Item_Ghillie_01_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "|吉利服");
		EntityNameIDMap[("Item_Ghillie_02_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "|吉利服");
		EntityNameIDMap[("Item_Ghillie_03_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "[吉利服");
		EntityNameIDMap[("Item_Ghillie_04_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "[吉利服");
		EntityNameIDMap[("Item_Head_F_02_Lv2_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR3, "|二头盔");
		EntityNameIDMap[("Item_Head_F_01_Lv2_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR3, "|二头盔");
		EntityNameIDMap[("Item_Head_G_01_Lv3_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "|三头盔");
		EntityNameIDMap[("Item_Back_F_01_Lv2_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR3, "|二背包");
		EntityNameIDMap[("Item_Back_F_02_Lv2_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR3, "|二背包");
		EntityNameIDMap[("Item_Back_C_01_Lv3_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "|三背包");
		EntityNameIDMap[("Item_Back_C_02_Lv3_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "|三背包");
		EntityNameIDMap[("Item_Back_BlueBlocker")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "|二背包");
		EntityNameIDMap[("Item_Armor_D_01_Lv2_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR3, "|二级甲");
		EntityNameIDMap[("Item_Armor_C_01_Lv3_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "|三级甲");
		EntityNameIDMap[("Item_EmergencyPickup_C")] = EntityNameIDTypeMap(EntityType::XC04, NNR0, "|呼救器");

	}
	/*XC01*/
	{

		EntityNameIDMap[("Item_Heal_MedKit_C")] = EntityNameIDTypeMap(EntityType::XC01, NNR7, "|医疗箱");
		EntityNameIDMap[("Item_Heal_FirstAid_C")] = EntityNameIDTypeMap(EntityType::XC01, NNR7, "|急救包");
		EntityNameIDMap[("Item_Boost_PainKiller_C")] = EntityNameIDTypeMap(EntityType::XC01, NNR7, "|止疼");
		EntityNameIDMap[("Item_Boost_EnergyDrink_C")] = EntityNameIDTypeMap(EntityType::XC01, NNR7, "|饮料");
		EntityNameIDMap[("Item_Weapon_VendingMachine_C")] = EntityNameIDTypeMap(EntityType::XC01, NNR7, "|售货级");
		EntityNameIDMap[("Item_Boost_AdrenalineSyringe_C")] = EntityNameIDTypeMap(EntityType::XC01, NNR7, "|肾上腺素");

	}
	/*XC05*/
	{


		EntityNameIDMap[("Item_Weapon_Grenade_C")] = EntityNameIDTypeMap(EntityType::XC05, NNR8, "|手雷");
		EntityNameIDMap[("Weapon_StickyGrenade_C")] = EntityNameIDTypeMap(EntityType::XC05, NNR8, "|C4炸弹");
		EntityNameIDMap[("Item_Weapon_C4_C")] = EntityNameIDTypeMap(EntityType::XC05, NNR8, "|C4炸弹");
		EntityNameIDMap[("Item_Weapon_SmokeBomb_C")] = EntityNameIDTypeMap(EntityType::XC05, NNR8, "|烟雾弹");
		EntityNameIDMap[("Item_Weapon_Molotov_C")] = EntityNameIDTypeMap(EntityType::XC05, NNR8, "|燃烧弹");
		EntityNameIDMap[("Item_Weapon_BluezoneGrenade_C")] = EntityNameIDTypeMap(EntityType::XC05, NNR8, "|篮圈手雷");
		EntityNameIDMap[("ProjGrenade_C")] = EntityNameIDTypeMap(EntityType::ProGrenade, NNR0, "|小心手雷");
		EntityNameIDMap[("ProjC4_C")] = EntityNameIDTypeMap(EntityType::ProGrenade, NNR0, "小心C4");
		EntityNameIDMap[("ProjStickyGrenade_C")] = EntityNameIDTypeMap(EntityType::ProGrenade, NNR0, "|小心C4");
		EntityNameIDMap[("ProjFlashBang_C")] = EntityNameIDTypeMap(EntityType::ProGrenade, NNR0, "|小心闪光弹");
		EntityNameIDMap[("ProjMolotov_C")] = EntityNameIDTypeMap(EntityType::ProGrenade, NNR0, "|小心燃烧弹");
		EntityNameIDMap[("ProjBluezoneGrenade_C")] = EntityNameIDTypeMap(EntityType::ProGrenade, NNR0, "|小心篮圈手雷");
	}
	/*空投盒子*/
	{
		EntityNameIDMap[("DeathDropItemPackage_C")] = EntityNameIDTypeMap(EntityType::XC08, NNR4, "死亡盒子");
		EntityNameIDMap[("Carapackage_RedBox_C")] = EntityNameIDTypeMap(EntityType::XC08, NNR0, "空投箱子");
		EntityNameIDMap[("Carapackage_FlareGun_C")] = EntityNameIDTypeMap(EntityType::XC08, NNR0, "超级空投");
		EntityNameIDMap[("Carapackage_SmallPackage_C")] = EntityNameIDTypeMap(EntityType::XC08, NNR0, "小空投箱");
		EntityNameIDMap[("Carapackage_RedBox_COL_C")] = EntityNameIDTypeMap(EntityType::XC08, NNR0, "空投箱子");
	}

	std::unordered_map<std::string, EntityNameIDTypeMap>::iterator EntityTypeIt = std::unordered_map<std::string, EntityNameIDTypeMap>::iterator();

	for (ULONG i = 0; i < 0x100000; i++)
	{
		string Name = GetNames(i);

		EntityTypeIt = EntityNameIDMap.find(Name);

		if (EntityTypeIt != EntityNameIDMap.end())
		{
			EntityIDTypeMap[i] = EntityTypeIt->second;
			continue;
		}
	}
	return;
}

void Game::push_back(vector<Player>* desire, Player member)
{
	desire->push_back(member);
}

bool Game::back_End(vector<Player> desire, uint64_t ptr)
{
	if (desire.size() == NULL) { return false; }
	for (auto i = 0; i < desire.size(); i++)
	{
		if (desire[i].Ptr == ptr) { return true; }
	}
	return false;
}

void Game::GetAxes(Vector3 Rotation, Vector3* X, Vector3* Y, Vector3* Z)
{
	float radPitch = (Rotation.x * float(M_PI) / 180.f);
	float radYaw = (Rotation.y * float(M_PI) / 180.f);
	float radRoll = (Rotation.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	X->x = CP * CY;
	X->y = CP * SY;
	X->z = SP;

	Y->x = SR * SP * CY - CR * SY;
	Y->y = SR * SP * SY + CR * CY;
	Y->z = -SR * CP;

	Z->x = -(CR * SP * CY + SR * SY);
	Z->y = CY * SR - CR * SP * SY;
	Z->z = CR * CP;
}

void Game::SubVector(Vector3 VecA, Vector3 VecB, Vector3* VecC)
{
	VecC->x = VecA.x - VecB.x;
	VecC->y = VecA.y - VecB.y;
	VecC->z = VecA.z - VecB.z;
}

bool Game::WorldScreen(Vector3 Location, Vector2* Screen, float* Distance)
{
	Vector3 Temp = Vector3(0, 0, 0);
	float X, Y, Z;
	SubVector(Location, Pos, &Temp);
	Z = DotProduct(Temp, AxisX);
	if (Z < 100099) {
		if (Z > 1) {
			X = DotProduct(Temp, AxisY);
			Y = DotProduct(Temp, AxisZ);
			X = Darw.GameCenterX + X * Darw.GameCenterX / Tangent / Z;
			Y = Darw.GameCenterY - Y * Darw.GameCenterX / Tangent / Z;
			if (X <= 0 || Y <= 0) { return false; }
			Screen->x = X;
			Screen->y = Y;
			*Distance = Z / 100;
			return true;
		}
		return false;
	}
	return false;
}

float Game::DotProduct(Vector3 VecA, Vector3 VecB)
{
	return VecA.x * VecB.x + VecA.y * VecB.y + VecA.z * VecB.z;
}

void Game::GetBonePos(Vector3 VE, Vector3* Result)
{
	float X_, Y_, Z_, X, Y, Z, _X, _Y, _Z, _11, _12, _13, _21, _22, _23, _31, _32, _33;
	X = Matrix.Rotation_X + Matrix.Rotation_X;
	Y = Matrix.Rotation_Y + Matrix.Rotation_Y;
	Z = Matrix.Rotation_Z + Matrix.Rotation_Z;
	_X = Matrix.Rotation_X * X;
	_Y = Matrix.Rotation_Y * Y;
	_Z = Matrix.Rotation_Z * Z;
	_11 = (1 - (_Y + _Z)) * Matrix.Scale3D_X;
	_22 = (1 - (_X + _Z)) * Matrix.Scale3D_Y;
	_33 = (1 - (_X + _Y)) * Matrix.Scale3D_Z;
	_Z = Matrix.Rotation_Y * Z;
	_X = Matrix.Rotation_W * X;
	_32 = (_Z - _X) * Matrix.Scale3D_Z;
	_23 = (_Z + _X) * Matrix.Scale3D_Y;
	_Y = Matrix.Rotation_X * Y;
	_Z = Matrix.Rotation_W * Z;
	_21 = (_Y - _Z) * Matrix.Scale3D_Y;
	_12 = (_Y + _Z) * Matrix.Scale3D_X;
	_Y = Matrix.Rotation_W * Y;
	_Z = Matrix.Rotation_X * Z;
	_13 = (_Z - _Y) * Matrix.Scale3D_X;
	_31 = (_Z + _Y) * Matrix.Scale3D_Z;
	Result->x = VE.x * _11 + VE.y * _21 + VE.z * _31 + 1 * Matrix.Translation_X;
	Result->y = VE.x * _12 + VE.y * _22 + VE.z * _32 + 1 * Matrix.Translation_Y;
	Result->z = VE.x * _13 + VE.y * _23 + VE.z * _33 + 1 * Matrix.Translation_Z;
}


void Game::ipush_back(vector<ItemBase>* desire, ItemBase member)
{
	desire->push_back(member);
}

bool Game::iback_End(vector<ItemBase> desire, uint64_t ptr)
{
	if (desire.size() == NULL) { return false; }
	for (auto i = 0; i < desire.size(); i++)
	{
		if (desire[i].Ptr == ptr) { return true; }
	}
	return false;
}
