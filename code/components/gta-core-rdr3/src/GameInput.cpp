#include "StdInc.h"
#include "GameInput.h"

#include "atArray.h"
#include "CustomText.h"
#include "Hooking.h"
#include "Hooking.Stubs.h"

namespace rage
{
	struct MappingList
	{
		void* vtable;
		uint32_t m_Name;
		char pad[4];
		atArray<uint32_t> m_Categories;
	};
}

static std::vector<std::string> g_bindings = {
	"INPUT_TEST1",
};

static uint32_t HashBinding(const std::string& key)
{
	return HashString(key.c_str()) | 0x80000000;
}

static hook::cdecl_stub<void*(void*)> fwUIBindCtor([]()
{
	return hook::get_call(hook::get_pattern("E8 ? ? ? ? 4C 8B F8 EB ? 4C 8B FF 48 8B D3"));
});

static hook::cdecl_stub<void(int, uint32_t, char*)> addString([]()
{
	return hook::get_call(hook::get_pattern("E8 ? ? ? ? 48 8D 55 ? EB ? 48 8B D7"));
});

static hook::cdecl_stub<uint32_t(int, char*)> computeHash([]()
{
	return hook::get_call(hook::get_pattern("E8 ? ? ? ? 89 86 ? ? ? ? 45 33 C9"));
});

/*static hook::cdecl_stub<char*(void*, uint32_t)> nameFromValueUnsafe([]()
{
	return hook::get_call(hook::get_pattern("E8 ? ? ? ? 48 8B D0 B9 ? ? ? ? 48 8B F8 E8 ? ? ? ? 4C 8B C7 8B D0 B9 ? ? ? ? 8B D8 E8 ? ? ? ? 48 8B 4D"));
});*/

/*static hook::cdecl_stub<void(void*, void* void*)> registerBindingClient([]()
{
	
});*/

static void* g_inputTypeData = nullptr;

/*static void RegisterBind(std::string bind)
{
	void* v8 = operator new(472);
	auto v9 = fwUIBindCtor(v8);
	auto bindHash = HashBinding(bind);
	addString(8, bindHash, (char*)bind.c_str());
	registerBindingClient(HashString("PM_PANE_CFX"), v9)
}*/

static void (*g_origGetMappingCategories)(atArray<unsigned int>& outCategories);
static void GetMappingCategories(atArray<unsigned int>& outCategories)
{
	g_origGetMappingCategories(outCategories);
	outCategories.Set(outCategories.GetCount(), HashString("PM_PANE_CFX"));
	trace("Added custom mapping category PM_PANE_CFX\n");
}

static const char* (*g_origGetActionName)(uint32_t controlId);
static const char* GetActionName(uint32_t controlId)
{
	/*if (controlId & 0x80000000)
	{
		static std::string str;
		str = fmt::sprintf("INPUT_%08X", HashString(g_bindings[0]));

		std::string displayName;
		displayName = fmt::sprintf("%s", "danii");
		game::AddCustomText(HashString(str.c_str()), displayName);

		trace("Get action name for special control %08x: %s\n", controlId, str.c_str());
		return str.c_str();
	}*/
	return g_origGetActionName(controlId);
}

static void* (*g_origGetCategoryInputs)(uint32_t* categoryHash, atArray<uint32_t>& controlIds);
static void* GetCategoryInputs(uint32_t* categoryHash, atArray<uint32_t>& controlIds)
{
	trace("Get category inputs: %08x\n", *categoryHash);
	auto result = g_origGetCategoryInputs(categoryHash, controlIds);
	if (*categoryHash == HashString("PM_PANE_CFX"))
	{
		trace("Tried to get inputs for PM_PANE_CFX\n");
		for (const auto& id : g_bindings)
		{
			/*GetActionName(HashBinding(id));
			auto v14 = HashBinding(id);
			addString(1, v14, (char*)id.c_str());*/
			
			//controlIds.Set(controlIds.GetCount(), 100);
			controlIds.Set(controlIds.GetCount(), HashBinding(id));
		}
	}
	else
	{
		/*for (int i = 0; i < controlIds.GetCount(); i++)
		{
			if (controlIds.Get(i) == 100)
			{
				trace("Removing 100 from category %08x\n", *categoryHash);
				controlIds.Remove(i);
				i--;
			}
		}*/
	}
	return result;
}

static void*(*g_origGetBindingForControl)(void* control, void* outBinding, uint32_t controlId);
static void* GetBindingForControl(void* control, void* outBinding, uint32_t controlId)
{
	if (controlId & 0x80000000)
	{
		trace("Special binding for control %08x\n", controlId);

		//return outBinding;
	}

	return g_origGetBindingForControl(control, outBinding, controlId);
}

