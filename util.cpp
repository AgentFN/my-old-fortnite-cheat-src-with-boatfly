
/*

Free open SRC thisisrico.de | Discord ThisIsRico#5872

*/


#include "stdafx.h"

namespace Util {

#define URotationToRadians( URotation )		( ( URotation ) * ( PI / 32768.0f ) ) 
#define URotationToDegree( URotation )		( ( URotation ) * ( 360.0f / 65536.0f ) ) 

#define DegreeToURotation( Degree )			( ( Degree ) * ( 65536.0f / 360.0f ) )
#define DegreeToRadian( Degree )			( ( Degree ) * ( PI / 180.0f ) )

#define RadianToURotation( URotation )		( ( URotation ) * ( 32768.0f / PI ) ) 
#define RadianToDegree( Radian )			( ( Radian ) * ( 180.0f / PI ) )

	uint64_t DiscordBaseAddress = 0;
	GObjects* objects = nullptr;
	FString(*GetObjectNameInternal)(PVOID) = nullptr;
	VOID(*FreeInternal)(PVOID) = nullptr;
	BOOL(*LineOfSightToInternal)(PVOID PlayerController, PVOID Actor, FVector* ViewPoint) = nullptr;
	VOID(*CalculateProjectionMatrixGivenView)(FMinimalViewInfo* viewInfo, BYTE aspectRatioAxisConstraint, PBYTE viewport, FSceneViewProjectionData* inOutProjectionData) = nullptr;

	struct {
		FMinimalViewInfo Info;
		float ProjectionMatrix[4][4];
	} view = { 0 };

	VOID CreateConsole() {
		AllocConsole();
		static_cast<VOID>(freopen(xorstr("CONIN$"), xorstr("r"), stdin));
		static_cast<VOID>(freopen(xorstr("CONOUT$"), xorstr("w"), stdout));
		static_cast<VOID>(freopen(xorstr("CONOUT$"), xorstr("w"), stderr));
	}

	BOOLEAN MaskCompare(PVOID buffer, LPCSTR pattern, LPCSTR mask) {
		for (auto b = reinterpret_cast<PBYTE>(buffer); *mask; ++pattern, ++mask, ++b) {
			if (*mask == 'x' && *reinterpret_cast<LPCBYTE>(pattern) != *b) {
				return FALSE;
			}
		}

		return TRUE;
	}

	PBYTE FindPattern(PVOID base, DWORD size, LPCSTR pattern, LPCSTR mask) {
		size -= static_cast<DWORD>(strlen(mask));

		for (auto i = 0UL; i < size; ++i) {
			auto addr = reinterpret_cast<PBYTE>(base) + i;
			if (MaskCompare(addr, pattern, mask)) {
				return addr;
			}
		}

		return NULL;
	}

	PBYTE FindPattern(LPCSTR pattern, LPCSTR mask) {
		MODULEINFO info = { 0 };
		GetModuleInformation(GetCurrentProcess(), GetModuleHandle(0), &info, sizeof(info));

		return FindPattern(info.lpBaseOfDll, info.SizeOfImage, pattern, mask);
	}

	bool DataCompare(PBYTE pData, PBYTE bSig, char* szMask)
	{
		for (; *szMask; ++szMask, ++pData, ++bSig)
		{
			if (*szMask == 'x' && *pData != *bSig)
				return false;
		}
		return (*szMask) == 0;
	}

	short DiscordGetAsyncKeyState(const int vKey)
	{
		static PBYTE addrGetAsyncKeyState = NULL;

		if (!addrGetAsyncKeyState)
		{
			addrGetAsyncKeyState = FindSignature((BYTE*)DiscordBaseAddress, 0x10000000, (BYTE*)xorstr("\x40\x53\x48\x83\xEC\x20\x8B\xD9\xFF\x15\x00\x00\x00\x00"), xorstr("xxxxxxxxxx????"), 0);
		}

		if (!addrGetAsyncKeyState)
			return false;

		using GetAsyncKeyState_t = short(__fastcall*)(int);
		auto fnGetAyncKeyState = (GetAsyncKeyState_t)addrGetAsyncKeyState;

		return fnGetAyncKeyState(vKey);
	}

