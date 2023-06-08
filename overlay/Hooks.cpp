#include <Include.hpp>

//
// Manager/Helper Functions
// 

void hooks::register_hk(const std::string& name, const hooks::data_t& data, uint32_t offset, bool resolve_jmp)
{
	auto& entry = data::list[name];
	entry = data;

	if (offset)
		entry.m_target = 
		mem::addr(entry.m_target)
		.Add(offset)
		.Ptr();

	if (resolve_jmp)
	{
		uintptr_t resolved_target{};

		if (!shared::user::zydis::resolve_jump(entry.m_target, resolved_target, true))
			return;

		entry.m_target =
			mem::addr(resolved_target).Ptr();
	}
}

void hooks::register_()
{
	register_hk(
		_XS("get_proc_address"), 
		{
			0,
			mem::addr(&GetProcAddress).Ptr(),
			&get_proc_address,
			nullptr,
		},
		0x4, true
	);

	register_hk(
		_XS("dbg_ui_remote_break_in"),
		{
			0,
			Util::find_export<void*>(_XS("ntdll.dll"), _XS("DbgUiRemoteBreakin")),
			&dbg_ui_remote_break_in,
			nullptr,
		},
		0x4, true
		);
}

bool hooks::initialize()
{
	register_();

	if (data::list.empty())
		return false;

	auto result = true;

	for (auto& entry : data::list)
	{
		if (!place(entry.second))
		{
			result = false;
			break;
		}
	}

	return result;
}

bool hooks::place(data_t& data)
{
	if (!shared::hook::Place(
		data.m_target,
		data.m_handler,
		&data.m_original,
		&data.m_backup_length
	))
		return false;

	return true;
}

//

//
// Function Handlers
//

void __fastcall hooks::dbg_ui_remote_break_in()
{
	auto info = integrity::violation_info_t{};
	info.m_severe = true;
	info.m_id = 0xB04;
	integrity::ReportViolation(info);

	SuspendThread(GetCurrentThread());
}

void* __fastcall hooks::get_proc_address(void* handle, const char* name)
{
	static const auto hk_data = data::list[_XSS("get_proc_address")];
	static auto* const hk_orig = reinterpret_cast<decltype(get_proc_address)*>(hk_data.m_original);

	const auto hk_result = hk_orig(handle, name);

	if (mem::addr(handle).Base() == 'HsHB')
		return G::ExportTable[name];
	
	return hk_result;
}

//