static bool (*g_origUpdateBindingState)(void*, uint32_t, void*);
static bool UpdateBindingState(void* self, uint32_t controlId, void* bitmask)
{
	trace("Update binding state: %d\n", controlId);

	return g_origUpdateBindingState(self, controlId, bitmask);
}


static const char* GetBindingName(uint32_t controlId)
{
	static thread_local char buf[64];
	strcpy_s(buf, "INPUT_TEST1");
	return buf;
}

static char* (*g_origNameFromValueUnsafe)(void* data, uint32_t index);
static char* GetNameFromValueUnsafe(void* data, uint32_t index)
{
	if (g_inputTypeData == data)
	{
		if(index > 700)
		{
			trace("Hooked nameFromValueUnsafe for index %d\n", index);
			static thread_local char buf[64];
			strcpy_s(buf, "INPUT_TEST1");
			return buf;
		}
	}

	return g_origNameFromValueUnsafe(data, index);
}

static void* (*g_origFindDeviceForInput)(int64_t thisPtr, uint64_t* outDevice, uint32_t inputId, uint8_t inputType, uint16_t deviceIndex, bool isAnalog);
static void* FindDeviceForInput(int64_t thisPtr, uint64_t* outDevice, uint32_t inputId, uint8_t inputType, uint16_t deviceIndex, bool isAnalog)
{
	auto result = g_origFindDeviceForInput(thisPtr, outDevice, inputId, inputType, deviceIndex, isAnalog);
	//trace("FindDeviceForInput: %d %p\n", inputId, (void*)outDevice);
	return result;
}

static uint32_t (*g_origGetInputByName)(char*);
static uint32_t GetInputByName(char* name)
{
	auto control = g_origGetInputByName(name);
	
	if(control == -1)
	{
		for (const auto& bind : g_bindings)
		{
			if (_stricmp(name, bind.c_str()) == 0)
			{
				control = HashBinding(bind);
				break;
			}
		}
	}

	return control;
}

static bool (*g_origIsInputConflicting)(atArray<int>&, uint32_t);
static bool IsInputConflicting(atArray<int>& categoryInputs, uint32_t controlId)
{
	//if(controlId & 0x80000000)
	{
		//trace("Returning false in conflict for custom input");
		return false;
	}
	return g_origIsInputConflicting(categoryInputs, controlId);
}

static bool (*g_origHandleInputConflicts)(void*, uint32_t);
static bool HandleInputConflicts(void* self, uint32_t controlId)
{
	trace("Input conflict: %d\n", controlId);
	//if (controlId & 0x80000000)
	{
		//trace("Returning false in handle conflict for custom input\n");
		return false;
	}
	return g_origHandleInputConflicts(self, controlId);
}

static uint64_t* (*g_origsub_140A3EF0C)(void*, void*, uint32_t, char, unsigned __int16, char);
static uint64_t* sub_140A3EF0C(void* thisPtr, void* outDevice, uint32_t inputId, char inputType, unsigned __int16 deviceIndex, char isAnalog)
{
	auto result = g_origsub_140A3EF0C(thisPtr, outDevice, inputId, inputType, deviceIndex, isAnalog);
	trace("sub_140A3EF0C: %d %p\n", inputId, (void*)outDevice);
	return result;
}

static atArray<rage::MappingList>* g_mappingLists = nullptr;