	bool DiscordCreateHook(PVOID originalPresent, PVOID hookFunction, PVOID pOriginal)
	{
		static PBYTE addrCreateHook = NULL;

		if (!addrCreateHook)
		{
			addrCreateHook = FindSignature((BYTE*)DiscordBaseAddress, 0x10000000, (BYTE*)xorstr("\x40\x53\x55\x56\x57\x41\x54\x41\x56\x41\x57\x48\x83\xEC\x60"), xorstr("xxxxxxxxxxxxxxx"), 0);
		}

		if (!addrCreateHook)
			return false;

		using CreateHook_t = uint64_t(__fastcall*)(LPVOID, LPVOID, LPVOID*);
		auto fnCreateHook = (CreateHook_t)addrCreateHook;

		return fnCreateHook((void*)originalPresent, (void*)hookFunction, (void**)pOriginal) == 0 ? true : false;
	}

	bool DiscordEnableHook(PVOID pTarget, bool toggle)
	{
		static PBYTE addrEnableHook = NULL;

		if (!addrEnableHook)
		{
			addrEnableHook = FindSignature((BYTE*)DiscordBaseAddress, 0x10000000, (BYTE*)xorstr("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x74\x24\x00\x57\x41\x56\x41\x57\x48\x83\xEC\x20\x33\xF6\x8B\xFA"), xorstr("xxxx?xxxx?xxxx?xxxxxxxxxxxxx"), 0);
		}

		if (!addrEnableHook)
			return false;

		using EnableHook_t = uint64_t(__fastcall*)(LPVOID, bool);
		auto fnEnableHook = (EnableHook_t)addrEnableHook;

		return fnEnableHook((void*)pTarget, toggle) == 0 ? true : false;
	}

	bool DiscordEnableHookQue()
	{
		static PBYTE addrEnableHookQueu = NULL;

		if (!addrEnableHookQueu)
		{
			addrEnableHookQueu = FindSignature((BYTE*)DiscordBaseAddress, 0x10000000, (BYTE*)xorstr("\x48\x89\x5C\x24\x00\x48\x89\x6C\x24\x00\x48\x89\x7C\x24\x00\x41\x57"), xorstr("xxxx?xxxx?xxxx?xx"), 0);
		}

		if (!addrEnableHookQueu)
			return false;

		using EnableHookQueu_t = uint64_t(__stdcall*)(VOID);
		auto fnEnableHookQueu = (EnableHookQueu_t)addrEnableHookQueu;

		return fnEnableHookQueu() == 0 ? true : false;
	}

	PBYTE FindSignature(PBYTE dwAddress, DWORD dwSize, PBYTE pbSig, char* szMask, long offset)
	{
		size_t length = strlen(szMask);
		for (size_t i = NULL; i < dwSize - length; i++)
		{
			if (DataCompare(dwAddress + i, pbSig, szMask))
				return dwAddress + i + offset;
		}
		return nullptr;
	}

	VOID Free(PVOID buffer) {
		FreeInternal(buffer);
	}

	std::wstring GetObjectFirstName(UObject* object) {
		auto internalName = GetObjectNameInternal(object);
		if (!internalName.c_str()) {
			return L"";
		}

		std::wstring name(internalName.c_str());
		Free(internalName.c_str());

		return name;
	}

	std::wstring GetObjectName(UObject* object) {
		std::wstring name(L"");
		for (auto i = 0; object; object = object->Outer, ++i) {
			auto internalName = GetObjectNameInternal(object);
			if (!internalName.c_str()) {
				break;
			}

			name = internalName.c_str() + std::wstring(i > 0 ? L"." : L"") + name;
			Free(internalName.c_str());
		}

		return name;
	}

