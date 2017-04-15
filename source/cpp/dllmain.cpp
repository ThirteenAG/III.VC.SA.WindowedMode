#include "dxhandler.h"

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		CDxHandler::ProcessIni();

		if (injector::address_manager::singleton().IsIII())
		{
			CDxHandler::SetupHooksIII();
		}
		else
		{
			if (injector::address_manager::singleton().IsVC())
			{
				CDxHandler::SetupHooksVC();
			}
			else
			{
				if (injector::address_manager::singleton().IsSA())
				{
					CDxHandler::SetupHooksSA();
				}
			}
		}
	}

	if (reason == DLL_PROCESS_DETACH)
	{
		if (CDxHandler::ShExecInfo.hProcess != nullptr)
			TerminateProcess(CDxHandler::ShExecInfo.hProcess, 0);
	}
	return TRUE;
}