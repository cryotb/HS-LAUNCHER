class C_WindWalker final
{
private:
	struct Wind_t
	{
		Wind_t()
		{
			Handle = nullptr;
			Info = WINDOWINFO{};
			FlagsLg = {};
			FlagsEx = {};
			OwnerProcId = {};
			ClassName = {};
			Name = std::string{};
		}

		HWND Handle;
		WINDOWINFO Info;
		RECT Bounds;
		LONG FlagsLg;
		LONG FlagsEx;
		DWORD OwnerProcId;
		std::string Name;
		std::string OwnerProcName;
		std::string ClassName;
	};
public:
	C_WindWalker() = default;
	~C_WindWalker() = default;

	BOOLEAN Populate();
	VOID PostProcess();
	[[nodiscard]] SIZE_T Count() CONST { return m_List.size(); }

	VOID Dump();
	std::optional<Wind_t> FindViableWindow();
	std::optional<Wind_t> FindWindowByName(const std::string& name, Math::range_t<INT> range = { 0, 0 });
	std::optional<Wind_t> FindWindowInProcess(const std::string& name, Math::range_t<INT> range = { 0, 0 });
private:
	typedef std::unique_lock<std::recursive_mutex> MutexLock_t;
	inline static std::vector<Wind_t> m_List;

	static BOOL CALLBACK EnumWindProc(HWND hWnd, LPARAM lParam)
	{
		CHAR pszName[MAX_PATH] = { 0 };
		RECT sBounds{};
		WINDOWINFO sInfo{};

		GetWindowTextA(hWnd, pszName, sizeof(pszName));
		GetWindowRect(hWnd, &sBounds);
		GetWindowInfo(hWnd, &sInfo);

		auto& sEntry = m_List.emplace_back();

		sEntry.Handle = hWnd;
		sEntry.Info = sInfo;
		sEntry.Bounds = sBounds;
		sEntry.FlagsLg = GetWindowLong(hWnd, GWL_STYLE);
		sEntry.FlagsEx = GetWindowLong(hWnd, GWL_EXSTYLE);
		sEntry.Name = std::string(pszName);
		sEntry.ClassName.resize(MAX_PATH);

		GetWindowThreadProcessId(hWnd, &sEntry.OwnerProcId);
		GetClassName(hWnd, sEntry.ClassName.data(), static_cast<int>(sEntry.ClassName.size()));

		return TRUE;
	}

	std::recursive_mutex m_Mutex{};
	std::vector<PROCESSENTRY32> m_ProcessList;
};

FORCEINLINE BOOLEAN C_WindWalker::Populate()
{
	auto sLock = MutexLock_t(m_Mutex);

	auto sProcessListOpt = Shared::Util::GetProcList();

	if (sProcessListOpt == std::nullopt)
		return FALSE;

	m_ProcessList = *sProcessListOpt;

	if (!EnumWindows(EnumWindProc, NULL))
		return FALSE;

	PostProcess();

	return TRUE;
}

FORCEINLINE VOID C_WindWalker::PostProcess()
{
	for (auto& sEntry : m_List)
	{
		if (sEntry.OwnerProcId)
			sEntry.OwnerProcName = Shared::Util::GetProcNameByIdEx(sEntry.OwnerProcId, m_ProcessList);
	}
}

FORCEINLINE VOID C_WindWalker::Dump()
{
	auto sLock = MutexLock_t(m_Mutex);

	if (m_List.empty())
		return;

	std::cout << _XS("NAME") << std::setw(75) << _XS("PID") << std::setw(75) <<
		_XS("PNAME") << std::setw(75) << _XS("BOUNDS") << std::setw(75) << std::endl;

	auto TruncateString = [](const std::string& inputData, size_t maxLength) -> std::string
	{
		if (inputData.length() < maxLength)
			return inputData;

		return inputData.substr(0, maxLength);
	};

	for (const auto& sEntry : m_List)
	{
		if (sEntry.Name.empty())
			continue;

		std::cout << TruncateString(sEntry.Name, 20) << std::setw(75) << sEntry.OwnerProcId << std::setw(75) <<
			sEntry.OwnerProcName << std::setw(75) << fmt::format(_XS("({}, {})"), sEntry.Bounds.right, sEntry.Bounds.bottom) <<
			std::setw(75) << std::endl;
	}
}