	PVOID FindObject(LPCWSTR name) {
		for (auto array : objects->ObjectArray->Objects) {
			auto fuObject = array;
			for (auto i = 0; i < 0x10000 && fuObject->Object; ++i, ++fuObject)
			{
				auto object = fuObject->Object;

				if (object->ObjectFlags != 0x41) {
					continue;
				}

				if (GetObjectName(object) == name) {
					return object;
				}
			}
		}
		return 0;
	}

	VOID ToMatrixWithScale(float* in, float out[4][4]) {
		auto* rotation = &in[0];
		auto* translation = &in[4];
		auto* scale = &in[8];

		out[3][0] = translation[0];
		out[3][1] = translation[1];
		out[3][2] = translation[2];

		auto x2 = rotation[0] + rotation[0];
		auto y2 = rotation[1] + rotation[1];
		auto z2 = rotation[2] + rotation[2];

		auto xx2 = rotation[0] * x2;
		auto yy2 = rotation[1] * y2;
		auto zz2 = rotation[2] * z2;
		out[0][0] = (1.0f - (yy2 + zz2)) * scale[0];
		out[1][1] = (1.0f - (xx2 + zz2)) * scale[1];
		out[2][2] = (1.0f - (xx2 + yy2)) * scale[2];

		auto yz2 = rotation[1] * z2;
		auto wx2 = rotation[3] * x2;
		out[2][1] = (yz2 - wx2) * scale[2];
		out[1][2] = (yz2 + wx2) * scale[1];

		auto xy2 = rotation[0] * y2;
		auto wz2 = rotation[3] * z2;
		out[1][0] = (xy2 - wz2) * scale[1];
		out[0][1] = (xy2 + wz2) * scale[0];

		auto xz2 = rotation[0] * z2;
		auto wy2 = rotation[3] * y2;
		out[2][0] = (xz2 + wy2) * scale[2];
		out[0][2] = (xz2 - wy2) * scale[0];

		out[0][3] = 0.0f;
		out[1][3] = 0.0f;
		out[2][3] = 0.0f;
		out[3][3] = 1.0f;
	}

	VOID MultiplyMatrices(float a[4][4], float b[4][4], float out[4][4]) {
		for (auto r = 0; r < 4; ++r) {
			for (auto c = 0; c < 4; ++c) {
				auto sum = 0.0f;

				for (auto i = 0; i < 4; ++i) {
					sum += a[r][i] * b[i][c];
				}

				out[r][c] = sum;
			}
		}
	}



	VOID GetBoneLocation(float compMatrix[4][4], PVOID bones, DWORD index, float out[3]) {
		float boneMatrix[4][4];
		ToMatrixWithScale((float*)((PBYTE)bones + (index * 0x30)), boneMatrix);

		float result[4][4];
		MultiplyMatrices(boneMatrix, compMatrix, result);

		out[0] = result[3][0];
		out[1] = result[3][1];
		out[2] = result[3][2];
	}

	VOID GetViewProjectionMatrix(FSceneViewProjectionData* projectionData, float out[4][4]) {
		auto loc = &projectionData->ViewOrigin;

		float translation[4][4] = {
			{ 1.0f, 0.0f, 0.0f, 0.0f, },
			{ 0.0f, 1.0f, 0.0f, 0.0f, },
			{ 0.0f, 0.0f, 1.0f, 0.0f, },
			{ -loc->X, -loc->Y, -loc->Z, 0.0f, },
		};

		float temp[4][4];
		MultiplyMatrices(translation, projectionData->ViewRotationMatrix.M, temp);
		MultiplyMatrices(temp, projectionData->ProjectionMatrix.M, out);
	}

