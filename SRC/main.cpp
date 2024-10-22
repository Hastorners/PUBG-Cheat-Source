#include "main.h"

int main()
{
	MSG Msg;

	AllocConsole();
	freopen_s((_iobuf**)__acrt_iob_func(1), "conout$", "w", (_iobuf*)__acrt_iob_func(1));

	Game ov1 = Game();
	ov1.Start();
	while (::GetMessage(&Msg, 0, 0, 0))
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		::TranslateMessage(&Msg);
		::DispatchMessage(&Msg);
	}
	exit(0);
	return Msg.wParam;
}