FORCEINLINE std::optional<C_WindWalker::Wind_t> C_WindWalker::FindViableWindow()
{
	const auto GetMonitorBounds = [](LPRECT pOut) -> BOOLEAN
	{
		auto hDesktop = GetDesktopWindow();

		if (hDesktop == nullptr)
			return FALSE;

		if (!GetWindowRect(hDesktop, pOut))
			return FALSE;

		return TRUE;
	};

	const auto CheckWindowFlags = [](const Wind_t& sEntry) -> BOOLEAN
	{
		const auto uFlags = sEntry.FlagsLg;
		const auto uFlagsEx = sEntry.FlagsEx;

		if (!(uFlagsEx & WS_EX_LAYERED) || !(uFlagsEx & WS_EX_TRANSPARENT) || !(uFlagsEx & WS_EX_TOPMOST))
			return FALSE;

		return TRUE;
	};

	const auto CheckWindowBounds = [&GetMonitorBounds](const Wind_t& sEntry) -> BOOLEAN
	{
		auto sMonitor = RECT{};
		const auto sBounds = sEntry.Bounds;

		if (!GetMonitorBounds(&sMonitor))
			return FALSE;

		//printf(" checking window bounds, l: %i, t: %i, r: %i, b: %i.\n", sBounds.left, sBounds.top, sBounds.right, sBounds.bottom);

		return TRUE;
	};

	auto sLock = MutexLock_t(m_Mutex);

	if (m_List.empty())
		return {};

	BOOLEAN bFound = FALSE;

	auto sResult = Wind_t{};
	auto sDisplay = RECT{};

	if (!GetMonitorBounds(&sDisplay))
		return {};

	for (const auto& sEntry : m_List)
	{
		// if we pick that one we'll get blurry screen for some reason.
		//if (sEntry.Name == XOR_STD_STRING("amd dvr overlay"))
			//continue;

		const auto& sBounds = sEntry.Bounds;

		if (sEntry.ClassName.empty() || sEntry.OwnerProcName.empty())
			continue;

		if (sEntry.OwnerProcName != "explorer.exe")
			continue;

		if (!CheckWindowFlags(sEntry) || !CheckWindowBounds(sEntry))
			continue;

		bFound = TRUE;
		sResult = sEntry;
		break;
	}

	if (bFound)
		return std::make_optional(sResult);

	return {};
}

FORCEINLINE std::optional<C_WindWalker::Wind_t> C_WindWalker::FindWindowByName(const std::string& name, Math::range_t<INT> range)
{
	auto sLock = MutexLock_t(m_Mutex);

	if (m_List.empty())
		return {};

	BOOLEAN bFound = FALSE;

	auto sResult = Wind_t{};
	auto sDisplay = RECT{};

	for (const auto& sEntry : m_List)
	{
		// if we pick that one we'll get blurry screen for some reason.
		//if (sEntry.Name == XOR_STD_STRING("amd dvr overlay"))
			//continue;

		const auto& sBounds = sEntry.Bounds;

		if (sEntry.Name == name)
		{
			if(range.min && range.max)
			{
				// min = width, max = height.
				if (sBounds.right < range.min || sBounds.bottom < range.max)
					continue;
			}

			bFound = TRUE;
			sResult = sEntry;

			break;
		}
	}

	if (bFound)
		return std::make_optional(sResult);

	return {};
}

FORCEINLINE std::optional<C_WindWalker::Wind_t> C_WindWalker::FindWindowInProcess(const std::string& name, Math::range_t<INT> range)
{
	auto sLock = MutexLock_t(m_Mutex);

	if (m_List.empty())
		return {};

	BOOLEAN bFound = FALSE;

	auto sResult = Wind_t{};
	auto sDisplay = RECT{};

	for (const auto& sEntry : m_List)
	{
		// if we pick that one we'll get blurry screen for some reason.
		//if (sEntry.Name == XOR_STD_STRING("amd dvr overlay"))
			//continue;

		const auto& sBounds = sEntry.Bounds;

		if (sEntry.OwnerProcName == name)
		{
			if (range.min && range.max)
			{
				// min = width, max = height.
				if (sBounds.right < range.min || sBounds.bottom < range.max)
					continue;
			}

			bFound = TRUE;
			sResult = sEntry;

			break;
		}
	}

	if (bFound)
		return std::make_optional(sResult);

	return {};
}