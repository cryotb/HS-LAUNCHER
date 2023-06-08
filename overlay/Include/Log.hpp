#pragma once

namespace Log
{
	inline void Info(const std::string& message)
	{
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("INFO"), message);

		return fmt::print(formatted_output);
	}

	inline void Warning(const std::string& message)
	{
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("WARN"), message);

		return fmt::print(formatted_output);
	}


	inline void Error(const std::string& message)
	{
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("ERROR"), message);

		MessageBox(nullptr, message.c_str(), _XS("Error"), NULL);

		return fmt::print(formatted_output);
	}

	inline void Fatal(const std::string& message)
	{
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("FATAL"), message);

		return fmt::print(formatted_output);
	}

	template <typename... T>
	void Info(const std::string& message, T&&... arguments)
	{
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("INFO"), message);

		return fmt::print(formatted_output, arguments...);
	}

	template <typename... T>
	void Warning(const std::string& message, T&&... arguments)
	{
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("WARN"), message);

		return fmt::print(formatted_output, arguments...);
	}

	template <typename... T>
	void Error(const std::string& message, T&&... arguments)
	{
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("ERROR"), message);

		return fmt::print(formatted_output, arguments...);
	}

	template <typename... T>
	void Fatal(const std::string& message, T&&... arguments)
	{
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("FATAL"), message);

		return fmt::print(formatted_output, arguments...);
	}

	inline void Debug(const std::string& message)
	{
#ifdef _DEBUG
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("DEBUG"), message);

		return fmt::print(formatted_output);
#endif
	}

	template <typename... T>
	void Debug(const std::string& message, T&&... arguments)
	{
#ifdef _DEBUG
		SYSTEMTIME system_time{};
		GetSystemTime(&system_time);

		const auto formatted_output = fmt::format(_XS("<{}:{}:{}> [{}]  {}\n"),
			system_time.wHour, system_time.wMinute, system_time.wSecond,
			_XS("DEBUG"), message);

		return fmt::print(formatted_output, arguments...);
#endif
	}
}