static HookFunction hookFunction([]()
{
	game::AddCustomText("INPUT_TEST1", "Test Binding 1");
	game::AddCustomText("KMS_INPUT_TEST1", "Test Binding 1");
	
	game::AddCustomText("PM_PANE_CFX", "RedM");
	g_origGetMappingCategories = hook::trampoline(hook::get_call(hook::get_pattern("E8 ? ? ? ? 48 8B 7C 24 ? 45 8B F4")), GetMappingCategories);
	g_origGetCategoryInputs = hook::trampoline(hook::get_call(hook::get_pattern("E8 ? ? ? ? 48 8B 5D ? 41 8B F5")), GetCategoryInputs);
	g_origGetActionName = hook::trampoline(hook::get_pattern("48 63 D1 48 8D 0D ? ? ? ? E9"), GetActionName);
	g_origGetBindingForControl = hook::trampoline(hook::get_pattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 41 56 48 83 EC ? 48 8D 99 ? ? ? ? 41 8B F0"), GetBindingForControl);
	g_origGetInputByName = hook::trampoline(hook::get_pattern("48 83 EC ? 48 8B D1 45 33 C9"), GetInputByName);
	
	//g_origIsInputConflicting = hook::trampoline(hook::get_pattern("48 89 74 24 ? 48 89 7C 24 ? 4C 89 74 24 ? 4C 8B 09"), IsInputConflicting);
	//g_origHandleInputConflicts = hook::trampoline(hook::get_pattern("48 8B C4 48 89 58 ? 48 89 68 ? 48 89 70 ? 48 89 78 ? 41 54 41 56 41 57 48 83 EC ? 4C 8B 1D ? ? ? ? 40 32 ED"), HandleInputConflicts);
	//g_origUpdateBindingState = hook::trampoline(hook::get_call(hook::get_pattern("E8 ? ? ? ? 48 83 C3 ? 48 FF C6 48 3B F7 75 ? 48 8B 5D")), UpdateBindingState);
	//g_origFindDeviceForInput = hook::trampoline(hook::get_call(hook::get_pattern("E8 ? ? ? ? 44 8A 0D ? ? ? ? 48 8D 54 24")), FindDeviceForInput);
	//g_origsub_140A3EF0C = hook::trampoline(hook::get_call(hook::get_pattern("E8 ? ? ? ? 48 8B 44 24 ? 48 85 C0 74 ? 48 8B 48 ? BA")), sub_140A3EF0C);

	g_origNameFromValueUnsafe = hook::trampoline(hook::get_call(hook::get_pattern(
		"E8 ? ? ? ? 33 D2 48 8B C8 E8 ? ? ? ? 48 8B 4F"
	)), GetNameFromValueUnsafe);

	g_inputTypeData = hook::get_address<void*>(hook::get_pattern("48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B D0 B9 ? ? ? ? 48 8B F8 E8 ? ? ? ? 4C 8B C7 8B D0 B9 ? ? ? ? 8B D8 E8 ? ? ? ? 48 8B 4D", 0x3));
	trace("g_inputTypeData: %p\n", (void*)g_inputTypeData);

	g_mappingLists = hook::get_address<atArray<rage::MappingList>*>(hook::get_pattern("4C 8B 05 ? ? ? ? 49 C1 E1", 0x3));
	trace("g_mappingLists: %p\n", (void*)g_mappingLists);

	//hook::nop(hook::get_pattern("E8 ? ? ? ? 48 8B 4D ? 4D 8B C5"), 5); // HashString

	// Allow duplicated
	hook::nop(hook::get_pattern("E8 ? ? ? ? 48 8D 4D ? BA ? ? ? ? 48 2B CB"), 5);

	hook::nop(hook::get_pattern("74 ? 41 8D 57 ? 44 89 7C 24"), 0x2);

	// UpdateBindingState
	// Add a stub to return the custom binding name if the 8 bit is set
	{
		auto location = hook::get_pattern("4C 8B F8 48 85 C0 0F 84 ? ? ? ? 4C 8B C0 C6 45");

		static struct : jitasm::Frontend
		{
			uintptr_t originalLocation;
			uintptr_t customLocation;

			void Init(uintptr_t original)
			{
				originalLocation = original;
			}

			virtual void InternalMain() override
			{
				mov(r15, rax);
				test(edi, 0x80000000);
				jz("NotCustom");

				movsxd(rcx, edi);
				mov(rax, (uintptr_t)GetBindingName);
				call(rax);
				mov(r15, rax);

				L("NotCustom");
				test(rax, rax);
				mov(rcx, originalLocation);
				jmp(rcx);
			}
		} stub;
		
		hook::nop(location, 0x6);
		stub.Init((uintptr_t)location + 0x6);
		hook::jump_rcx(location, stub.GetCode());
	}

	// UpdateBindingState
	// Remove the bitmask use for custom bindings, this bitmask determines if a input is already registered, valid, etc.
	{
		auto location = hook::get_pattern("0F B6 C8 48 8B C7 48 C1 E8");
		auto skipBitmask = hook::get_pattern("48 8b 4c 24 38 83 cf ff 48 85 c9");
		
		static struct : jitasm::Frontend
		{
			uintptr_t originalLocation;
			uintptr_t customLocation;

			void Init(uintptr_t original, uintptr_t custom)
			{
				originalLocation = original;
				customLocation = custom;
			}

			virtual void InternalMain() override
			{
				movzx(ecx, al);
				mov(rax, rdi);
				
				test(edi, 0x80000000);
				jz("NotCustom");
				mov(rbx, customLocation);
				jmp(rbx);

				L("NotCustom");
				mov(rbx, originalLocation);
				jmp(rbx);
			}
		} stub;
		
		hook::nop(location, 0x6);
		stub.Init((uintptr_t)location + 0x6, (uintptr_t)skipBitmask);
		hook::jump_rcx(location, stub.GetCode());
	}
});