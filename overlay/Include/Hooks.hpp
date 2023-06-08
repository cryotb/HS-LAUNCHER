#pragma once

namespace hooks
{
	struct data_t
	{
		SIZE_T m_backup_length;
		void* m_target;
		void* m_handler;
		void* m_original;
	};

	namespace data
	{
		inline std::map<std::string, data_t> list{};
	}

	extern void register_();
	extern void register_hk(const std::string& name, const hooks::data_t& data, uint32_t offset = 0x00, bool resolve_jmp = false);
	extern bool initialize();

	extern bool place(data_t& data);

	extern void __fastcall dbg_ui_remote_break_in();
	extern void* __fastcall get_proc_address(void* handle, const char* name);
}