	BOOLEAN ProjectWorldToScreen(float viewProjection[4][4], float width, float height, float inOutPosition[3]) {
		float res[4] = {
			viewProjection[0][0] * inOutPosition[0] + viewProjection[1][0] * inOutPosition[1] + viewProjection[2][0] * inOutPosition[2] + viewProjection[3][0],
			viewProjection[0][1] * inOutPosition[0] + viewProjection[1][1] * inOutPosition[1] + viewProjection[2][1] * inOutPosition[2] + viewProjection[3][1],
			viewProjection[0][2] * inOutPosition[0] + viewProjection[1][2] * inOutPosition[1] + viewProjection[2][2] * inOutPosition[2] + viewProjection[3][2],
			viewProjection[0][3] * inOutPosition[0] + viewProjection[1][3] * inOutPosition[1] + viewProjection[2][3] * inOutPosition[2] + viewProjection[3][3],
		};

		auto r = res[3];
		if (r > 0) {
			auto rhw = 1.0f / r;

			inOutPosition[0] = (((res[0] * rhw) / 2.0f) + 0.5f) * width;
			inOutPosition[1] = (0.5f - ((res[1] * rhw) / 2.0f)) * height;
			inOutPosition[2] = r;

			return TRUE;
		}

		return FALSE;
	}

	VOID CalculateProjectionMatrixGivenViewHook(FMinimalViewInfo* viewInfo, BYTE aspectRatioAxisConstraint, PBYTE viewport, FSceneViewProjectionData* inOutProjectionData) {
		CalculateProjectionMatrixGivenView(viewInfo, aspectRatioAxisConstraint, viewport, inOutProjectionData);

		view.Info = *viewInfo;
		GetViewProjectionMatrix(inOutProjectionData, view.ProjectionMatrix);
	}

	BOOLEAN WorldToScreen(float width, float height, float inOutPosition[3]) {
		return ProjectWorldToScreen(view.ProjectionMatrix, width, height, inOutPosition);
	}

	BOOLEAN LineOfSightTo(PVOID PlayerController, PVOID Actor, FVector* ViewPoint) {
		return SpoofCall(LineOfSightToInternal, PlayerController, Actor, ViewPoint);
	}

	FMinimalViewInfo& GetViewInfo() {
		return view.Info;
	}

	FVector* GetPawnRootLocation(PVOID pawn) {
		auto root = ReadPointer(pawn, offsets::Actor::RootComponent);
		if (!root) {
			return nullptr;
		}

		return reinterpret_cast<FVector*>(reinterpret_cast<PBYTE>(root) + offsets::SceneComponent::RelativeLocation);
	}

	FVector2D WorldToRadar(FVector Location, INT RadarPositionX, INT RadarPositionY, int Size, int RadarScale)
	{
		FVector2D Return;

		FLOAT CosYaw = cos(URotationToRadians(GetViewInfo().Rotation.Yaw));
		FLOAT SinYaw = sin(URotationToRadians(GetViewInfo().Rotation.Yaw));

		FLOAT DeltaX = Location.X - GetViewInfo().Location.X;
		FLOAT DeltaY = Location.Y - GetViewInfo().Location.Y;

		FLOAT LocationX = (DeltaY * CosYaw - DeltaX * SinYaw) / (RadarScale + 25);
		FLOAT LocationY = (DeltaX * CosYaw + DeltaY * SinYaw) / (RadarScale + 25);

		if (LocationX > ((Size / 2) - 5.0f) - 2.5f)
			LocationX = ((Size / 2) - 5.0f) - 2.5f;
		else if (LocationX < -(((Size / 2) - 5.0f) - 2.5f))
			LocationX = -(((Size / 2) - 5.0f) - 2.5f);

		if (LocationY > ((Size / 2) - 5.0f) - 2.5f)
			LocationY = ((Size / 2) - 5.0f) - 2.5f;
		else if (LocationY < -(((Size / 2) - 5.0f) - 2.5f))
			LocationY = -(((Size / 2) - 5.0f) - 2.5f);

		Return.X = LocationX + RadarPositionX;
		Return.Y = -LocationY + RadarPositionY;

		return Return;
	}


	float Normalize(float angle) {
		float a = (float)fmod(fmod(angle, 360.0) + 360.0, 360.0);
		if (a > 180.0f) {
			a -= 360.0f;
		}
		return a;
	}




	VOID CalcAngle(float* src, float* dst, float* angles) {
		float rel[3] = {
			dst[0] - src[0],
			dst[1] - src[1],
			dst[2] - src[2],
		};

		auto dist = sqrtf(rel[0] * rel[0] + rel[1] * rel[1] + rel[2] * rel[2]);
		auto yaw = atan2f(rel[1], rel[0]) * (180.0f / PI);
		auto pitch = (-((acosf((rel[2] / dist)) * 180.0f / PI) - 90.0f));

		angles[0] = Normalize(pitch);
		angles[1] = Normalize(yaw);
	}

	BOOLEAN Initialize()
	{

		// GObjects
		auto addr = FindPattern(xorstr("\x48\x8B\x05\x00\x00\x00\x00\x48\x8B\x0C\xC8\x48\x8B\x04\xD1"), xorstr("xxx????xxxxxxxx"));
		//auto addr = FindPattern(xorstr(""), xorstr(""));
		objects = reinterpret_cast<decltype(objects)>(RELATIVE_ADDR(addr, 7));
		if (!addr) {
			MessageBox(0, L"GObjects", L"Failure", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		
		//          48 8B 05 7E 38 79 05 4C 8D 34 CD
		// GetObjectName
		addr = FindPattern(xorstr("\x48\x89\x5C\x24\x00\x48\x89\x74\x24\x00\x55\x57\x41\x56\x48\x8D\xAC\x24\x00\x00\x00\x00\x48\x81\xEC\x00\x00\x00\x00\x48\x8B\x05\x00\x00\x00\x00\x48\x33\xC4\x48\x89\x85\x00\x00\x00\x00\x45\x33\xF6\x48\x8B\xF2\x44\x39\x71\x04\x0F\x85\x00\x00\x00\x00\x8B\x19\x0F\xB7\xFB\xE8\x00\x00\x00\x00\x8B\xCB\x48\x8D\x54\x24"), xorstr("xxxxx????xxxxxxxxxxxxxxxx????xxxxx????xxxxxxxxxxxxx????xxx"));
		GetObjectNameInternal = reinterpret_cast<decltype(GetObjectNameInternal)>(addr);
		if (!addr) {
			MessageBox(0, L"GetObjectName", L"Failure", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		// Free
		addr = FindPattern(xorstr("\x48\x85\xC9\x0F\x84\x00\x00\x00\x00\x53\x48\x83\xEC\x20\x48\x89\x7C\x24\x30\x48\x8B\xD9\x48\x8B\x3D\x00\x00\x00\x00\x48\x85\xFF\x0F\x84\x00\x00\x00\x00\x48\x8B\x07\x4C\x8B\x40\x30\x48\x8D\x05\x00\x00\x00\x00\x4C\x3B\xC0"), xorstr("xxxxx????xxxxxxxxxxxxxxxx????xxxxx????xxxxxxxxxxxxx????xxx"));
		FreeInternal = reinterpret_cast<decltype(FreeInternal)>(addr);

		// CalculateProjectionMatrixGivenView
		addr = FindPattern(xorstr("\xE8\x00\x00\x00\x00\x41\x88\x07\x48\x83\xC4\x30"), xorstr("x????xxxxxxx"));
		addr -= 0x280;
		MH_CreateHook(addr, CalculateProjectionMatrixGivenViewHook, (PVOID*)&CalculateProjectionMatrixGivenView);
		MH_EnableHook(addr);
		//  45 0F 57 C0 45 8B 81 ? ? ? ? ? ? ? ? ? ? ? ?

		// LineOfSightTo
		addr = FindPattern(xorstr("\xE8\x00\x00\x00\x00\x48\x8B\x0D\x00\x00\x00\x00\x33\xD2\x40\x8A\xF8"), xorstr("x????xxx????xxxxx"));
		LineOfSightToInternal = reinterpret_cast<decltype(LineOfSightToInternal)>(addr);

		return TRUE;
	}
}
